/** @file   stepper.c
 *  @brief  Implementation of 4-wire stepper motor control using timer peripheral for custom speed and bidirectional support
**/

#include "stepper.h"
#include "timer.h"
#include "error.h"

/// Stepper motor currently attached to the nrf52840
stepper_t attached_stepper;

/// Boolean flag indicated if the stepper motor has been initialized (motor must be initialized before any other actions can be done)
uint8_t stepper_init_called = 0;

/**
 * Initialize a stepper motor configuration in the global `attached_stepper`.
 * Start the 4-step control sequence at step zero and forward direction. 
 * Initialize associated gpio pins for control wires (IN1-IN4).
 */
int stepper_init(uint32_t steps_per_revolution, gpio_port control_port_1, uint32_t control_pin_1, gpio_port control_port_2, uint32_t control_pin_2, gpio_port control_port_3, uint32_t control_pin_3, gpio_port control_port_4, uint32_t control_pin_4) {
    // Initialize the motor configuration (default to first step CW)
    attached_stepper.step_number = 0; 
    attached_stepper.steps_per_revolution = steps_per_revolution;
    attached_stepper.direction = StepperCW;  

    // Initialize gpio pins and bind them to the stepper motor (stepper owns these pins now)
    // Check if any of the initializations failed
    int rv = gpio_init(control_port_1, control_pin_1, Output, Pullnone, S0S1);
    rv |= gpio_init(control_port_2, control_pin_2, Output, Pullnone, S0S1);
    rv |= gpio_init(control_port_3, control_pin_3, Output, Pullnone, S0S1);
    rv |= gpio_init(control_port_4, control_pin_4, Output, Pullnone, S0S1);
    if (rv) return rv;

    // Bind pins to the attached stepper
    attached_stepper.control_port_1 = control_port_1;
    attached_stepper.control_pin_1 = control_pin_1;
    attached_stepper.control_port_2 = control_port_2;
    attached_stepper.control_pin_2 = control_pin_2;
    attached_stepper.control_port_3 = control_port_3;
    attached_stepper.control_pin_3 = control_pin_3;
    attached_stepper.control_port_4 = control_port_4;
    attached_stepper.control_pin_4 = control_pin_4;

    stepper_init_called = 1;
    return SUCCESS;
}

/**
 * Set the stepper motor speed to the requested revolutions per minute.
 * Stepping delay is managed by TIMER0, so determine the necessary frequency of interrupts for TIMER0 to have.
 * TIMER0 handler. is responsible for actually calling the advance motor functions during its interrupt.
 */
int stepper_speed(uint32_t rpm) {
    // Don't allow for setting the speed if the stepper has not been initialized.
    if (!stepper_init_called) return STEPPER_MOTOR_UNINITIALIZED;
    uint32_t step_delay_us = 60 * 1000 * 1000 / attached_stepper.steps_per_revolution / rpm;
    uint32_t frequency = 1000000 / step_delay_us;
    timer0_init(frequency);
    return SUCCESS;
}

/**
 * Move the stepper motor through a number of steps according to `step_to_move`.
 * Initializes a timer peripheral to begin firing interrupts at a rate equal to the `step_delay_us` of the attached stepper.
 * Interrupts are fired repeatedly until steps_to_move have been counted, after which the timer peripheral is de-activated.
 * This call is blocking so that user threads can accurately profile the time needed to turn the motor (i.e. do not have to dynamically change the task period based on how often the motor needs to be turned - can eliminate yielding by just treating the interrupt handler as work)
 * Both CW and CCW directions are supported (sign of steps_to_move indicates direction with positive indicating CW and negative indicating CCW).
 */
int stepper_move(int32_t steps_to_move) {
    // Return early if the incorrect number of steps
    if (!stepper_init_called) return STEPPER_MOTOR_UNINITIALIZED;
    
    // Set direction based on sign of arguement
    if (steps_to_move >= 0) {
        attached_stepper.direction = StepperCW;
    } else {
        attached_stepper.direction = StepperCCW;
    }

    // Start timer and fire the correct number of interrupts until steps are handled
    timer0_num_interrupts_after_start = steps_to_move >= 0 ? steps_to_move : -steps_to_move;
    timer0_start();
    while (timer0_num_interrupts_already_handled < timer0_num_interrupts_after_start) continue; // Wait in a busy loop until the TIMER handler has handled the correct number of interrupts
    return SUCCESS;
}

/**
 * Advance the stepper motor to the next step in the sequence.
 * Direction is determined by `direction` field in `attached_stepper`.
 * Apply the control sequence of the current step and then figure out the next step based on the direction of motion.
 */
void stepper_advance_step() {
    // Apply the current step pattern
    switch (attached_stepper.step_number) {
      case 0:  // 1010
        gpio_set(attached_stepper.control_port_1, attached_stepper.control_pin_1);
        gpio_clr(attached_stepper.control_port_2, attached_stepper.control_pin_2);
        gpio_set(attached_stepper.control_port_3, attached_stepper.control_pin_3);
        gpio_clr(attached_stepper.control_port_4, attached_stepper.control_pin_4);
        break;
      case 1:  // 0110
        gpio_clr(attached_stepper.control_port_1, attached_stepper.control_pin_1);
        gpio_set(attached_stepper.control_port_2, attached_stepper.control_pin_2);
        gpio_set(attached_stepper.control_port_3, attached_stepper.control_pin_3);
        gpio_clr(attached_stepper.control_port_4, attached_stepper.control_pin_4);
        break;
      case 2:  //0101
        gpio_clr(attached_stepper.control_port_1, attached_stepper.control_pin_1);
        gpio_set(attached_stepper.control_port_2, attached_stepper.control_pin_2);
        gpio_clr(attached_stepper.control_port_3, attached_stepper.control_pin_3);
        gpio_set(attached_stepper.control_port_4, attached_stepper.control_pin_4);
        break;
      case 3:  //1001
        gpio_set(attached_stepper.control_port_1, attached_stepper.control_pin_1);
        gpio_clr(attached_stepper.control_port_2, attached_stepper.control_pin_2);
        gpio_clr(attached_stepper.control_port_3, attached_stepper.control_pin_3);
        gpio_set(attached_stepper.control_port_4, attached_stepper.control_pin_4);
        break;
    }
    
    // Advance to next step based on direction
    if (attached_stepper.direction == StepperCW) {
        // Forward for CW
        attached_stepper.step_number++;
        if (attached_stepper.step_number >= 4) {
            attached_stepper.step_number = 0;
        }
    } else { 
        // Backward for CCW
        if (attached_stepper.step_number == 0) {
            attached_stepper.step_number = 3;
        } else {
            attached_stepper.step_number--;
        }
    }
}