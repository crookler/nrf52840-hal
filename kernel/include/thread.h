/** @file   thread.h
 *  @brief  Common ethread structs and constants needed according source files aiding with mulititasking
**/

#ifndef _THREAD_H_
#define _THREAD_H_

#include "arm.h"

// The maximum number of threads that the user is allowed to specifiy (included 2 additional threads - the main thread and the idle thread)
#define MAX_NUM_THREADS (14)

/// The maximum number of locks that the user application is allowed to use to coordinate between threads
#define MAX_USER_LOCKS (32)

/// Enforce a maximum of 32kB of space for the user thread stacks (max for only a single stack)
#define MAX_TOTAL_THREAD_STACK_SIZE (32768)

/**
 * The possible states that the thread can be in (running state). 
 */
typedef enum {
    ThreadRunning, ///< Thread is currently running
    ThreadReady, ///< Thread is available to be scheduled
    ThreadWaiting, ///< Thread is waiting for next period
    ThreadBlocked, ///< Thread is blocked on a resource and cannot progress (attempting to acquire a locked mutex for instance)
    ThreadDefunct ///< Thread is not schedulable
} thread_state;

/**
 * TCB containing all of the necessary metadata to completely encapsulate the current running state of a thread.
 */
typedef struct {
    uint32_t id; ///< Task ID
    void* base_process_stack; ///< The base address of the process stack for this thread (not current pointer)
    void* base_main_stack; ///< The base address of the main stack for this thread (not current pointer)
    void* limit_process_stack; ///< The limit address of the process stack for this thread (not current pointer - used for detecting under/overflow)
    void* limit_main_stack; ///< The limit address of the kernel stack for this thread (not current pointer - used for detecting under/overflow)
    void* psp; ///< Process stack pointer (user space) of the thread
    void* msp; ///< Main stack pointer of the thread
    thread_state state; ///< Execution status of the thread 
    uint32_t c; ///< Worst case execution time in a single period
    uint32_t t; ///< Amount of time between releases for this task
    uint32_t static_priority; ///< Total ordering of the threads in the system (from 0 to num_threads) with 0 being the highest priority thread based on period and ID fields
    uint32_t dynamic_priority; ///< Dynamic priority of the current thread (will either equal the static priority or an inherited dynamic priority)
    uint32_t active_time; ///< The total number of scheduler periods that this task has been scheduled since global start
    uint32_t remaining_work; ///< Number of scheduler periods that this task still needs to be active before its next period
    uint32_t time_until_release; ///< Number of scheduler periods until the next instance of this task arrives
    uint32_t svc_status; ///< Boolean variable determining whether the executing thread was handling an SVC request when it was suspended
} tcb_t;

/**
 * Contents that is manually saved on the MSP so that a copy of the registers is not needed to be stored in the TCB directly.
 * Ensures that all of the information is available for use 
 */
typedef struct {
    uint32_t psp; ///< User stack address
    uint32_t r4; ///< Register 4 old content (callee saved)
    uint32_t r5; ///< Register 5 old content (callee saved)
    uint32_t r6; ///< Register 6 old content (callee saved)
    uint32_t r7; ///< Register 7 old content (callee saved)
    uint32_t r8; ///< Register 8 old content (callee saved)
    uint32_t r9; ///< Register 9 old content (callee saved)
    uint32_t r10; ///< Register 10 old content (callee saved)
    uint32_t r11; ///< Register 11 old content (callee saved)
    uint32_t lr; ///< Exec return address (how to return out of handler)
} main_stackframe_t;

#endif