/** @file   gpiote.c
 *  @brief  Overrides weak GPIOTE interrupt handler with functionality for various configured GPIOTE channels
**/

#include "gpiote.h"
#include "events.h"
#include "reset.h"
#include "ultrasonic.h"
#include "timer.h"

/**
 * Determine which GPIOTE channel caused an interrupt. 
 * Clear the event that triggered this handler and then perform the specified action based on the GPIOTE channel specified.
 */
void GPIOTE_Handler() {
    // Figure out which channel fired 
    if (*(volatile uint32_t *)GPIOTE_EVENTS_IN_ADDR(RESET_GPIOTE_CHANNEL)) {
        // Reset button
        // Manually pends the AIRCR register to notify system that a reset is being requested. 
        volatile uint32_t* gpiote_events_in_register = (volatile uint32_t *)GPIOTE_EVENTS_IN_ADDR(RESET_GPIOTE_CHANNEL);
        *gpiote_events_in_register = NotGenerated;

        volatile uint32_t* aircr_register = (volatile uint32_t *)AIRCR_ADDR;
        *aircr_register = (AIRCR_VECTKEY << AIRCR_VECTKEY_INDEX) | (1 << AIRCR_SYSRESETREQ_INDEX);

        // Infinite loop to make sure this handler never returns
        while (1) {}
    } else if (*(volatile uint32_t *)GPIOTE_EVENTS_IN_ADDR(ULTRASONIC_GPIOTE_CHANNEL)) {
        // Ultrasonic sensor 
        // Reset flag
        volatile uint32_t* gpiote_events_in_register = (volatile uint32_t *)GPIOTE_EVENTS_IN_ADDR(ULTRASONIC_GPIOTE_CHANNEL);
        *gpiote_events_in_register = NotGenerated;

        // This GPIOTE channel is configured to listen for any changes on the pin (so assuming that the pin starts out low the in_measurement flag represents the parity / oddness of the number of ultrasonic GPIOTE interrupts handled)
        // If in a measurement, the signal is high and a falling edge is being waited on (i.e. the thing that caused this interrupt)
        // If not in a measurement, the signal is low and a rising edge is being waited on (i.e. the thing that caused this interrupt)
         if (in_measurement) {
            // In a measurement and a valid amount of time elapsed since the range measurement was started
            // Reset flag
            in_measurement = 0;

            // Stop TIMER1 and capture current value of the timer to CC1 (do not overwrite CC0)
            timer1_stop();
            volatile uint32_t* timer_task_capture_register = (volatile uint32_t *)TIMER_TASKS_CAPTURE_ADDR(TIMER1_BASE_ADDR, CC1);
            *timer_task_capture_register = TRIGGER;

            // Read value just placed in CC1 (TIMER1 counts at 1 MHz so this is the elapsed time in uS)
            // Divide elapsed time in uS by 58 to get range in centimeters (per datasheet)
            uint32_t elapsed_time_us = *(volatile uint32_t *)TIMER_CC_ADDR(TIMER1_BASE_ADDR, CC1);
            last_ultrasonic_measurement = elapsed_time_us / 58;
        } else {
            // Just started the timing for a measurement (rising edge)
            in_measurement = 1;
            timer1_start();
        }
    }
}