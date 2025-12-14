/** @file   timer.c
 *  @brief  Configures timer peripheral to match desired a desired frequency and perform a custom action within the TIMER0 interrupt handler.
**/

#include "timer.h"
#include "stepper.h"
#include "ultrasonic.h"

/// Specifies the number of interrupts that should be handled by the TIMER0 handler after a start task is issued
volatile uint32_t timer0_num_interrupts_after_start = 0;

/// Specifies the number of interrupts that have already been handled since the last start task was issued
volatile uint32_t timer0_num_interrupts_already_handled = 0;

/**
 * Configures the TIMER0 peripheral to begin firing interrupts at a rate `freq`.
 * The comparison value for a 1 MHz timer (16 MHz default after subdividing) is calculated and loaded into CC0.
 * Interrupts for the COMPARE[0] event are enabled, and the NVIC is configured to listen for interrupts originating from TIMER0.
 * Finally, the counter is started (begins counting down) at the end of the function call.
 */
void timer0_init(int freq) {
    // Set timer precaler to 16 so timer counts cleanly at 1 MHz (making math easier)
    // Prescaler value is used as exponent for 2 so 2^4 = 16 -> 16 MHz / 16 = 1 MHz
    uint8_t prescaler = 4;
    volatile uint32_t* timer_prescaler_register = (volatile uint32_t *)TIMER_PRESCALER_ADDR(TIMER0_BASE_ADDR);
    *timer_prescaler_register = prescaler;

    // Calculate comparison value for the inputted frequency (i.e. figure out how high the counter needs to count to match the sampling frequency)
    // 16 bit value by default
    // Scaled frequency / freq = counter value
    // Assume only CC[0] for TIMER0 will be used (as is the only capture compare register that will generate interrupts for the TIMER0_Handler)
    timer_cc cc_reg = CC0;
    uint16_t comparison_value = TIMER_BASE_FREQUENCY / (1 << prescaler) / freq;
    volatile uint32_t* timer_cc_register = (volatile uint32_t *)TIMER_CC_ADDR(TIMER0_BASE_ADDR, cc_reg);
    *timer_cc_register = comparison_value;

    // Configure TIMER0 to produce interrupts when the value in CC[0] is reached by the timer
    volatile uint32_t* timer_intenset_register = (volatile uint32_t *)TIMER_INTENSET_ADDR(TIMER0_BASE_ADDR);
    *timer_intenset_register |= (1 << (TIMER_INTENSET_INDEX_OFFSET + cc_reg)); // Enable the COMPARE[0] event to generate interrupts

    // Enable TIMER0 interrupts on NVIC (placing 1 in index TIMER0_IRQ of NVIC_ISER0 since this is the register responsible for the range 0-31)
    volatile uint32_t* nvic_iser0_register = (volatile uint32_t *)NVIC_ISER0_ADDR;
    *nvic_iser0_register |= (1 << TIMER0_IRQ);

    // Reset interrupt counters
    timer0_num_interrupts_after_start = 0;
    timer0_num_interrupts_already_handled = 0;
}

/**
 * Triggers timer to begin handling its own interrupts at the specified frequency (i.e. start to increment the counter value in timer mode).
 * This can be used in default mode (without calling init) but behavior is undefined.
 */
void timer0_start() {
    // Clear any stale value still remaining in the timer (and clear potentially stale comparison event)
    // Start the timer peripheral (trigger start task)
    timer_cc cc_reg = CC0;
    volatile uint32_t* timer_events_compare_register = (volatile uint32_t *)TIMER_EVENTS_COMPARE_ADDR(TIMER0_BASE_ADDR, cc_reg);
    volatile uint32_t* timer_tasks_clear_register = (volatile uint32_t *)TIMER_TASKS_CLEAR_ADDR(TIMER0_BASE_ADDR);
    volatile uint32_t* timer_tasks_start_register = (volatile uint32_t *)TIMER_TASKS_START_ADDR(TIMER0_BASE_ADDR);
    timer0_num_interrupts_already_handled = 0;
    *timer_events_compare_register = NotGenerated;
    *timer_tasks_clear_register = TRIGGER;
    *timer_tasks_start_register = TRIGGER;
}

/**
 * Trigger timer to stop incrementing its own internal counter (but keeps the same configurations specified in timer_init).
 * This can be used in default mode (without calling init) but behavior is undefined.
 */
void timer0_stop() {
    // Stop the timer peripheral (trigger stop task)
    volatile uint32_t* timer_tasks_stop_register = (volatile uint32_t *)TIMER_TASKS_STOP_ADDR(TIMER0_BASE_ADDR);
    *timer_tasks_stop_register = TRIGGER;
}

/**
 * Custom handler for the TIMER0 peripheral that moves the stepper motor at the desired frequency.
 * At each interrupt, the next control sequence of the stepper motor will be called.
 * The timer itself is deactivated after the number of steps specified by the call to stepper_move is satisfied.
 * Expected to manually clear the timer so that the timer will not count until wrap-around (i.e. once the single comparison value for the sampling frequency is reached then the timer is reset).
 * Assumes that the only interrupts being generated from TIMER0 originate from the CC0 register being found as equal to the timer.
 */
void TIMER0_Handler() {
    // Clear compare register event and clear timer
    *(volatile uint32_t *)TIMER_TASKS_CLEAR_ADDR(TIMER0_BASE_ADDR) = TRIGGER;
    *(volatile uint32_t *)TIMER_EVENTS_COMPARE_ADDR(TIMER0_BASE_ADDR, CC0) = NotGenerated;

    // Check if the correct number of interrupts have been handled
    // Manually stop the timer and return if no more actions are needed
    if (timer0_num_interrupts_after_start == timer0_num_interrupts_already_handled) {
        timer0_stop();
        return;
    }

    // Advance stepper motor control sequence
    stepper_advance_step();
    timer0_num_interrupts_already_handled++;
}

/**
 * Configures the TIMER1 peripheral to be prepared to start counting at 1 MHz.
 * Only generates an interrupt if the maximum value is reached (stored in CC[0]).
 * If GPIOTE has falling edge before maximum value, this value is placed in CC[1]
 */
void timer1_init() {
    // Set timer precaler to 16 so timer counts cleanly at 1 MHz
    // Prescaler value is used as exponent for 2 so 2^4 = 16 -> 16 MHz / 16 = 1 MHz
    uint8_t prescaler = 4;
    volatile uint32_t* timer_prescaler_register = (volatile uint32_t *)TIMER_PRESCALER_ADDR(TIMER1_BASE_ADDR);
    *timer_prescaler_register = prescaler;

    // Place maximum value in CC0 (do not touch this after initialization)
    timer_cc cc_reg = CC0;
    uint16_t comparison_value = ULTRASONIC_TIMEOUT_US;
    volatile uint32_t* timer_cc_register = (volatile uint32_t *)TIMER_CC_ADDR(TIMER1_BASE_ADDR, cc_reg);
    *timer_cc_register = comparison_value;

    // Enable TIMER1 to generate an interrupt in the NVIC
    volatile uint32_t* timer_intenset_register = (volatile uint32_t *)TIMER_INTENSET_ADDR(TIMER1_BASE_ADDR);
    *timer_intenset_register |= (1 << (TIMER_INTENSET_INDEX_OFFSET + cc_reg)); // Enable the COMPARE[0] event to generate interrupts
    volatile uint32_t* nvic_iser0_register = (volatile uint32_t *)NVIC_ISER0_ADDR;
    *nvic_iser0_register |= (1 << TIMER1_IRQ);
}

/**
 * Triggers timer1 to begin counting upwards until max value is hit or GPIOTE handler is invoked.
 */
void timer1_start() {
    // Clear any stale value still remaining in the timer (and clear potentially stale comparison event)
    // Start the timer peripheral (trigger start task)
    timer_cc cc_reg = CC0;
    volatile uint32_t* timer_events_compare_register = (volatile uint32_t *)TIMER_EVENTS_COMPARE_ADDR(TIMER1_BASE_ADDR, cc_reg);
    volatile uint32_t* timer_tasks_clear_register = (volatile uint32_t *)TIMER_TASKS_CLEAR_ADDR(TIMER1_BASE_ADDR);
    volatile uint32_t* timer_tasks_start_register = (volatile uint32_t *)TIMER_TASKS_START_ADDR(TIMER1_BASE_ADDR);
    *timer_events_compare_register = NotGenerated;
    *timer_tasks_clear_register = TRIGGER;
    *timer_tasks_start_register = TRIGGER;
}

/**
 * Trigger timer1 to stop incrementing its own internal counter (but keeps the same configurations specified in timer_init).
 */
void timer1_stop() {
    // Stop the timer peripheral (trigger stop task)
    volatile uint32_t* timer_tasks_stop_register = (volatile uint32_t *)TIMER_TASKS_STOP_ADDR(TIMER1_BASE_ADDR);
    *timer_tasks_stop_register = TRIGGER;
}

/**
 * Custom handler for the TIMER1 peripheral that simply sets the ultrasonic measurement to the maximum possible value
 * If the timeout value is reached, the timer is stopped.
 * Assumes that the timeout value is always in CC0 (so this should not be overwritten when capturing a valid timer value).
 */
void TIMER1_Handler() {
    // Clear compare register event and clear timer
    // This interrupt should only fire if the timeout value was reached so just assume this is the case
    *(volatile uint32_t *)TIMER_TASKS_CLEAR_ADDR(TIMER1_BASE_ADDR) = TRIGGER;
    *(volatile uint32_t *)TIMER_EVENTS_COMPARE_ADDR(TIMER1_BASE_ADDR, CC0) = NotGenerated;
    timer1_stop();
    last_ultrasonic_measurement = 0xFFFFFFFF;
}