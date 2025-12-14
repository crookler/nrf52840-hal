/** @file   svc_num.h
 *  @brief  Contains constants for SVC numbers corresponding to syscalls.
 */ 

#ifndef _SVC_NUM_H_
#define _SVC_NUM_H_

/// SVC number for SBRK system call
#define SVC_SBRK 0

/// SVC number for write system call
#define SVC_WRITE 1

/// SVC number for read system call
#define SVC_READ 2

/// SVC number for exit system call
#define SVC_EXIT 3

/// SVC number for sleep system call
#define SVC_SLEEP_MS 22

/// SVC number for read read system call
#define SVC_LUX_READ 23

/// SVC number for neopixel set system call
#define SVC_NEOPIXEL_SET 24

/// SVC number for loadd neopixel sequence system call
#define SVC_NEOPIXEL_LOAD 25

/// SVC number for multitask request system call
#define SVC_MULTITASK_REQUEST 31

/// SVC number for thread define system call
#define SVC_THREAD_DEFINE 32

/// SVC number for multitask start system call
#define SVC_MULTITASK_START 33

/// SVC number for thread id system call
#define SVC_THREAD_ID 34

/// SVC number for thread yield system call
#define SVC_THREAD_YIELD 35

/// SVC number for thread end system call
#define SVC_THREAD_END 36

/// SVC number for get time system call
#define SVC_GET_TIME 37

/// SVC number for thread_time system call
#define SVC_THREAD_TIME 38

/// SVC number for thread priority system call
#define SVC_THREAD_PRIORITY 39

/// SVC number of lock init system call
#define SVC_LOCK_INIT 41

/// SVC number of lock system call
#define SVC_LOCK 42

/// SVC number of unlock system call
#define SVC_UNLOCK 43

/// SVC number of stepper set speed system call
#define SVC_STEPPER_SET_SPEED 51

/// SVC number of stepper move system call
#define SVC_STEPPER_MOVE 52

/// SVC number for ultrasonic sensor measurement system call
#define SVC_ULTRASONIC_SENSOR_READ 53

#endif