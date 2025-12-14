/** @file   stepper.h
 *  @brief  Useful structs and prototypes mirroring the stepper library maintained by Arduino (see here: https://github.com/arduino-libraries/Stepper)
**/

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include "arm.h"
#include "gpio.h"

/// Number of steps in one revolution of the stepper motor
#define STEPPER_STEPS_PER_REVOLUTION 2048

/// Port for control wire 1
#define STEPPER_CONTROL_PORT_1 P1

/// Pin for control wire 1
#define STEPPER_CONTROL_PIN_1 8

/// Port for control wire 2
#define STEPPER_CONTROL_PORT_2 P0

/// Pin for control wire 2
#define STEPPER_CONTROL_PIN_2 7

/// Port for control wire 3
#define STEPPER_CONTROL_PORT_3 P0

/// Pin for control wire 3
#define STEPPER_CONTROL_PIN_3 26

/// Port for control wire 4
#define STEPPER_CONTROL_PORT_4 P0

/// Pin for control wire 4
#define STEPPER_CONTROL_PIN_4 27

/**
 * Specifies the direction of rotation for the attached stepper motor (stored internally)
 */
typedef enum {
    StepperCW, ///< Clockwise rotation when viewed from top (indicated by a positive step count)
    StepperCCW ///< Counter clockwise rotation when viewed from top (indicated by a negative step count)
} stepper_direction;

/**
 * Struct containing all of the metadata needed to drive a stepper motor with four control wires.
 */
typedef struct {
    uint32_t step_number; ///< Current step in the four step sequence (0-3)
    uint32_t steps_per_revolution; ///< Number of steps for one complete motor revolution
    stepper_direction direction; ///< Current direction of rotation
    gpio_port control_port_1; ///< Port of the first control wire
    uint32_t control_pin_1; ///< Pin within the port of the first control wire
    gpio_port control_port_2; ///< Port of the second control wire
    uint32_t control_pin_2; ///< Pin within the port of the second control wire
    gpio_port control_port_3; ///< Port of the third control wire
    uint32_t control_pin_3; ///< Pin within the port of the third control wire
    gpio_port control_port_4; ///< Port of the fourth control wire
    uint32_t control_pin_4; ///< Pin within the port of the fourth control wire
} stepper_t;

/// Stepper motor currently attached to the nrf52840 (assuming only one used at a time)
extern stepper_t attached_stepper;

/**
 * Initializer for 4-wire control sequence.
 * This is the number of control pins available with the driver board and is also the default of the above referenced library.
 * Creates an instance of stepper_t with the chosen parameters and maintains it in the global variable.
 */
int stepper_init(uint32_t steps_per_revolution, gpio_port control_port_1, uint32_t control_pin_1, gpio_port control_port_2, uint32_t control_pin_2, gpio_port control_port_3, uint32_t control_pin_3, gpio_port control_port_4, uint32_t control_pin_4);

/**
 * Sets the speed of rotation for the motor in revolutions per minute.
 * Used to determine delay until applying next control sequence.
 */
int stepper_speed(uint32_t rpm);

/**
 * Move the motor through the specified number of steps in the specified direction.
 * The apparent rotation of the motor is determined as the ratio between 
 * the number of steps per revolution and steps_to_move.
 * This call is blocking - waits until all steps are completed.
 * A CW rotation is applied if steps_to_move is positive and a CCW rotation is applied if steps_to_move is negative
 */
int stepper_move(int32_t steps_to_move);

/**
 * Advances the current step of the motor to the next sequence for the 4-wire configuration.
 * Direction is determined by the current direction setting in attached_stepper.
 */
void stepper_advance_step();

#endif