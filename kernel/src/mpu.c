/** @file   mpu.c
 *  @brief  Helper functions for managing the MPU and using its peripheral registers in the SCS.
**/

#include "mpu.h"
#include "printk.h"
#include "multitask.h"
#include "error.h"
#include "syscall.h"


/** @brief  enables an aligned memory protection region
 *
 *  Helper function used locally to wrap mapping desired MPU configuration into
 *  proper encodings for MPU RNR, RBAR, and RASR fields.
 *
 *  @param  region      region number to enable
 *  @param  base_addr   region's base address
 *  @param  size_log2   ceil(log_2) of the region's size
 *  @param  execute     indicator/non-zero if region is executable
 *  @param  write       indicator/non-zero if region is writable
 *
 *  @return -1 on error, 0 otherwise
 */
int mpu_region_enable(uint32_t region, void *base_addr, uint32_t size_log2, uint8_t execute, uint8_t write) {
    if(region > MPU_RNR_REGION_MAX) {
        printk("error: invalid region number\n");
        return -1;
    }

    if(size_log2 - 1 > MPU_RASR_SIZE_MAX || size_log2 - 1 < MPU_RASR_SIZE_MIN) {
        printk("error: invalid region size\n");
        return -1;
    }

    if((uint32_t)base_addr & ((1 << size_log2) - 1)) {
        printk("error: misaligned region base address\n");
        return -1;
    }

    MPU_RNR = (region & MPU_RNR_REGION_MAX) << MPU_RNR_REGION_POS;
    MPU_RBAR = ((uint32_t)base_addr) & MPU_RBAR_ADDR_MASK;

    uint32_t xn = (execute ? 0 : 1) << MPU_RASR_XN_POS;
    uint32_t ap = (write ? MPU_RASR_AP_RW : MPU_RASR_AP_RO) << MPU_RASR_AP_POS;
    uint32_t size = ((size_log2 - 1) & MPU_RASR_SIZE_MAX) << MPU_RASR_SIZE_POS;
    uint32_t en = 1 << MPU_RASR_ENABLE_POS;

    MPU_RASR = xn | ap | size | en;

    return 0;
}

/** @brief  Disables a memory protection region
 *
 *  @param  region  Region number to disable
 */
void mpu_region_disable(uint32_t region) {
    MPU_RNR = (region & MPU_RNR_REGION_MAX) << MPU_RNR_REGION_POS;
    MPU_RASR &= ~(1 << MPU_RASR_ENABLE_POS);
}

/**
 * Creates default memory protection regeions for user test, user read only data, user data/bss, heap, and main thread stacks. 
 * Enables the MPU with the background memory region and allows high priority system handler access to be handled by MPU.
 */
void mpu_enable() {
    // Linker symbols for enforcing RX to .user_text section of memory
    // Set this protection region up as region 0
    extern uint32_t __svc_stub_start; 
    extern uint32_t __user_text_end;
    uint32_t region_size_log2 = ceil_log2((uint32_t)&__user_text_end - (uint32_t)&__svc_stub_start);
    mpu_region_enable(0, (void *)&__svc_stub_start, region_size_log2, 1, 0);

    // Linker symbols for enforcing RO to .user_rodata section of memory
    extern uint32_t __user_rodata_start;
    extern uint32_t __user_rodata_end;
    region_size_log2 = ceil_log2((uint32_t)&__user_rodata_end - (uint32_t)&__user_rodata_start);
    mpu_region_enable(1, (void *)&__user_rodata_start, region_size_log2, 0, 0);

    // Linker symbols for enforcing RW to .data (but only for user sections)
    extern uint32_t __user_data_start;
    extern uint32_t __user_data_end;
    region_size_log2 = ceil_log2((uint32_t)&__user_data_end - (uint32_t)&__user_data_start);
    mpu_region_enable(2, (void *)&__user_data_start, region_size_log2, 0, 1);

    // Linker symbols for enforcing RW to .bss (but only for user sections)
    extern uint32_t __user_bss_start;
    extern uint32_t __user_bss_end;
    region_size_log2 = ceil_log2((uint32_t)&__user_bss_end - (uint32_t)&__user_bss_start);
    mpu_region_enable(3, (void *)&__user_bss_start, region_size_log2, 0, 1);

    // Linker symbols for enforcing RW to the heap
    // Heap grows up (so base is lowest address)
    extern uint32_t __heap_base;
    extern uint32_t __heap_limit;
    region_size_log2 = ceil_log2((uint32_t)&__heap_limit - (uint32_t)&__heap_base);
    mpu_region_enable(4, (void *)&__heap_base, region_size_log2, 0, 1);

    // Linker symbols for enforcing RW to the main thread's user and process stack
    // Stack grows down (so limit is lowest address)
    extern uint32_t __user_process_stack_limit;
    extern uint32_t __user_process_stack_base;
    region_size_log2 = ceil_log2((uint32_t)&__user_process_stack_base - (uint32_t)&__user_process_stack_limit);
    mpu_region_enable(5, (void *)&__user_process_stack_limit, region_size_log2, 0, 1);

    // Enable the memory management fault (not activated by default - bit 16 of the SHCSR)
    SHCSR |= (1 << MEMFAULT_SHCSR_ENABLE_OFFSET);
    
    // Enable MPU (with background region also enabled) and let the MPU handle hard faults
    MPU_CTRL = (1 << MPU_CTRL_HFNMIENA_POS) | (1 << MPU_CTRL_PRIVDEFENA_POS) | (1 << MPU_CTRL_ENABLE_POS);
    data_sync_barrier();
}

/**
 * Disables the MPU and regresses to using the default memory map for unpriviledged access (i.e. flips HFNMIENA and ENABLE bits). 
 */
void mpu_disable() {
    MPU_CTRL &= ~((1 << MPU_CTRL_HFNMIENA_POS) | (1 << MPU_CTRL_ENABLE_POS));
}

/**
 * Creates a user stack thread memory protection region according to whether the user wants to protect individual threads or just the kernel.
 * If the user application only wants to protect kernel, a single region covers the entire range of thread user stack space (else this thread should only access its own user stack space).
 * Thread will have RW access to its own user stack space.
 */
void mpu_thread_region_enable(void *base_addr, uint32_t size) {
    // Designate region 6 as dealing with the user stack
    mpu_region_enable(6, base_addr, ceil_log2(size), 0, 1);
    data_sync_barrier();
}

/**
 * Deactive the memory protection region associated with the user stack.
 * Will always deactive region 6 (as this is what is assigned above).
 */
void mpu_thread_region_disable() {
    mpu_region_disable(6);
    data_sync_barrier();
}

/**
 * Creates a kernel stack thread memory protection region according to whether the user wants to protect individual threads or just the kernel.
 * If the user application only wants to protect kernel, a single region covers the entire range of thread kernel stack space (else this thread should only access its own kernel stack space)
 * Thread will have RW access to its own kernel stack space.
 */
void mpu_kernel_region_enable(void *base_addr, uint32_t size) {
    // Designate region 7 as dealing with the kernel stack
    mpu_region_enable(7, base_addr, ceil_log2(size), 0, 1);
    data_sync_barrier();
}

/**
 * Deactive the memory protection region associated with the kernel stack.
 * Will always deactive region 7 (as this is what is assigned above).
 */
void mpu_kernel_region_disable() {
    mpu_region_disable(7);
    data_sync_barrier();
}

/**
 * Takes the `psp` of the faulting instruction and compares against known flags in the CFSR to determine the cause of the MemFault.
 * If the psp experienced stack overflow or underflow (and potentially corrupted other stacks), the entire user application exists.
 * Otherwise, just the out-of-line thread is ended (through the kernel level call to syscall_thread_end).
 */
void MemFault_C_Handler(void *psp) {
    // Print a few fields from the CFSR, then clear them since they're sticky
    printk("Memory Fault\n");
    if(CFSR & (1 << CFSR_MSTKERR_POS)) {
        printk("* MemFault occurred on exception entry (MSTKERR)\n");
        CFSR |= (1 << CFSR_MSTKERR_POS);
    }
    if(CFSR & (1 << CFSR_MUNSTKERR_POS)) {
        printk("* MemFault occurred on exception return (MUNSTKERR)\n");
        CFSR |= (1 << CFSR_MUNSTKERR_POS);
    }
    if(CFSR & (1 << CFSR_DACCVIOL_POS)) {
        printk("* Data access violation (DACCVIOL)");
        if(CFSR & (1 << CFSR_MMFARVALID_POS))
            printk(" @ address = 0x%x", MMFAR);
        printk("\n");
        CFSR |= (1 << CFSR_DACCVIOL_POS);
    }
    if(CFSR & (1 << CFSR_IACCVIOL_POS)) {
        printk("* Instruction access violation (IACCVIOL)");
        if(CFSR & (1 << CFSR_MMFARVALID_POS))
            printk(" @ address = 0x%x", MMFAR);
        printk("\n");
        CFSR |= (1 << CFSR_IACCVIOL_POS);
    }

    // Check if there is an unrecoverable stack overflow or underflow (for not the main thread)
    // If so, print an appropriate error message and exit the application with a suitable error code
    if (active_thread_index < num_user_threads+1) {
        void* current_process_stack_base = user_threads[active_thread_index].base_process_stack;
        void* current_process_stack_limit = user_threads[active_thread_index].limit_process_stack;
        if (psp >= current_process_stack_base) {
            printk("MemFault occured because user thread with ID %d experienced underflow of process stack\n", user_threads[active_thread_index].id);
            syscall_exit(THREAD_MEMORY_OUT_OF_BOUNDS_ACCESS);
        }
        if (psp < current_process_stack_limit) {
            printk("MemFault occured because user thread with ID %d experienced overflow of process stack\n", user_threads[active_thread_index].id);
            syscall_exit(THREAD_MEMORY_OUT_OF_BOUNDS_ACCESS);
        }
    }

    // Check if the fault occurred when the main thread is running
    // If so, print an appropriate error message and exit the application with a suitable error code
    if (active_thread_index == num_user_threads+1) {
        extern uint32_t __user_process_stack_limit;
        extern uint32_t __user_process_stack_base;
        if ((uint32_t)psp >= (uint32_t)__user_process_stack_base) {
            printk("MemFault occured because main thread experienced underflow of process stack\n");
            syscall_exit(MAIN_MEMORY_OUT_OF_BOUNDS_ACCESS);
        }
        if ((uint32_t)psp < (uint32_t)__user_process_stack_limit) {
            printk("MemFault occured because main thread experienced overflow of process stack\n");
            syscall_exit(MAIN_MEMORY_OUT_OF_BOUNDS_ACCESS);
        }
    }
        
    // Otherwise, force the offending thread to end
    // Invoke scheduler to schedule next available thread
    syscall_thread_end();
}
