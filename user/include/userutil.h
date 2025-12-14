/** @file   userutil.h
 *  @brief  Macros and wrapper functions to expose general helpful user-space utility.
**/

#ifndef _USERUTIL_H_
#define _USERUTIL_H_

#include <unistd.h>

/** @brief param attribute to avoid compiler warnings */
#define UNUSED __attribute__((unused))

/** @brief candidate return value for a passed test */
#define TEST_PASSED 0x900d7e57

/** @brief candidate return value for a failed test */
#define TEST_FAILED 0x1bad7e57

/** @brief Give some leeway when testing */
#define SLACK       5

/**
 * User level abstraction for a corresponding `mutex_t` in kernel space.
 */
typedef struct {
    uint32_t handle; ///< Address of the inaccessible `mutex_t` in kernel space
} lock_t;

/** @struct     u32_pair
 *  @brief      struct to hold two unsigned int values
 */
typedef struct {
    uint32_t u32_0; /*!< @brief first unsigned int value */
    uint32_t u32_1; /*!< @brief second unsigned int value */
} u32_pair;

/** @struct     u32_and_ptr
 *  @brief      struct to hold unsigned int value and a pointer
 */
typedef struct {
    uint32_t u32;   /*!< @brief unsigned int value */
    void *ptr;      /*!< @brief a pointer */
} u32_and_ptr;

/** @struct     u32_pair_and_ptr
 *  @brief      struct to hold two unsigned int values and a pointer */
typedef struct {
    uint32_t u32_0; /*!< @brief first unsigned int value */
    uint32_t u32_1; /*!< @brief second unsigned int value */
    void *ptr;      /*!< @brief a pointer */
} u32_pair_and_ptr;

/** @brief   wrapper function for creating breakpoint in user application */
void breakpoint();

/** @brief   wrapper for wfi assembly instruction */
void wait_for_interrupt();

/** @brief   print status message with thread id and counter value */
void print_id_count(uint32_t count);

/** @brief   print status message with system time and thread id */
void print_time_id();

/** @brief   print status message with system time, thread id, and counter value */
void print_time_id_count(uint32_t count);

/** @brief   print status message with system time, thread id, thread priority, and counter value */
void print_time_id_prio_count(uint32_t count);

/** @brief   print status message with system time, thread id, and arbitrary message */
void print_time_id_msg(char *msg);

/** @brief   allow thread to do nothing for a given amount of its execution time */
void do_nothing_for(uint32_t t);

/** @brief   allow thread to do nothing until a particular system time has passed */
void do_nothing_until(uint32_t t);

#endif




