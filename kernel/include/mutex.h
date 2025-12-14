/** @file   mutex.h
 *  @brief  Provides stubs and data structures for implementing mutual exclusion.
**/

#ifndef _MUTEX_H_
#define _MUTEX_H_

#include "arm.h"
#include "thread.h"

/// C wrapper function for exclusive store (does not perform store if address has been accessed since load - returns a 1 if unsuccessful)
intrinsic uint32_t store_exclusive(uint32_t *addr, uint32_t value) {
    uint32_t status;
    asm volatile("strex %0, %1, [%2]" : "=r" (status) : "r" (value), "r" (addr));
    return status;
}

/// C wrapper function for exclusive load (tracking any subsequent accesses to this address)
intrinsic uint32_t load_exclusive(uint32_t *addr) {
    uint32_t value;
    asm volatile("ldrex %0, [%1]" : "=r" (value) : "r" (addr));
    return value;
}

/**
 * Contains fields for semaphores and lock metadata (such as time locked and last locker).
 * Address to mutex_t struct will also be address to semaphore struct field.
 */ 
typedef struct {
    uint32_t s; ///< Semaphore value (1 means unlocked and 0 means locked)
    tcb_t* current_locker; ///< Specifies the address of the TCB that current holds this lock
    tcb_t* blocked_threads[MAX_NUM_THREADS]; ///< Specifies all of the threads that are currently block waiting for this mutex to unlock
    uint32_t num_blocked_threads; ///< Specifies the length of the blocked_threads field (i.e. the number of waiting threads)
    uint32_t priority_ceiling; ///< Priority of the task specified as being the highest locker of this lock
    uint32_t highest_locker_id; ///< Priority of the thread whose static priority gets assigned to the priority ceiling mentioned above
} mutex_t;

/**
 * Initializes a provided mutex_t struct and prepares it for locking/unlocking.
 */
void mutex_init(volatile mutex_t *m);

/**
 * Decrements the current value of the semaphore (locking) if s > 0.
 * Otherwise sleeps until signalled that locking is possible.
 */
void mutex_lock(volatile mutex_t *m);

/**
 * Attempts to immediately lock the provided mutex `m`.
 * Returns 0 if the mutex was successfully acquired, else returns 1.
 */
uint32_t mutex_try(volatile mutex_t *m);

/**
 * Returns the current value of the semaphore parameter `m` (either locked or unlocked).
 */
uint32_t mutex_is_locked(volatile mutex_t *m);

/**
 * Releases the lock on the mutex parameter `m`.
 * Signals any waiting tasks/handlers that the lock is available to be acquired.
 */
void mutex_unlock(volatile mutex_t *m);

#endif