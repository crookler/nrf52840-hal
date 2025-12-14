/** @file   kernel.c
 *  @brief  Entry point of the code with a singular main function defined.
**/

#include "rtt.h"
#include "mpu.h"
#include "printk.h"
#include "stepper.h"
#include "pix.h"
#include "ultrasonic.h"
#include "reset.h"

extern void enter_user_mode(void);

/**
 * Main kernel function and entry point for uC.
 * This function initializes relevant peripherals and then transitions to the user_mode application specified in the USERAPP linking arguement.
 */
int kernel_main() {
    // Initializations for integrated peripherals (and floating point computation)
    reset_enable();
    rtt_init();
    enable_fpu();
    mpu_enable();
    pix_init();
    stepper_init(STEPPER_STEPS_PER_REVOLUTION, STEPPER_CONTROL_PORT_1, STEPPER_CONTROL_PIN_1, STEPPER_CONTROL_PORT_3, STEPPER_CONTROL_PIN_3, STEPPER_CONTROL_PORT_2, STEPPER_CONTROL_PIN_2, STEPPER_CONTROL_PORT_4, STEPPER_CONTROL_PIN_4); // Sequence assumes 3-wired declared as second arguement (for some reason)
    stepper_speed(10); // 10 RPM default speed
    ultrasonic_init();

    // Enter user mode directly (should never return from here)
    enter_user_mode();
    return -1;
}