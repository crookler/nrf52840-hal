/** @file   pwm.h
 *  @brief  Generic function prototypes, constants, and enums for PWM MMIO addresses, possible configuration values, and status.
**/

#ifndef _PWM_H_
#define _PWM_H_

#include "gpio.h"
#include "error.h"

/// Maximum value of a 15-bit parameter (will result in an error if COUNTERTOP or COUNT is above this value)
#define MAX_15_BIT 0x00007FFF

/// Maximum value of a 24-bit parameter (will result in an error if REFRESH or ENDDELAY is above this value)
#define MAX_24_BIT 0x00FFFFFF

/**
 * The four possible channels that can be configured for each PWM instance.
 * Channels can be configured to perform interleaving reads from a single sequence string.
 * Common mode (default) passes every sequence value to every enabled channel.
 */
typedef enum {
    Channel0, ///< PSEL.OUT[0] configuration
    Channel1, ///< PSEL.OUT[1] configuration
    Channel2, ///< PSEL.OUT[2] configuration
    Channel3 ///< PSEL.OUT[3] configuration
} pwm_channel;

/**
 * The two possible sequence values that can be configured for each PWM instance.
 * Sequences can be configured and loaded independently to support complex operation.
 */
typedef enum {
    Sequence0, ///< SEQ[0] registers
    Sequence1 ///< SEQ[1] registers
} pwm_sequence;

/**
 * Specifies the counting mode for the wave counter (should be placed at `PWM_MODE_ADDR`)
 */
typedef enum {
    Up, ///< Up counter, edge-aligned PWM duty cycle
    UpAndDown ///< Up and down counter, center-aligned PWM duty cycle
} pwm_mode;

/**
 * The possible prescaler values that can be used to subdivide 16 MHz source clock.
 * Values from this enum should be placed in `PWM_PRESCALER_ADDR`
 */
typedef enum {
    DIV_1, ///< Divide by 1 (16 MHz)
    DIV_2, ///< Divide by 2 (8 MHz)
    DIV_4, ///< Divide by 4 (4 MHz)
    DIV_8, ///< Divide by 8 (2 MHz)
    DIV_16, ///< Divide by 16 (1 MHz)
    DIV_32, ///< Divide by 32 (500 kHz)
    DIV_64, ///< Divide by 64 (250 kHz)
    DIV_128 ///< Divide by 128 (125 kHz)
} pwm_prescaler;

/**
 * Contains all of the memory mapped registers necessary for completely specifying the configuration of a sequence (either sequence 1 or sequence 0).
 * Registers are in the correct order, and this struct should be instantiated at the address of the sequence pointer (i.e. PWM_SEQ_BASE_ADDR for PWM_0)
 */
typedef struct {
    volatile uint32_t PTR; ///< Sets the source pointer to use for the given sequence (sequence must be an instance of pwm_sequence).
    volatile uint32_t CNT; ///< Sets the size of the source array for the given sequence (sequence must be an instance of pwm_sequence).
    volatile uint32_t REFRESH; ///< Number of periods to burn between loading next compare value for next duty cycle (set to 0 to update every PWM period)
    volatile uint32_t END_DELAY; ///< Number of clock ticks to wait after the end of a finished sequence
} pwm_sequence_type;

/// PWM unit 0 address
#define PWM_0 0x4001C000

/// MMIO address for issuing the stop task (from a trigger)
#define PWM_TASKS_STOP_ADDR (volatile uint32_t *)(PWM_0 + 0x00000004)

/// MMIO address for issuing the sequence start task (must include sequence as an instance of pwm_sequence to determine which to begin)
#define PWM_TASKS_SEQSTART_ADDR(sequence) (volatile uint32_t *)(PWM_0 + 0x00000008 + 4*sequence)

/// MMIO address to checking if the provided sequence (pwm_sequence type) has begun
#define PWM_EVENTS_SEQSTART_ADDR(sequence) (volatile uint32_t *)(PWM_0 + 0x00000108 + 4*sequence)

/// MMIO address to checking if the provided sequence (pwm_sequence type) has finished
#define PWM_EVENTS_SEQEND_ADDR(sequence) (volatile uint32_t *)(PWM_0 + 0x00000110 + 4*sequence)

/// Enable register for the entire peripheral (in this case PWM_0)
#define PWM_ENABLE_ADDR (volatile uint32_t *)(PWM_0 + 0x00000500)

/// Register to specify counting mode for PWM (either UP and then reset to 0 or UP and DOWN)
#define PWM_MODE_ADDR (volatile uint32_t *)(PWM_0 + 0x00000504)

/// Controls the period of the PWM (value in counter is reset to 0 once equal to COUNTERTOP)
#define PWM_COUNTERTOP_ADDR (volatile uint32_t *)(PWM_0 + 0x00000508)

/// Prescaler value to subdivide 16 MHz clock source
#define PWM_PRESCALER_ADDR (volatile uint32_t *)(PWM_0 + 0x0000050C)

/// Sets the source pointer to use for the given sequence (sequence must be an instance of pwm_sequence).
#define PWM_SEQ_BASE_ADDR(sequence) (pwm_sequence_type *)(PWM_0 + 0x00000520 + 32*sequence)

/// Ties a provided output channel to a specific pin (channel must be an instance of pwm_channel)
#define PWM_PSEL_OUT_ADDR(channel) (volatile uint32_t *)(PWM_0 + 0x00000560 + 4*channel)

/**
 * Helper macro for placing the provided pin and port in the correct location of the PSEL registers of a given channel and PWM instance
 * Pin must be an instance of the `gpio_pin` enum and port must be an instance of the `gpio_port` enum.
 * Also labels the pin as connected using a 0 shifted into the 31st position.
 */
#define PWM_PIN_ASSIGNMENT(pin, port) ((0 << 31) | (port << 5) | (pin << 0))

/**
 * Stub for initializing the PWM with sequence parameters and clock confgiruations.
 * These are parameters that are shared by all channels and sequences on the peripheral
 * Scale is the division of the 16 MHz clock to apply, countertop is the number of (potentially divided) clock cycles to reset at once reached, and mode is the direction that the clock counts.
 * Assumes that PWM_0 instance is being configured and enables this instance upon success.
 * Will return 0 on success or a negative error code if parameters are invalid.
 */
int pwm_global_init(pwm_prescaler scale, pwm_mode mode, uint16_t countertop);

/**
 * Stub for tying the selected sequeunce to the provided sequence configurations.
 * These are parameters that are unique to the selected sequence.
 * The `duty_cycles` pointer is the start of an array with `sequence_length` size with 16-bit half-words detailing the compare value for each PWM cycle.
 * The `refresh` parameter is the number of PWM periods to delay between next sample is loaded, and `end_delay` parameter is the number of PWM periods to hold the input constant after the passed in sequence has completed.
 * Assumes that PWM_0 instance is being configured.
 * Will return 0 on success or a negative error code if parameters are invalid.
 */
int pwm_sequence_init(pwm_sequence sequence, uint16_t *duty_cycles, uint16_t sequence_length,  uint32_t refresh, uint32_t end_delay);

/**
 * Stub for tying the selected channel to the provided GPIO pin.
 * These are parameters that are unique to the provided channel.
 * Assumes that PWM_0 instance is being configured.
 * Will return 0 on success or a negative error code if parameters are invalid.
 */
int pwm_channel_init( pwm_channel channel, gpio_port port, uint8_t pin);

/**
 * Loads specified sequence for exactly one PWM period and does not repeat.
 * Loaded in default common mode, so all enabled PWM channels receive same duty cycles.
 * Assumes that calls have been made to `pwm_global_init`, `pwm_sequence_init`, and `pwm_channel_init` before attempting to load.
 * Blocks until the sequence has completed (and clears the associated event).
 */
void pwm_load_sequence(pwm_sequence sequence);

#endif