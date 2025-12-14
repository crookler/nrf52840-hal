/** @file   multitask.h
 *  @brief  Stores syscall function prototypes and kernel level structs for managing inter-process states.
**/

#ifndef _MULTITASK_H_
#define _MULTITASK_H_

#include "arm.h"
#include "thread.h"
#include "mpu.h"
#include "mutex.h"


/// Array of TCB's of threads specificed by user
extern tcb_t user_threads[MAX_NUM_THREADS+2];

/// Number of active threads that the user specified (cannot be greater than MAX_NUM_THREADS)
extern uint8_t num_user_threads;

/// Number of threads that are still active from what the user initially specified
extern uint8_t num_active_threads;

/// Active index of the thread in user_thread
extern uint8_t active_thread_index;

/// The current timeslot that the scheduler is using since program start
extern uint32_t global_timeslot_counter;

/// Used for determining the source of the schedulign decision (asserted when scheduling is performed from systick timer)
extern uint8_t preemption_flag;

/// Maintain the utilization of the currently active task set (used in admission control)
extern float total_utilization; 

/// Array of locks for coordination between user threads
extern mutex_t user_locks[MAX_USER_LOCKS];

/// Number of locks requested by the user application
extern uint8_t num_user_locks;

/// Number of locks already defined by the user application (should not exceed num_user_locks)
extern uint8_t num_defined_locks;

/// Current priority ceiling for user tasks (i.e. the maximum priority ceiling for all currently held locks)
extern uint32_t global_priority_ceiling;

/// Address of the lock that currently is dictating the global_priority_ceiling (i.e. the lock who set the current global priority ceiling equal to its priority ceiling)
extern mutex_t* highest_priority_lock;

/**
 * Returns the next thread id that will be scheduled (from currently active threads) using a rate-monotonic scheduler
 */
uint32_t schedule_rms();

/**
 * Syscall requesting the paritioning of kernel and user stack space for multiple threads (`num_threads` pieces each with both process and main stack size of `stack_bytes`) and specifies an optional `idle_function` for when no other tasks are schedulable
 * Also specifies the number of locks that have be used by the user application.
 */
int syscall_multitask_request(uint32_t num_threads, uint32_t stack_bytes, void* idle_function, mpu_mode mpu_protect, uint32_t num_locks);

/**
 * Syscall spawning a thread that asynchronously executes the given `fn` with an optional `arg` and a given `id` (id must be unique for thread to spawn) along with periodic behavior given by worst-case execution time `c` and period `t`
 */
int syscall_thread_define(uint32_t id, void *fn, void *arg, uint32_t c, uint32_t t);

/**
 * Syscall specifying the frequency of the scheduling call (with a call with zero being interpretted as a non-preemptive scheduler - i.e. the scheduler will not run unless a task explicitly yields)
 */
int syscall_multitask_start(uint32_t freq);

/**
 * Syscall for returning the thread id of the currently running thread
 */
uint32_t syscall_thread_id();

/**
 * Syscall for manually yielding the running thread and returning control to the scheduler
 */
void syscall_thread_yield();

/**
 * Syscall for suspending the currently running thread (does not make end syscall with infinite loop trap)
 */
void syscall_thread_end();

/**
 * Syscall for getting the up-time of the system (number of scheduling periods that have passed)
 */
uint32_t syscall_get_time();

/**
 * Syscall for returning the number of scheduling cycles that this thread has been active for 
 */
uint32_t syscall_thread_time();

/**
 * Syscall for returning the priority of the the current thread 
 */
uint32_t syscall_thread_priority();

/**
 * Syscall for initializing a lock (kernel must already have defined empty structs for mutexes by the time this function is called from user space) with an additional parameter to specify the highest locker ID `prio`
 */
mutex_t* syscall_lock_init(uint32_t prio);

/**
 * Syscall for locking a lock with the opaque address `m` (will be of a different type at user level but maps to a mutex_t struct at kernel level)
 */ 
void syscall_lock(mutex_t* m);

/** 
 * Syscall for unlocking a lock with the opaque address `m` (lock_t at userlevel and mutex_t at kernel level)
 */ 
void syscall_unlock(mutex_t* m);

#endif