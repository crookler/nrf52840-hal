/** @file   peripheral_trap.h
 *  @brief  Contains syscalls that are unique to providing custom peripheral interaction to user-space (all functionality tied to specific peripherals and not generic syscalls)
**/

#ifndef _PERIPHERAL_TRAP_H_
#define _PERIPHERAL_TRAP_H_

#include "arm.h"

/**
 * sleep_ms system call acting as a wrapper for the systick_delay function.
 */
void syscall_sleep_ms(uint32_t ms);

/**
 * lux_read system call to provide an ambient light measurement to the user.
 */
uint16_t syscall_lux_read();

/**
 * neopixel_set system call acting as a wrapper for the neopixel_set function.
 */
void syscall_neopixel_set(uint8_t red, uint8_t green, uint8_t blue, uint32_t pix_index);

/**
 * neopixel_load system call acting as a wrapper for the pix_load_sequence function.
 */
void syscall_neopixel_load();

/**
 * stepper_set_speed system call for specifying the RPM of the attach stepper motor (essentially a wrapper of the set_speed kernel function)
 */
int syscall_stepper_set_speed(uint32_t rpm);

/**
 * stepper_move_steps system call for rotating the stepper motor through a specified number of steps with direction based on sign of steps_to_move (wrapper of move kernel function - blocking)
 */
int syscall_stepper_move_steps(int32_t steps_to_move);

/**
 * Wrapper for the ultrasonic_read system call that calls ultrasonic_range kernel functionality and returns the measured range (in cm) - blocking
 */
uint32_t syscall_ultrasonic_read();

#endif