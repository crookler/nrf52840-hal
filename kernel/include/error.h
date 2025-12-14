/** @file   error.h
 *  @brief  Constants for unified error codes across all peripherals. Each error code specifies exactly one unique error.
**/

#ifndef _ERROR_H_
#define _ERROR_H_

// This is a unified file for specifying all error codes used among peripherals (with the goal of error codes being uniquely identifiable to determine issue).

/**
 * Return value of any function returning a numeric type if the pin is out of range for the given port (i.e. >15 for P1 and >31 for P0). 
 * Must be positive to match uint8_t return type.
 */ 
#define GPIO_INVALID_PORT_ERROR_CODE 5

/// Standard definition of 0 being successful function completion
#define SUCCESS 0

/// Error code for previous value being clobbered before being read into read buffer
#define I2C_OVERRUN_ERROR_CODE -1

/// Error code for address byte not being acknowledged by follower
#define I2C_ADDRESS_NACK_ERROR_CODE -2

/// Error code for data byte not being acknowledged by follower
#define I2C_DATA_NACK_ERROR_CODE -3

/// Error code for an invalid arguement passed to `i2c_leader_write` or `i2c_leader_read`
#define I2C_INVALID_BUFFER_ERROR_CODE -4

/// Returned if any of the inputted parameters for the PWM configuration are outisde the 15-bit or 24-bit ranges of certain registers
#define PWM_INVALID_ARG_RANGE_ERROR_CODE -5

/// Returned if the SysTick timer is trying to be instantiated with an invalid value
#define SYSTICK_INVALID_ARG -6

/// Returned if the scheduling frequency for multitask start syscall is out of valid range
#define MULTITASK_START_INVALID_FREQ -7

/// Returned if multitasking is attempting to be started without a suitable user thread
#define MULTITASK_START_WITHOUT_THREAD -8

/// Returned if parameters for multitask_request are malformed (out of allowable range)
#define MULTITASK_REQUEST_INVALID_PARAMS -9

/// Returned if multitask request is trying to be called more than once
#define MULTITASK_REQUEST_REPEATED -10

/// Returned if a thread is trying to be defined without a suitable TCB (either no call to multitask request or space has been exhausted)
#define THREAD_DEFINE_NO_TCB -11

/// Returned if a thread is defined with an id already present in the user defined threads
#define THREAD_DEFINE_DUPLICATE -12

/// Returned if a malformed arguements are trying to define a thread
#define THREAD_DEFINE_INVALID_ARGS -13

/// Returned if task cannot be safely admitted without putting task set at risk of missing a deadline
#define THREAD_DEFINE_UNSAFE_ADMISSION -14

/// Returned if stack overflow/underflow occurred for a thread stack
#define THREAD_MEMORY_OUT_OF_BOUNDS_ACCESS -15

/// Returned if main thread experiences overflow/underflow 
#define MAIN_MEMORY_OUT_OF_BOUNDS_ACCESS -16

/// Returned if user threads try to set a highest locker as a non-specified thread
#define LOCK_SPECIFIES_NONEXISTENT_HIGHEST_LOCKER -17

/// Returned if the stepper motor is attempting to be used without initialization
#define STEPPER_MOTOR_UNINITIALIZED -18

#endif