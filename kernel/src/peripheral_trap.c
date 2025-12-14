/** @file   peripheral_trap.c
 *  @brief  Implements syscalls that drive custom peripheral functionality (not standardized)
**/

#include "peripheral_trap.h"
#include "i2c.h"
#include "pix.h"
#include "systick.h"
#include "stepper.h"
#include "ultrasonic.h"

/**
 * Actively waits by calling the blocking systick_delay function with `ms`.
 */
void syscall_sleep_ms(uint32_t ms) {
    systick_delay(ms);
}

/**
 * Takes a measurements on the i2c of the lux sensor.
 * Returns the uint16_t measurement (which is packaged into r0 by Systick_C_Handler)
 */
uint16_t syscall_lux_read() {
    uint8_t command_code = 0x04; // Register for sensor values
    uint8_t lux_values[2];

    // Issue read to lux sensor lux sensor
    // Package returned bytes into sensor value measurement
    i2c_leader_write(&command_code, 1, LUX_BASE_ADDRESS);
    i2c_leader_read(lux_values, 2, LUX_BASE_ADDRESS);
    i2c_leader_stop();

    uint16_t sensor_value = ((lux_values[1] << 8) | lux_values[0]); /// LSB comes back first
    return sensor_value;
}

/**
 * Wraps the neopixel_set functionality (passing in red, green, and blue to existing peripheral function).
 * Sets the provided color scheme to `pix_index` in the internal array of colors.
 * Assumes that the peripheral was initialized by kernel_main.
 */
void syscall_neopixel_set(uint8_t red, uint8_t green, uint8_t blue, uint32_t pix_index) {
    pix_color_set(red, green, blue, pix_index);
}

/**
 * Wraps the neopixel_load functionality (indicating for PWM peripheral to begin loading the configured sequence).
 */
void syscall_neopixel_load() {
    pix_load_sequence();
}

/**
 * Wraps the stepper_speed functionality (passing in rpm from user space to be configured on the global stepper motor).
 */
int syscall_stepper_set_speed(uint32_t rpm) {
    return stepper_speed(rpm);
}

/**
 * Wraps the stepper_move kernel level functionality.
 * The kernel level implementation is blocking so that the user space application accurately understands when the motor is again ready to move.
 * Treat the blocking time as work performed by the TIMER0 handler (i.e. still have it be profiled by the motor control task to avoid guessing game).
 */
int syscall_stepper_move_steps(int32_t steps_to_move) {
    return stepper_move(steps_to_move);
}

/**
 * Wraps the ultrasonic_range kenrnel level functionality.
 * The kernel level implementation is blocking so user space can accurately profile how long work takes to get a sensor measurement.
 * Treat the blocking time as work performed by the TIMER1 handler.
 */
uint32_t syscall_ultrasonic_read() {
    return ultrasonic_range();
}