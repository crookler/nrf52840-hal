/** @file   userutil.h
 *  @brief  Contains user-space functions stubs to invoke helpful assembly syscalls.
**/

#ifndef _USYSCALL_H_
#define _USYSCALL_H_

/** 
 * Indicator that a user-space application only wants memory isolation for the kernel or if it wants memory isolation for all user threads as well.
 */
typedef enum {
    KERNEL_PROTECT, ///< Only the kernel should have its memory protected
    THREAD_PROTECT ///< Kernel and threads should be isolated in memory from each other
} mpu_mode;

/// User level stub for sleep_ms syscall (this function is implemented in assembly and invoking this function will automatically place needed arguements in correct registers for SVC_C_Handler)
void sleep_ms(unsigned int ms);

/// User level stub for lux_read syscall (will call assembly svc implementation upon linking - return value will be in r0 from SVC_C_Handler)
unsigned short lux_read();

/// User level stub for neopixel_set (will call assembly svc implementation upon linking which populates correct registers)
void neopixel_set(unsigned char red, unsigned char green, unsigned char blue, unsigned int pix_index);

/// User level stub for neopixel_load (loading the internal sequence created with neopixel_set for individual LEDs)
void neopixel_load();

/**
 * User level stub for requesting the paritioning of kernel and user stack space for multiple threads (`num_threads` pieces each with both process and main stack size of `stack_bytes`) with an optional `idle_function` task to perform when no other thread is scheduled.
 * Also specifies an indicator to determine if the threads should be isolated from each other in memory (or just from the kernel).
 * Additionally specifies the number of unique locks that the user-level application will user
 */
int multitask_request(unsigned int num_threads, unsigned int stack_bytes, void* idle_function, mpu_mode mpu_protect, unsigned int num_locks);

/// User level stub for spawning a thread that asynchronously executes the given `fn` with an optional `arg` and a given `id` (id must be unique for thread to spawn). Also configures the task with worst case execution time `c` and period `t`
int thread_define(unsigned int id, void *fn, void *arg, unsigned int c, unsigned int t);

/// User level stub for specifying the frequency of the scheduling call (with a call with zero being interpretted as a non-preemptive scheduler - i.e. the scheduler will not run unless a task explicitly yields)
int multitask_start(unsigned int freq);

/// User level stub for returning the thread id of the currently running thread
unsigned long thread_id();

/// User level stub for manually yielding the running thread and returning control to the scheduler
void thread_yield();

/// User level stub for suspending the currently running thread (does not make end syscall with infinite loop trap)
void thread_end();

/// User level stub for getting the up-time of the system (number of scheduling periods that have passed)
unsigned long get_time();

/// User level stub for returning the number of scheduling cycles that this thread has been active for 
unsigned long thread_time();

/// User level stub for returning the priority of the the current thread 
unsigned long thread_priority();

/// User level stub for initializing a lock (kernel must already have defined empty structs for mutexes by the time this function is called from user space) which also has a space for specifiying the `prio` or the task ID with the associated priority ceiling
lock_t *lock_init(unsigned int prio);

/// User level stub for locking a lock with the opaque address `m` 
void lock(lock_t *m);

/// User level stub for unlocking a lock with the opaque address `m` 
void unlock(lock_t *m);

/// User level stub for setting the speed of an attached stepper motor
int set_stepper_speed(unsigned int speed_rpm);

/// User level stub for moving the stepper motor through a specified number of steps (blocking) with direction based on sign of num_steps (positive is CW and negative is CCW)
int move_stepper(int num_steps);

/// Take a measurement from the ultrasonic sensor and report back range in cm (blocking)
unsigned int ultrasonic_read();

#endif
