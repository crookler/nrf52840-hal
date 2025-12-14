/** @file   mutex.c
 *  @brief  Implements a generic binary semaphore for handling race conditions.
**/

#include "mutex.h"
#include "error.h"

/**
 * Set the lock variable `m` to an initial value of 1 to indicate that it is currently unlocked.
 * Also populate additional initial bookkeeping fields for the implementation of PCP.
 */
void mutex_init(volatile mutex_t *m) {
    m->s = 1;
    m->current_locker = NULL;
    m->num_blocked_threads = 0;
    m->priority_ceiling = 0xFFFFFFFF;
    m->highest_locker_id = 0xFFFFFFFF;
    data_mem_barrier(); // Ensure write occurs before other function calls
}

/**
 * Repeatedly checks the current value of the provided semaphore in a loop until both the value is read to be greater than 0 and it is exclusively written to 0.
 * If either the value is read as 0 initially or the exclusive write fails, the function sleeps until notified of an event change.
 */
void mutex_lock(volatile mutex_t *m) {
    // Execute infinite loop until manually broken by getting all the way through the loop
    while (1) {
        // Check if lock status was zero and go to sleep if it was
        uint32_t mutex_unlocked = load_exclusive((uint32_t *)m);
        if (!mutex_unlocked) {
            wait_for_event();
            continue; // Go back to top of loop
        }

        // Try to store 0 in m->s (try to lock) and sleep if unsuccesful
        uint32_t write_failed = store_exclusive((uint32_t *)m, 0);
        if (write_failed) {
            wait_for_event();
            continue; // Go back to top of loop
        }

        // If got this far, then the lock was acquired and m->s was set to 0 by this function call
        data_mem_barrier(); // Ensure m->s = 0 before returning
        break;
    }
}

/**
 * Attempts to immediately lock the provided mutex `m`.
 * Returns 0 (success) if the mutex was successfully acquired, else returns 1 (failure).
 * Essentially the same as mutex_lock without the outer while loop and the wait_for_event calls (instead just returns 0).
 */
uint32_t mutex_try(volatile mutex_t *m) {
    // Check if lock status was zero
    uint32_t mutex_unlocked = load_exclusive((uint32_t *)m);
    if (!mutex_unlocked) {
        return 1; // Lock already held
    }

    // Try to store 0 in m->s (try to lock) 
    uint32_t write_failed = store_exclusive((uint32_t *)m, 0);
    if (write_failed) {
        return 1; // m->s was accessed/modified before this call
    }

    // If got this far, then the lock was acquired and m->s was set to 0 by this function call
    data_mem_barrier(); // Ensure m->s = 0 before returning
    return SUCCESS;
}

/**
 * Returns the negation of the current value of the semaphore parameter `m` (either locked or unlocked).
 * Returns 0 (success) if the mutex is unlocked and 1 (failure) if the mutex is locked.
 */
uint32_t mutex_is_locked(volatile mutex_t *m) {
    // Read value of s and return its negation (0 locked -> 1 boolean true locked) and (1 unlocked -> 0 boolean false locked)
    return !(m->s);
}

/**
 * Sets the value of m->s back to 1 (unlocked).
 * Signals any stale function calls in `mutex_lock` to try to reacuire the lock.
 */
void mutex_unlock(volatile mutex_t *m) {
    m->s = 1; // Lock available to get (do not need store_exclusive here since not in contention with anything else to update value from 1 to 0 - already holds lock)
    data_mem_barrier(); // Ensure lock is set to 1 before signalling wakeup
    send_event();
}