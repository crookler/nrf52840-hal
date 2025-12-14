/** @file   syscall.c
 *  @brief  Implementation of SVC handler allowing for generic software-pended exceptions and select NEWLIB required and other custom syscalls.
**/

#include "svc_num.h"
#include "syscall.h"
#include "peripheral_trap.h"
#include "multitask.h"
#include "rtt.h"
#include "printk.h"
#include "gpio.h"

/**
 * Takes the provided stack pointer and retrieves the value of the PC (next instruction to execute in user space).
 * Uses the PC to retreive the SVC instruction with the svc_num immediate value.
 * Calls the appropriate syscall implementation depending on the svc_num in the svc instruction.
 */
void SVC_C_Handler(void *psp) {
    stack_frame_t *s = (stack_frame_t *)psp;

    // svc_num is located in the lower byte of the previously executed svc instruction (also was a thumb instruction so only 2 bytes subtracted)
    // System is also little endian so the desired byte is the lower addressable byte (i.e. want pc-2 so just immediately derefrence this byte)
    uint8_t svc_num = *(uint8_t*)(s->pc - 2);

    // Place return value in s->r0 for syscalls that return a value (casting any return value to uint32_t)
    // Also places the expected number of arguements in the correct order from r0, r1, r2, and r3 with casts to correct arguement type
    if (svc_num == SVC_SBRK) {
        s->r0 = (uint32_t)syscall_sbrk((int)s->r0); 
    } else if (svc_num == SVC_WRITE) {
        s->r0 = (uint32_t)syscall_write((int)s->r0, (char *)s->r1, (int)s->r2);
    } else if (svc_num == SVC_READ) {
        s->r0 = (uint32_t)syscall_read((int)s->r0, (char *)s->r1, (int)s->r2);
    } else if (svc_num == SVC_EXIT) {
        syscall_exit((int)s->r0); // Should not return
    } else if (svc_num == SVC_SLEEP_MS) {
        syscall_sleep_ms(s->r0); // Returns nothing (void)
    } else if (svc_num == SVC_LUX_READ) {
        s->r0 = (uint32_t)syscall_lux_read(); 
    } else if (svc_num == SVC_NEOPIXEL_SET) {
        syscall_neopixel_set((uint8_t)s->r0, (uint8_t)s->r1, (uint8_t)s->r2, s->r3); // Returns nothing (void)
    } else if (svc_num == SVC_NEOPIXEL_LOAD) {
        syscall_neopixel_load(); // Returns nothing (void)
    } else if (svc_num == SVC_MULTITASK_REQUEST) {
        s->r0 = (uint32_t)syscall_multitask_request(s->r0, s->r1, (void *)s->r2, s->r3, *((uint32_t*)psp + 8)); // 5th arguement is on stack after all 8 other registers
    } else if (svc_num == SVC_THREAD_DEFINE) {
        s->r0 = (uint32_t)syscall_thread_define(s->r0, (void *)s->r1, (void *)s->r2, s->r3, *((uint32_t*)psp + 8)); // 5th arguement is on stack after all 8 other registers
    } else if (svc_num == SVC_MULTITASK_START) {
        s->r0 = (uint32_t)syscall_multitask_start(s->r0);
    } else if (svc_num == SVC_THREAD_ID) {
        s->r0 = (uint32_t)syscall_thread_id();
    } else if (svc_num == SVC_THREAD_YIELD) {
        syscall_thread_yield(); // Returns nothing (void)
    } else if (svc_num == SVC_THREAD_END) {
        syscall_thread_end(); // Returns nothing (void)
    } else if (svc_num == SVC_GET_TIME) {
        s->r0 = (uint32_t)syscall_get_time();
    } else if (svc_num == SVC_THREAD_TIME) {
        s->r0 = (uint32_t)syscall_thread_time();
    } else if (svc_num == SVC_THREAD_PRIORITY) {
        s->r0 = (uint32_t)syscall_thread_priority();
    } else if (svc_num == SVC_LOCK_INIT) {
        s->r0 = (uint32_t)syscall_lock_init(s->r0);
    } else if (svc_num == SVC_LOCK) {
        syscall_lock((mutex_t *)s->r0); // Returns nothing (void)
    } else if (svc_num == SVC_UNLOCK) {
        syscall_unlock((mutex_t *)s->r0); // Returns nothing (void)
    } else if (svc_num == SVC_STEPPER_SET_SPEED) {
        s->r0 = (uint32_t)syscall_stepper_set_speed(s->r0);
    } else if (svc_num == SVC_STEPPER_MOVE) {
        s->r0 = (uint32_t)syscall_stepper_move_steps((int32_t)s->r0);
    } else if (svc_num == SVC_ULTRASONIC_SENSOR_READ) {
        s->r0 = (uint32_t)syscall_ultrasonic_read();
    }
}

/// External symbol for accessing heap base (linker script symbol)
extern uint32_t __heap_base;

/// External symbol for accessing __heap_limit (linker script symbol)
extern uint32_t __heap_limit;

/// Maintain current system break output of syscall to preserve value (value is initialized with address of __heap_base label from linker script)
static void* current_system_break = (void *)&__heap_base;

/**
 * Manages the current system break in the heap as allocaed by the linker script (using __heap_base as the initial system break value).
 * The system break value is maintained as a static variable in this file (so it is preserved between system calls).
 * Returns either a void pointer to the previous system break value if successful or (void *) -1 on failure.
 */
void *syscall_sbrk(int incr) {
    // Heap grows upward so check if increment would break above __heap_limit
    // Assumes that the heap is only being grown with this implementation (thus incr is always positive)
    if (incr < 0) {
        return (void *)-1;
    }

    // Calculaute new heap boundary if this increment were to be successful
    // Check if this boundary exceeds address of __heap_limit
    uint32_t new_system_break = (uint32_t)current_system_break + (uint32_t)incr;
    if (new_system_break > (uint32_t)&__heap_limit) {
        return (void *)-1;
    }

    // new_system_break was valid so update current_system_break and return old value
    void* old_system_break = current_system_break;
    current_system_break = (void *)new_system_break;
    return (void *)old_system_break;
}

/**
 * Writes `len` amount of content at `ptr` to stdout. 
 * This write implementation will only work with stdout so the fd should always equal 1 (will return -1 if invalid).
 * Returns number of bytes written on success.
 */
int syscall_write(int file, char *ptr, int len) {
    // This implementation assumes file == 1 (else this indicates an error)
    // Check other conditions as well to ensure len > 0 and ptr is non-null
    if ((file != 1) || (ptr == NULL) || (len < 0)) {
        return -1;
    }

    // Print content using rtt_write (which returns number of bytes written)
    return rtt_write(ptr, len);
}

/**
 * Read a maximum of 'len' amount of content into `ptr` from stdin.
 * This read implementation assumes that content is only ever read from stdin and will return -1 if this is not fd value.
 * Returns number of bytes read on success.
 */
int syscall_read(int file, char *ptr, int len) {
    // This implementation assumes file == 0 (else this indicates an error)
    // Check other conditions as well to ensure len > 0 and ptr is non-null
    if ((file != 0) || (ptr == NULL) || (len < 0)) {
        return -1;
    }

    // Should not block to read content from RTT down buffer
    // Check how many bytes are available using rtt_peek and read either this amount of len (whichever is smaller)
    // Return amount of bytes read
    uint32_t bytes_available = rtt_peek();
    uint32_t read_amount = MIN((uint32_t)len, bytes_available);
    return rtt_read(ptr, read_amount);
}

/**
 * Called when a user-space program returnss or terminates from an error.
 * Prints status message with `status` value and turns on error LED.
 * Sits in an infinite while loop waiting for interrupts as a failsafe error check.
 */
void syscall_exit(int status) {
    // Print status message
    printk("User space returned with status: %d\n", status);

    // Toggle error LED if status was nonzero (red LED is on P1.15)
    if (status) {
        gpio_port error_port = P1;
        uint8_t error_pin = 15;

        // Could be initialized before now but this is safe
        gpio_init(error_port, error_pin, Output, Pullnone, S0S1);
        gpio_set(error_port, error_pin);
    }

    // Disable interrupts and exceptions then wait infinitely for interrupts after exiting (should never occur)
    disable_interrupts();
    while (1) {
        wait_for_interrupt();
    }
}

