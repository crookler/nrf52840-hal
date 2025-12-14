/** @file   userutil.c
 *  @brief  Implementations for generally helpful user-space utility.
**/

#include <stdio.h>
#include "userutil.h"
#include "usyscall.h"

/** @brief    Default idle function used when no idle_function arguement is provided to multitask_request syscall*/
void default_idle() {
    // Spin while waiting for an interrupt (likely from the scheduler)
    while (1) {
        asm volatile("wfi"); 
    }
}

/** @brief   wrapper function for creating breakpoint in user application */
void breakpoint() {
    asm volatile("bkpt");
}

/** @brief   wrapper for wfi assembly instruction */
void wait_for_interrupt() {
    asm volatile("wfi");
}

/** @brief   print status message with thread id and counter value */
void print_id_count(uint32_t count) {
    printf("Thread %lu: count = %lu\n", thread_id(), count);
}

/** @brief   print status message with system time and thread id */
void print_time_id() {
    printf("t=%lu -- Thread %lu\n", get_time(), thread_id());
}

/** @brief   print status message with system time, thread id, and counter value */
void print_time_id_count(uint32_t count) {
    printf("t=%lu -- Thread %lu: count = %lu\n", get_time(), thread_id(), count);
}

/** @brief   print status message with system time, thread id, thread priority, and counter value */
void print_time_id_prio_count(uint32_t count) {
    printf("t=%lu -- Thread %lu: priority = %lu, count = %lu\n", get_time(), thread_id(), thread_priority(), count);
}

/** @brief  print status message with system time, thread id, and arbitrary string */
void print_time_id_msg(char *msg) {
    printf("t=%lu -- Thread %lu: %s\n", get_time(), thread_id(), msg);
}

/** @brief   allow thread to do nothing for a given amount of its execution time */
void do_nothing_for(uint32_t t) {
    uint32_t target = thread_time() + t;
    while(thread_time() < target)
        wait_for_interrupt();
}

/** @brief   allow thread to do nothing until a particular system time has passed */
void do_nothing_until(uint32_t t) {
    while(get_time() < t)
        wait_for_interrupt();
}

