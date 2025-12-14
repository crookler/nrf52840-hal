/** @file   multitask.c
 *  @brief  Implementation of SVC handlers associated with multithreading.
**/

#include "multitask.h"
#include "syscall.h"
#include "systick.h"
#include "error.h"
#include "printk.h"

/// Array of TCB's of threads specificed by user (the active thread will be at index num_user_threads - i.e. one more than the last defined user thread)
tcb_t user_threads[MAX_NUM_THREADS+2] = { 0 };

/// Number of threads that the user specified initialized specified to run (cannot be greater than MAX_NUM_THREADS)
uint8_t num_user_threads;

/// Number of threads that are still active from what the user initially specified (once this hits 0 then the main thread is again scheduled)
uint8_t num_active_threads = 0;

/// Active index of the thread in user_thread
uint8_t active_thread_index;

/// The current timeslot that the scheduler is using (i.e. how many scheduling decisions have been made as a result of timer preemption)
uint32_t global_timeslot_counter = 0;

/// Used to maintain the current utilization of the scheduled tasks (used in admission control)
float total_utilization = 0;

/// Store the memory protection mode for this round of tasks (memory protection regions will be repeatedly created for individual threads if equal to thread_protect - otherwise a single region covering all of the user stack space for user and kernel threads will be created)
mpu_mode protection_status;

/// Array of locks for coordination between user threads
mutex_t user_locks[MAX_USER_LOCKS];

/// Number of locks requested by the user application
uint8_t num_user_locks;

/// Number of locks already defined by the user application (should not exceed num_user_locks)
uint8_t num_defined_locks = 0;

/// Current priority ceiling for user tasks (i.e. the maximum priority ceiling for all currently held locks)
uint32_t global_priority_ceiling = 0xFFFFFFFF;

/// Address of the lock that currently is dictating the global_priority_ceiling (i.e. the lock who set the current global priority ceiling equal to its priority ceiling)
mutex_t* highest_priority_lock = 0;

/// Signal for indiciating that a scheduling decision needs to be made from preemption (and not from an explicit yield - used for charging time units)
uint8_t preemption_flag = 0;

/**
 * Helper function for determining if the current thread holds any locks.
 * Returns a boolean value (0 false / 1 true) indicating if locks are held by the active thread at the point of invoking this function
 */
uint32_t active_thread_holds_locks() {
    // Sweep through the user lock fields and look for any locks that the active thread is the current locker of
    for (uint8_t index = 0; index < num_defined_locks; index++) {
        if (user_locks[index].current_locker == &user_threads[active_thread_index]) {
            return 1;
        }
    }
    return 0;
}

/**
 * Continues to service the PendSV interrupt after the assembly-level interrupt has finished.
 * Accepts a pointer to a main stack frame with all needed information (expected to be called from the assembly-level PendSV_Handler which merely prepares the stack prior to this function being invoked).
 * Performs the essential acts of context switching and runs the global scheduler.
 * Returns a pointer to the new MSP of the task to be run (can restore context from this in the assembly handler)
 */
void* PendSV_C_Handler(void* msp) {
    // Clear signal for handler (not technically necessary since it is automatically cleared but it makes me feel good)
    clr_pendsv();

    // Check if this scheduling decision was made as a result of preemption (if yes charge the time to the current thread - if no do not charge the time yet for both total time active and remaining work)
    // Thread will not be in ThreadRunning state if this was not a result of preemption (so scheduler can work as needed)
    // Decrement the remaining work of the current task (assuming it is not the idle thread)
    // Only change thread back to ready if it is still in running state and there is more work to do (i.e. not done for this period and not defunct/ended) 
    // Thread becomes waiting if computation time for this thread is exhausted
    if (preemption_flag) {
        global_timeslot_counter++; // Only increment global counter if this was a result of preempt (counting time instead of raw number of decision)
        user_threads[active_thread_index].active_time++; // Charge active time (total time)

        // Decrement except for the idle thread (just put idle thread back in ready)
        // Check to see if the thread should actually be waiting instead
        if (active_thread_index != num_user_threads) {
            user_threads[active_thread_index].remaining_work--;
        } 

        // Mark as waiting instead if all of the computation for this period was finished
        // Else say it can run again
        if (user_threads[active_thread_index].remaining_work == 0) { 
            user_threads[active_thread_index].state = ThreadWaiting; 

            // Check if thread held locks when it got put into waiting
            if (active_thread_holds_locks()) {
                printk("Thread with ID %d elapsed computation time while holding a lock\n", user_threads[active_thread_index].id);
            }
        } else {
            user_threads[active_thread_index].state = ThreadReady;
        }
    }

    // Run scheduler to figure out which index in user_threads should be scheduled next (according to RMS)
    uint32_t next_index = schedule_rms();
    preemption_flag = 0; // Reset flag (now that it is no longer needed for scheduling to reduce time until next release)

    // Skip TCB context saving / restoring if next_index == active_thread_index (i.e. rescheduling same thread)
    if (next_index == active_thread_index) {
        user_threads[active_thread_index].state = ThreadRunning; // Change the thread back to running
        return msp; // Just return the MSP that was just passed in (nothing to change)
    }

    // Save current context to the active TCB then switch to new TCB based on next_index returned from scheduling policy
    // Construct a stack frame at MSP arguement so that PSP value can be retrieved and stored in the TCB
    main_stackframe_t* frame = (main_stackframe_t*)(msp);
    user_threads[active_thread_index].psp = (void*)frame->psp;
    user_threads[active_thread_index].msp = msp;
    user_threads[active_thread_index].svc_status = get_svc_status(); // Save current svc status based on register value

    // Restore context of the new thread (from when it was saved on its TCB)
    active_thread_index = next_index;
    user_threads[active_thread_index].state = ThreadRunning;
    set_svc_status(user_threads[active_thread_index].svc_status);

    // If performing thread-wise protection, disable the old stack protection region for the old thread and re-enable it with the current thread
    // Only try to set up the protection region for an actual user defined thread (not main)
    if (protection_status == THREAD_PROTECT && active_thread_index < num_user_threads+1) {
        mpu_thread_region_disable();
        mpu_kernel_region_disable();
        void* process_stack_limit = user_threads[active_thread_index].limit_process_stack;
        void* kernel_stack_limit = user_threads[active_thread_index].limit_main_stack;
        uint32_t stack_size = (uint32_t)user_threads[active_thread_index].base_process_stack - (uint32_t)process_stack_limit; // Size is the same for both stacks
        
        // Enable MPU regions for only this thread's stacks
        mpu_thread_region_enable(process_stack_limit, stack_size);
        mpu_kernel_region_enable(kernel_stack_limit, stack_size);
    }
    
    return user_threads[active_thread_index].msp; // Return pointer to the new MSP to have registers popped off of it
}

/**
 * Scheduling policy using RMS that returns the index in the user_threads array of the next task to be scheduled.
 * Sweeps through the user_threads array and determines the highest priority task (including the idle thread) that is ready to run.
 * Also performs bookkeeping related to maintaining the number of scheduling cycles until a waiting task can be made ready again (i.e. only consider tasks that are ThreadReady but change thread from ThreadWaiting to ThreadReady when time_until_release = 0).
 * Maintain periodic behavior by incrementing time_until_release by thread period every time it equals 0 and by decrementing remaining_work for every cycle the current task is scheduled (reset to computation time upon reaching 0).
 */
uint32_t schedule_rms() {
    // Maintain highest priority that is currently schedulable (and keep track of what index this TCB corresponds to)
    // Set initial state as the idle thread (so if no other thread is schedulable this is returned)
    uint32_t highest_priority = 0xFFFFFFFF;
    uint32_t next_index = num_user_threads;

    // Check if all tasks have exited (return main in this case)
    if (num_active_threads == 0) {
        return num_user_threads+1;
    }

    // Loop through each index in user_threads and determine what the highest priority task is (ignoring defunct threads)
    // Perform bookkeeping as needed to maintain periodic nature for each task
    for (uint8_t index = 0; index < num_user_threads; index++) {
        if (user_threads[index].state == ThreadDefunct) {continue;}
        
        // Check if the current task has higher priority than the highest seen
        // Use ID as tie-breaker in the event of equal priority
        if ((user_threads[index].state == ThreadReady) && (user_threads[index].dynamic_priority < highest_priority)) {
            // New task guranteed to be more important
            highest_priority = user_threads[index].dynamic_priority;
            next_index = index;
        }

        // Decrement the time until release of all active threads if they are not defunct (and this was a preempt round)
        // Reset next period of any tasks that have reached their release point (in time for scheduling decision)
        // Set time until release equal to period and reset the amount of work to do (also mark this thread as being schedulable in a future decision)
        if (preemption_flag) {
            user_threads[index].time_until_release--;
            if (user_threads[index].time_until_release == 0) {
                user_threads[index].time_until_release = user_threads[index].t;
                user_threads[index].remaining_work = user_threads[index].c;
                user_threads[index].state = ThreadReady;
            }
        }
    }

    return next_index;
}

/// External user space declaration for making the svc call to thread_end (should be what is called when user space function end)
extern void thread_end();

/**
 * Helper function that constructs a default user-level stack frame on the PSP to allow this thread to be cleanly scheduled (mirrors the contents of the stack frame as if this thread moved to handler execution).
 * Sets registers to expecting default values based on the values of `fn` and arg` at `index` in user_threads.
 */
void thread_function_define(void *fn, void *arg, uint8_t index) {
    // Construct custom stack frames at MSP and PSP such that registers hold expected values for when this thread is scheduled
    // This allows the new thread to be scheduled like expected (i.e. pushing and popping values off stacks)
    // Start with initial PSP frame
    stack_frame_t* custom_user_frame = (stack_frame_t*)(user_threads[index].base_process_stack);
    custom_user_frame--; // Decrement so that a frame is actually being constructed in the correct slot instead of in the slot immediately below where desired
    
    custom_user_frame->r0 = (uint32_t)arg; // Place arguement in first register (not dereferenced?) if it was specified (otherwise pass in junk - if NULL this is just 0)
    custom_user_frame->r1 = 0; // Junk (Only expecting one arg)
    custom_user_frame->r2 = 0;
    custom_user_frame->r3 = 0;
    custom_user_frame->r12 = 0;
    custom_user_frame->lr = (uint32_t)thread_end | 1; // Call user space thread_end function instead of kernel space to correctly set svc bit (branch to external declaration)
    custom_user_frame->pc = (uint32_t)fn | 1; // Set PC as function pointer with a bitwise or 1 to indicate execution in thumb mode
    custom_user_frame->xpsr = 0x01000000;

    // Define initial MSP frame
    main_stackframe_t* custom_kernel_frame = (main_stackframe_t*)(user_threads[index].base_main_stack);
    custom_kernel_frame--; // Decrement so that a frame is actually being constructed in the correct slot instead of in the slot immediately below where desired
    
    custom_kernel_frame->psp = (uint32_t)custom_user_frame; // Place new PSP address in the first slow of kernel frame
    custom_kernel_frame->r4 = 0; // Don't care
    custom_kernel_frame->r5 = 0; 
    custom_kernel_frame->r6 = 0; 
    custom_kernel_frame->r7 = 0; 
    custom_kernel_frame->r8 = 0; 
    custom_kernel_frame->r9 = 0; 
    custom_kernel_frame->r10 = 0; 
    custom_kernel_frame->r11 = 0; 
    custom_kernel_frame->lr = 0xfffffffd; // Assume uesr thread starts out in unpriviledged thread mode (initially)

    // Update PSP and MSP after decrement (pointing to the top of the new stack frame - lowest address)
    // This may not be strictly necessary with the handler but it is nice to know the struct is self consistent
    user_threads[index].psp = (void*)custom_user_frame;
    user_threads[index].msp = (void*)custom_kernel_frame;
}

/// External symbol for accessing base of thread user stacks (linker script symbol)
extern uint32_t __thread_user_stacks_base;

/// External symbol for accessing base of thread main stacks (linker script symbol)
extern uint32_t __thread_kernel_stacks_base;

/// External user space default idle function (used in the case that idle_function is null in multitask_request)
extern void default_idle();

/// Boolean flag for determining if multitask request was called and successfully returned (used to make sure thread_define does not run unless multitask_request was already called)
uint8_t multitask_request_called = 0;

/// Boolean flag for determining if thread_define was called at least once before multitask_start is called
uint8_t thread_define_called = 0;


/// RMS tight bounds for admission control (used to see if a new task can be safely admitted in thread_define)
float util_bound[] = {
    0.000, 1.000, .8284, .7798, .7568, .7435, .7348, .7286,
    .7241, .7205, .7177, .7155, .7136, .7119, .7106, .7094
};

/**
 * Returns an error code if the allocated space of the request would be larger than 32kB of stack space (or if the number of threads exceeds 14)
 * Otherwise, this implementation sets up the requested stacks by taking slices starting from the base addresses (i.e. stack pointer decreasing from the base_addr - growing down for each num_thread).
 * Transforms the request `stack_bytes` into the next highest power of 2 (ceiling) so that the stacks stay well aligned and behaved.
 * This transformation may result in a rejection of parameters if they are at (but not exactly at the limit) of stack space (i.e. would not be rejected without rounding).
 * Also configures memory regions to prevent unwanted access according to `mpu_protect` policy.
 * Additionally initializes empty mutex_t structs in the global `user_locks` (one empty struct per num_locks specified by user)
 */
int syscall_multitask_request(uint32_t num_threads, uint32_t stack_bytes, void* idle_function, mpu_mode mpu_protect, uint32_t num_locks) {
    // Do not allow repeat multitask request call
    if (multitask_request_called) {
        return MULTITASK_REQUEST_REPEATED;
    }
    // Round stack bytes to the nearest power of two
    // Parition the idle thread in the user space
    uint32_t stack_bytes_aligned = 1 << ceil_log2(stack_bytes);
    uint32_t num_threads_plus_idle = num_threads+1;

    // Check if modified parameters are feasible
    // Check if the number of requested locks is greater than the maximum number of available locks
    // Num_threads cannot be greater than the max or 0 and stack size cannot be greater than the max or 0
    if ((num_threads > MAX_NUM_THREADS) || (num_threads == 0) || ((stack_bytes_aligned * num_threads_plus_idle) > MAX_TOTAL_THREAD_STACK_SIZE) || (num_locks > MAX_USER_LOCKS)) {
        return MULTITASK_REQUEST_INVALID_PARAMS;
    }

    // If params are valid then parition the user and kernel thread stacks according to stack_bytes (rounded up) including a space for the idle thread
    // Create a "dummy" TCB with pointers to the correct MSP and PSP (but without fields actually meaninfully set with id and function to execute)
    // Start at __thread_kernel_stacks_base and decrement by the requested task_bytes
    // Iterate over num_threads+1 such that the idle thread TCB will be correctly partioned and will be at index num_user_threads and the main thread will be at index num_user_threads+1 (14 and 15 respectively in worst case)
    for (uint8_t thread_index = 0; thread_index < (num_threads_plus_idle); thread_index++) {
        uint32_t psp = (uint32_t)&__thread_user_stacks_base - (thread_index)*stack_bytes_aligned;
        uint32_t msp = (uint32_t)&__thread_kernel_stacks_base - (thread_index)*stack_bytes_aligned;

        // ID is initialized as zero, pass in psp/msp pointer, say the thread is defunct/not schedulable (set to ready when actually defined), and indicate it is not coming from SVC (0 - false)
        tcb_t dummy_tcb;
        dummy_tcb.id = 0;
        dummy_tcb.base_process_stack = (void*)psp; // Save the original base address so it can be reset to if the TCB is being overwritten with a new thread (if old TCB becomes defunct)
        dummy_tcb.base_main_stack = (void*)msp; // Save the original base address so it can be reset to if the TCB is being overwritten with a new thread (if old TCB becomes defunct)
        dummy_tcb.limit_process_stack = (void*)(psp - stack_bytes_aligned); // Save the limit for the current stack (to check against when looking for under/overflow)
        dummy_tcb.limit_main_stack = (void*)(msp - stack_bytes_aligned); // Save the limit for the current stack (to check against when looking for under/overflow)
        dummy_tcb.psp = (void*)psp;
        dummy_tcb.msp = (void*)msp;
        dummy_tcb.state = ThreadDefunct;
        dummy_tcb.static_priority = 0xFFFFFFFF; // Priorities will be later overwritten when an absolute ordering is created
        dummy_tcb.dynamic_priority = 0xFFFFFFFF; 
        dummy_tcb.active_time = 0; 
        dummy_tcb.remaining_work = 0;
        dummy_tcb.time_until_release = 0;
        dummy_tcb.svc_status = 0;
        user_threads[thread_index] = dummy_tcb;
    }

    // Set global variables to correct parameters
    // Create a TCB for the main thread
    // Add it to position num_threads+1 of user_threads array (i.e. last index in the array)
    tcb_t main_tcb;
    main_tcb.id = 0;
    main_tcb.psp = 0; // Will need to be set from first call to scheduler (currently running so probably not at linker label)
    main_tcb.msp = 0; // Will need to be set from first call to scheduler (currently running so probably not at linker label)
    main_tcb.state = ThreadRunning;
    main_tcb.active_time = 0; 
    main_tcb.remaining_work = 1; // Will have its remaining_work decremented on first scheduling
    main_tcb.time_until_release = 0;
    main_tcb.svc_status = 0;
    main_tcb.svc_status = 0;
    user_threads[num_threads_plus_idle] = main_tcb;
    num_user_threads = num_threads;
    active_thread_index = num_threads_plus_idle; // Show that the currently active thread is the main thread (i.e. idle_index +1)

    // Flesh out TCB for idle thread (defunct TCB allocated above)
    // Invoke function definition helper function to not influence global state
    // Assume ID FFFFFFFF is valid (but this will not be enforced since other threads will not check this index during admission control)
    // Supply default idle function if null otherwise use user-defined idle
    // Assume idle thread is 100% utilization (i.e. period of 1 and computation time of 1) so that it will always be scheduled if it is the only task
    user_threads[num_threads].id = 0xFFFFFFFF; // Make ID maximum so it always loses in the case of a tie (if for some reason another task has a horrendous period)
    user_threads[num_threads].c = 1;
    user_threads[num_threads].t = 1;
    user_threads[num_threads].state = ThreadReady;
    user_threads[num_threads].static_priority = 0xFFFFFFFF; // Assign lowest possible priority (i.e. incredibly high period) so the idle thread never takes precedence over another valid thread
    user_threads[num_threads].dynamic_priority = 0xFFFFFFFF;
    user_threads[num_threads].remaining_work = 1;
    user_threads[num_threads].time_until_release = 0;

    if (idle_function == NULL) {
        thread_function_define(default_idle, NULL, num_threads); // Place idle at num_thread index
    } else {
        thread_function_define(idle_function, NULL, num_threads);   
    }

    // Check what kind of memory protection the user requested
    // Create a unified block of all stack space (for user and kernel stacks) if kernel_protect
    protection_status = mpu_protect;
    if (mpu_protect == KERNEL_PROTECT) {
        // Only protect the kernel (just lump everything else together)
        extern uint32_t __thread_user_stacks_limit;
        extern uint32_t __thread_kernel_stacks_limit;
        mpu_thread_region_enable(&__thread_user_stacks_limit, MAX_TOTAL_THREAD_STACK_SIZE);
        mpu_kernel_region_enable(&__thread_kernel_stacks_limit, MAX_TOTAL_THREAD_STACK_SIZE);
    } else {
        // Need to create thread regions on the fly so just disable them for now (will be enabled/disabled by scheduler)
        mpu_thread_region_disable();
        mpu_kernel_region_disable();
    }

    // Set global variable for number of user locks (indicates how many structs in user_locks are valid / "initialized")
    num_user_locks = num_locks;
    multitask_request_called = 1; // Flag that this function was called
    return SUCCESS;
}

/**
 * Assigns static priorities of tasks as the tasks are defined (i.e. create an absolute ordering from 0 to num_active_threads based on lower field / tie-breaking in favor of lower priority)
 * Every additional task defined must require the previous ordering to be re-evaluated (since it is not known how the period/ID of the new task will compare to the previous ones).
 * Performs insertion sort (basically) to determine the highest priority task. Would be more efficient to sort user_threads array based on period/ID, but with such a low number of threads this complexity seems acceptable (and requires the least amount of rewrites for creating the new priority system)
 * The next smallest prioritiy number (starting at 0 through num_threads) is assigned to the thread with the lowest period (and in the even to a tie is given to the thread with the lowest ID)
 */
void order_absolute_priorities() {
    uint32_t highest_priority_index;
    uint32_t highest_priority_id;
    uint32_t highest_priority_period;

    // First remove any old priorities that may still be attached to threads
    for (uint8_t assignment_index = 0; assignment_index < num_user_threads; assignment_index++) {
        user_threads[assignment_index].static_priority = 0xFFFFFFFF;
        user_threads[assignment_index].dynamic_priority = 0xFFFFFFFF;
    }

    // Then assign threads the highest priority in the range 0 to num_threads (only considering threads that do not yet have a priority)
    for (uint8_t assignment_index = 0; assignment_index < num_active_threads; assignment_index++) {
        highest_priority_index = 0xFFFFFFFF;
        highest_priority_id = 0xFFFFFFFF;
        highest_priority_period = 0xFFFFFFFF;

        // Only look at threads that have not yet been assigned a priority
        for (uint8_t thread_index = 0; thread_index < num_user_threads; thread_index++) {
            if (user_threads[thread_index].static_priority != 0xFFFFFFFF || user_threads[thread_index].state == ThreadDefunct) continue;

            // Only select as highest priority if it has the highest period or if it has the equal highest period but a lower id
            if ((user_threads[thread_index].t < highest_priority_period) ||
                ((user_threads[thread_index].t <= highest_priority_period) && user_threads[thread_index].id < highest_priority_id)) {
                highest_priority_index = thread_index;
                highest_priority_id = user_threads[thread_index].id;
                highest_priority_period = user_threads[thread_index].t;
            }
        }

        // Assign the best found index tas he lowest remaining priority
        user_threads[highest_priority_index].static_priority = assignment_index;
        user_threads[highest_priority_index].dynamic_priority = assignment_index;
    }
}

/**
 * Performs admission control and parameter validation on the inputted arguements.
 * Overwrites the "dummy" TCB created in multitask_request with the parameter id, async function, and periodic data needed for scheduling.
 * Returns a negative error code if the task cannot be safely accepted, but otherwise accepts the task and places it in an empty location in the user_threads array.
 */
int syscall_thread_define(uint32_t id, void *fn, void *arg, uint32_t c, uint32_t t) {
    // Check if arguements are invalid
    if (fn == NULL || (c == 0) || (t == 0) || (c > t)) {
        return THREAD_DEFINE_INVALID_ARGS;
    }

    // Immediately return if no thread space was set up or if there is no allocated TCBs
    if (!multitask_request_called) {
        return THREAD_DEFINE_NO_TCB;
    }

    // Check to see if ID was already used in the array
    // Also find an index of a TCB to potentially overwrite (with default being the idle thread)
    // If no open TCB (non-defunct) was found, tcb_index will still equal the idle thread (which indicates no space)
    // By searching through whole array (even defunct) it is guranteed that another instance of this ID is not running somewhere else in the array 
    uint8_t tcb_index = num_user_threads;
    for (uint8_t index = 0; index < num_user_threads; index++) {
        if (user_threads[index].id == id && user_threads[index].state != ThreadDefunct) {
            // ID already existed in the valid portion of the array and its status was not defunct
            return THREAD_DEFINE_DUPLICATE;
        }

        if (user_threads[index].state == ThreadDefunct) {
            tcb_index = index;
        }
    }

    // If no open TCB was found then this task cannot be accepted
    if (tcb_index == num_user_threads) {
        return THREAD_DEFINE_NO_TCB;
    }

    // Perform admission control for the provided parameters by comparing new task utilization to the upper bound for the would-be number of tasks
    float new_utilization = ((float)c / (float)t) + total_utilization; // Cast to float to ensure proper number
    if (new_utilization <= util_bound[num_active_threads+1]) {
        // Thread can be accepted
        // Update utliziation
        total_utilization = new_utilization;

        // Overwrite the "dummy" TCB at index `num_active_threads` (incremented from 0)
        // Set the new thread as ready instead of defunct (able to be scheduled now that it has a definition)
        // Set values for id, c, t, and priority (equal to t) - other parameters can be kept at default 0 (also assumes all tasks are released at time 0)
        // When working with thread_end, this ensures that if num_active_threads is less than the total space, it is guranteed to be defunct
        user_threads[tcb_index].id = id;
        user_threads[tcb_index].c = c;
        user_threads[tcb_index].t = t;
        user_threads[tcb_index].state = ThreadReady;
        user_threads[tcb_index].active_time = 0; 
        user_threads[tcb_index].remaining_work = c;
        user_threads[tcb_index].time_until_release = t-1;
        user_threads[tcb_index].svc_status = 0;

        // Call function definition helper to place default values on the stack for this thread
        thread_function_define(fn, arg, tcb_index);
        
        // Increment number of active threads since a new valid thread was just defined
        num_active_threads++;
        thread_define_called = 1; // Flag that this function was called

        // Must recompute static priorities of tasks every time a new one is defined
        // NOTE: If there was a gurantee that defunct tasks (from spawner for instace) are always defined such that the user application is back to the maximum number of threads, this function call could be only made once when num_active_threads == num_user_threads
        // Without this gurantee though (i.e. where a spawner thread could not redefine all threads or the user could request more threads than they actually define) it is safest to make this call for each additional thread defined
        order_absolute_priorities();
        return SUCCESS;
    } else {
        // Thread cannot be accepted with conservative certainty
        return THREAD_DEFINE_UNSAFE_ADMISSION;
    }
}

/// Reset this value on first syscall_multitask_start invocation to make sure it is initialized correctly
extern uint8_t timer_wrap_around;

/// Set this value to something other than 1 if more than one invocation of the SysTick timer handler is needed
extern uint8_t timer_wrap_comparison;


/**
 * Helper function for validating that all of the specified hightest locker IDs are valid in the `user_locks` array.
 * If any ID is invalid / not present in the taskset, the call to multitask_start fails.
 * Otherwise, the priority_ceiling field is populated with the static priority of the specified task
 */
uint8_t validate_ceiling_id() {
    uint8_t thread_found; // Boolean for determining if a thread with the matching ID was found
    for (uint8_t lock_index = 0; lock_index < num_defined_locks; lock_index++) {
        thread_found = 0;
        for (uint8_t thread_index = 0; thread_index < num_user_threads; thread_index++) {
            // Check if a nondefunct thread has the specified ID (use this as the priority ceiling)
            if ((user_threads[thread_index].state != ThreadDefunct) && (user_threads[thread_index].id == user_locks[lock_index].highest_locker_id)) {
                user_locks[lock_index].priority_ceiling = user_threads[thread_index].static_priority;
                thread_found = 1;
                break;
            }
        }

        if (!thread_found) {
            return 0;
        }
    }
    return 1; // All locks are valid if made it to this point
}

/**
 * Configures the SysTick timer to fire with interrupts as fast as 1 second (a frequency of zero of implies that the scheduler is not-preemptive and will only be invoked through explicit).
 * Starts the SysTick timer to begin counting down (with the scheduling decision being pended directly from the SysTick timer handler).
 * This function call is not again scheduled until all other user threads have terminated (i.e. joins the threads) and stops the SysTick timer when all threads have terminated. 
 */
int syscall_multitask_start(uint32_t freq) {
    // Make sure that `freq` is within correct range
    if (freq > SYSTICK_BASE_FREQUENCY) {
        return MULTITASK_START_INVALID_FREQ;
    }

    // Make sure that a thread was actually defined before attempting to start
    if (!thread_define_called) {
        return MULTITASK_START_WITHOUT_THREAD;
    }

    // Initialize global variables for locking and validate locks
    if (!validate_ceiling_id()) {
        return LOCK_SPECIFIES_NONEXISTENT_HIGHEST_LOCKER;
    } 
    global_priority_ceiling = 0xFFFFFFFF; // Set current priority ceiling as low as possible for starting threads (any reasonable periodic task will be able to lock this)
    highest_priority_lock = 0; // Set the current locker to none

    // Check if this scheduler is preemptive (otherwise just call scheduler and do not configure systick timer)
    // Avoid 0 division issue in the case of nonpreemptive scheduler
    if (freq > 0) {
        // Calculate reset value for the inputted frequency (i.e. figure out what the value should be so that the interrupt fires at the regular freqeuncy)
        // If the reset value is higher than the maximum possible reset value, split into bins until it is small enough (i.e. increase number of invocations required)
        uint32_t reload_value = (SYSTICK_BASE_FREQUENCY / freq) -1; // Account for off-by-one in initial assignment (this means the clock could be slightly fast if needing to subdivide) 
        systick_clksource clksource = Processor;
        systick_tickint tickint = Exception; 
        
        // Increase number of bins until the reload value is less than the max
        uint8_t bins = 1;
        while ((reload_value / bins) > MAX_24_BIT) { bins++; }

        // Configure and enable SysTick timer
        timer_wrap_around = 1;
        timer_wrap_comparison = bins;
        int rv = systick_configure(reload_value/bins, clksource, tickint);
        if (rv) return rv;
    }

    // Suspend main thread until all other threads are finished running
    // Going into schedule active_thread_index = num_user_threads so the main thread is active (immediately switched out though by the first schedule)
    // Reset global counter time (if this is not the first time that multitask_start is being invoked)
    global_timeslot_counter = 0;
    set_pendsv();

    // Will only return here after this thread is scheduled again (i.e. all others are terminated)
    // Stop SysTick Timer and return to caller
    systick_disable();
    return SUCCESS;
}

/**
 * Return the ID field of the TCB for the currently running thread.
 */
uint32_t syscall_thread_id() {
    return user_threads[active_thread_index].id;
}

/**
 * Manually pends the PendSV signal to immediately invoke the scheduler.
 * Labels the current thread as waiting (i.e. not to be scheduled again until the start of the next period)
 * Under current scheduler schemes, the thread could be immediately rescheduled if it is the only one remaining (or is it has the highest priority)
 */
void syscall_thread_yield() {
    // Do not allow for placing the idle thread in waiting (should always be schedulable)
    if (active_thread_index != num_user_threads) {
        user_threads[active_thread_index].state = ThreadWaiting;
    }

    // Check if thread held any locks when it yielded
    if (active_thread_holds_locks()) {
        printk("Thread with ID %d suspended while holding a lock\n", user_threads[active_thread_index].id);
    }

    set_pendsv();
}

/**
 * Immediately invokes the scheduler and labels the current thread as having exited (i.e. will not be scheduled).
 * The dead TCB for this thread will still exist in the user_threads array and can be overwritten with a new definition now that this TCB is defunct.
 * Decrements the number of active threads to allow the scheduler to schedule the main thread if all others have ended.
 * Also cleans up the `user_threads` array to allow non-defunct threads to take the place
 * Additionally releases any locks which the ending thread may currently possess
 */
void syscall_thread_end() {
    // Do not allow for idle thread to end
    if (active_thread_index == num_user_threads) {
        return;
    }

    // Sweep through the user lock fields and unlock any locks that were held by this thread (i.e. should not exit while holding a lock)
    for (uint8_t index = 0; index < num_user_locks; index++) {
        if (user_locks[index].current_locker == &user_threads[active_thread_index]) {
            syscall_unlock(&user_locks[index]);
        }
    }

    // Subtract the current thread's utilization from the global utilization
    // Mark the TCB as defunct (able to be overwritten by an ID with the same definition)
    total_utilization -= ((float)user_threads[active_thread_index].c / (float)user_threads[active_thread_index].t);
    user_threads[active_thread_index].state = ThreadDefunct;
    num_active_threads--; // Decrement the value of num_active_threads to potentially alert main thread to run
    set_pendsv();
}

/**
 * A call to this function represents the time that a task occupied rather than the raw number of decisions made as a result of timer preemption.
 * The counter is treated as the absolute source of truth and represents the time after multitask_start initially invokes the scheduler (so counter == 0 implies the scheduler is working on timeslot 0).
 * Timeslot is only incremented when a systick timer interrupt fires (to keep global_timeslot_counter in time with the number of scheduling decisions).
 */
uint32_t syscall_get_time() {
    return global_timeslot_counter;
}

/**
 * Return the active time for the current thread.
 * The active time is the number of scheduling cycles that the thread has been running (i.e. number of times the thread has been scheduled)
 */
uint32_t syscall_thread_time() {
    return user_threads[active_thread_index].active_time;
}

/**
 * Return the static priority of the currently running task.
 * This is determined by the period parameter (with a lower period indiciating a higher priorty) unless a special priority has been assigned manually (such as the case of the idle task or priority inheritance)
 */
uint32_t syscall_thread_priority() {
    return user_threads[active_thread_index].dynamic_priority;
}

/**
 * Checks if there is space to initialize a new mutex.
 * If it can be accomodated, a new mutex from the free batch in user_locks is initialized, and its address is returned.
 * The `prio` field is the thread ID of the user thread whose static priority should be set as highest_priority.
 * There is no gurantee that this function will be called after thread define, so must validate the ID in multitask_start (i.e. before the thing is actually scheduled)
 */
mutex_t* syscall_lock_init(uint32_t prio) {
    // Make sure that multitask request was called before any lock initialization (so the num_locks requested can be known)
    // Lock init should only be called by the main user thread
    if (num_defined_locks >= num_user_locks || active_thread_index != (num_user_threads+1)) {
        return NULL;
    }

    // Initialize the next unitialized mutex and return its address
    mutex_init(&user_locks[num_defined_locks]);
    user_locks[num_defined_locks].highest_locker_id = prio;
    num_defined_locks++;
    return &user_locks[num_defined_locks-1];
}

/**
 * Blocking system call for a user application to request control of a lock.
 * Locks requests are granted / denied according to the original priority ceiling protocol (see interior comments for details about what this implies).
 * Will not return until the lock has been acquired.
 * Since this syscall is blocking, it is guranteed that any thread will only be waiting on a maximum of 1 lock.
 */ 
void syscall_lock(mutex_t* m) {
    // Forcibly end a thread that tries to access a lock with a lower priority ceiling than the current thread's priority (this break the initialization assumption)
    // Only check the static priority (since dynamic priority could feasibly be higher)
    if (user_threads[active_thread_index].static_priority < m->priority_ceiling) {
        // This thread should not be trying to lock this lock - end it
        printk("Thread%d tried to lock a mutex that has a lower priority ceiling than Thread%d's priority\n", user_threads[active_thread_index].id, user_threads[active_thread_index].id);
        syscall_thread_end();
    }

    // Check if the active thread is the current holder of the lock (can lead to deadlock if trying to lock itself again)
    if (m->current_locker == &user_threads[active_thread_index]) {
        printk("Thread%d attempted to lock a mutex it already held\n", user_threads[active_thread_index].id);
        return;
    }

    // Disable interrupts to make sure the locked mutex has its state completely updated before continuing
    disable_interrupts();
    
    // Check if the priority of the active thread is stricly higher than the current priority ceiling (if yes this thread can lock the lock)
    // Can also lock the lock if this thread holds the highest priority lock (i.e. the locked lock that inflated the current priority ceiling)
    // If either of these conditions are true, allow the thread to attempt to lock the lock (another thread still could have beaten it to this point since mutex_try uses atomic operations)
    // mutex_try returns 0 on success
    // If priority is not higher or thread does not hold the mutex, place the thread in the waiting queue for the lock
    while (!(((user_threads[active_thread_index].dynamic_priority < global_priority_ceiling) || (highest_priority_lock->current_locker == &user_threads[active_thread_index])) && mutex_try(m) == 0)) {
        // Disable interrupts so this syscall in particular is free from preemption (critical section enforcing critical sections)
        disable_interrupts();
        
        user_threads[active_thread_index].state = ThreadBlocked;

        // If the current lock is locked, set up as waiting for this lock to unlock
        // Otherwise, the current highest locker (with its heightened global_priority_ceiling) caused the first condition to fail (i.e. lock was open but could not lock it)
        if (mutex_is_locked(m)) {
            m->blocked_threads[m->num_blocked_threads] = &user_threads[active_thread_index];
            m->num_blocked_threads++;
        } else {
            highest_priority_lock->blocked_threads[highest_priority_lock->num_blocked_threads] = &user_threads[active_thread_index];
            highest_priority_lock->num_blocked_threads++;
        }

        // Check if the priority of the current thread is higher than the priority of the thread that currently holds the lock
        // Let the current locker inherit the priority of the active thread if it is higher
        m->current_locker->dynamic_priority = MIN(m->current_locker->dynamic_priority, user_threads[active_thread_index].dynamic_priority);

        // Reenable interrupts before calling scheduler
        // When returning to this thread again, disable interrupts before rechecking condition
        enable_interrupts();
        set_pendsv();
        disable_interrupts();
    }

    // Already acquired the lock in the above condition evaluation so if the thread makes it here, then it acquired the lock
    m->current_locker = &user_threads[active_thread_index];

    // Only update the priority ceiling if this lock was actually a higher priority ceiling than one already locked 
    // Do not want to overwrite a higher priority lock if the second condition of holding the highest priority was true
    if (m->priority_ceiling < global_priority_ceiling) {
        global_priority_ceiling = m->priority_ceiling;
        highest_priority_lock = m;
    }

    // Enable interrupts now that the lock has been acquired and global state is consistent
    enable_interrupts();
}

/** 
 * System call for unlocking the provided mutex `m` (opaque at user level).
 * Since threads are only waiting on a maximum of 1 lock (from above) and unlocks occur immediately (unconditionally), all threads waiting on this lock can immediately move to the ready state to contend for it again (those unsuccessful will go back to the blocked state).
 * Once unlocked, the current mutex has its num_blocked_threads and current_locker field reset (to keep state consistent).
 * After the mutex is unlocked, the new highest_priority_lck is determined, and the new prioriy of the running task is potentially changed to inherit a higher priority task's priority if it is blocking on a lock held by the current thread.
 */ 
void syscall_unlock(mutex_t* m) {
    // Do nothing if the lock was already unlocked
    if (!mutex_is_locked(m)) {
        printk("Thread%d attempted to unlock an already open mutex\n", user_threads[active_thread_index].id);
        return;
    }

    // Update state of the mutex m and free all threads that are blocked by this lock (could immediately go back to sleep on another one but this was the original thing blocking them from progress)
    for (uint8_t index = 0; index < m->num_blocked_threads; index++) {
        m->blocked_threads[index]->state = ThreadReady;
    }

    // Disable interrupts to make sure the unlocked mutex has its state completely updated before continuing
    disable_interrupts();
    
    // Unlock the mutex and reset status for mutex
    mutex_unlock(m);
    m->num_blocked_threads = 0;
    m->current_locker = 0;

    // Change the global priority ceiling to the highest priority ceiling of any locks that are still locked (and update the most important lock)
    // Also potentially change the priority of the currently executing thread if it still holds another lock that a higher priority thread is waiting on (i.e. do not completely de-escalate the dynamic priority automatically back to static priority)
    uint32_t new_global_ceiling = 0xFFFFFFFF;
    mutex_t* new_highest_lock = 0;
    user_threads[active_thread_index].dynamic_priority = user_threads[active_thread_index].static_priority; // Assume the thread goes back to its default static priority initially (which could be overwritten if another higher priority thread is still found to be waiting on it)

    for (uint8_t lock_index = 0; lock_index < num_defined_locks; lock_index++) {
        if (mutex_is_locked(&user_locks[lock_index])) {
            if (user_locks[lock_index].priority_ceiling < new_global_ceiling) {
                new_global_ceiling = user_locks[lock_index].priority_ceiling;
                new_highest_lock = &user_locks[lock_index];
            }

            // The lock is locked but check if it is locked by the active thread
            // If it is locked, potentially re-raise the priority of the thread again (i.e. increase the dynamic priority again and keep it elevated from its original priority)
            if (user_locks[lock_index].current_locker == &user_threads[active_thread_index]) {
                // Current thread is the locker so check to see if there are any other higher priority threads waiting on this lock
                for (uint8_t thread_index = 0; thread_index < user_locks[lock_index].num_blocked_threads; thread_index++) {
                    user_threads[active_thread_index].dynamic_priority = MIN(user_threads[active_thread_index].dynamic_priority, user_locks[lock_index].blocked_threads[thread_index]->dynamic_priority);
                }
            }
        }
    }

    // Update global locking fields
    global_priority_ceiling = new_global_ceiling;
    highest_priority_lock = new_highest_lock;
    user_threads[active_thread_index].state = ThreadReady; // Change currently executing thread to ready (expected by scheduler since this was not a result of preemption)
    
    // Re-enable interrupts and invoke scheduler after mutex state has been made consistent
    enable_interrupts();
    set_pendsv(); 
}
