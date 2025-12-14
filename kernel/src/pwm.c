/** @file   pwm.c
 *  @brief  Implementation of generic PWM configuration and sequence loading.
**/

#include "pwm.h"
#include "gpio.h"
#include "events.h"

/**
 * Configures global parameters on the PWM_0 instance.
 * Returns 0 on success or a negative error code on failure.
 */
int pwm_global_init(pwm_prescaler scale, pwm_mode mode, uint16_t countertop) {
    // Countertop is a 15 bit value (but there is no native C support for this)
    // Could mask off lowest 15 bits, but this (I feel) is better to alert of misunderstanding or error
    if (countertop > MAX_15_BIT) {
        return PWM_INVALID_ARG_RANGE_ERROR_CODE;
    }

    // Write scale, mode, and countertop values to appropriate registers
    volatile uint32_t* countertop_register = PWM_COUNTERTOP_ADDR;
    volatile uint32_t* scale_register = PWM_PRESCALER_ADDR;
    volatile uint32_t* mode_register = PWM_MODE_ADDR;
    *countertop_register = countertop;
    *scale_register = scale;
    *mode_register = mode;

    // Enable peripheral
    volatile uint32_t* enable_register = PWM_ENABLE_ADDR;
    *enable_register = TRIGGER;

    return SUCCESS;
}

/**
 * Configures sequence parameters on the PWM_0 instance.
 * Returns 0 on success or a negative error code on failure.
 */
int pwm_sequence_init(pwm_sequence sequence, uint16_t *duty_cycles, uint16_t sequence_length, uint32_t refresh, uint32_t end_delay) {
    // sequence_length is a 15-bit value and refresh and end_delay are 24-bit values (but there is no native C support for this)
    // Manually check if ranges are valid
    if ((sequence_length > MAX_15_BIT) || (refresh > MAX_24_BIT) || (end_delay > MAX_24_BIT) || (duty_cycles == NULL)) {
        return PWM_INVALID_ARG_RANGE_ERROR_CODE;
    }

    // Write pointer, array length, refresh, and end_delay values to 
    pwm_sequence_type* seq_register = PWM_SEQ_BASE_ADDR(sequence);
    seq_register->PTR = (uint32_t)duty_cycles;
    seq_register->CNT = sequence_length;
    seq_register->REFRESH = refresh;
    seq_register->END_DELAY = end_delay;
    
    // Reset any stale events relating to this sequence start and end
    volatile uint32_t* events_seqend_register = PWM_EVENTS_SEQEND_ADDR(sequence);
    volatile uint32_t* events_seqstart_register = PWM_EVENTS_SEQSTART_ADDR(sequence);
    *events_seqend_register = NotGenerated;
    *events_seqstart_register = NotGenerated;
    return SUCCESS;
}

/**
 * Configures channel parameters on the PWM_0 instance.
 * Returns 0 on success or a negative error code on failure.
 */
int pwm_channel_init(pwm_channel channel, gpio_port port, uint8_t pin) {
    // Check if GPIO initialization failed
    int rv = gpio_init(port, pin, Output, Pullnone, S0S1);
    if (rv) {
        return rv;
    }

    // Pass configured GPIO pin to peripheral in PSEL register
    volatile uint32_t* psel_out_register = PWM_PSEL_OUT_ADDR(channel);
    *psel_out_register = PWM_PIN_ASSIGNMENT(pin, port);
    return SUCCESS;
}

/**
 * Trigger the SEQSTART event for the specified sequence.
 * Immediately returns after the sequence begins
 */
void pwm_load_sequence(pwm_sequence sequence) {
    // Start sequence
    volatile uint32_t* tasks_seqstart_register = PWM_TASKS_SEQSTART_ADDR(sequence);
    *tasks_seqstart_register = TRIGGER;
}