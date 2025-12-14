/** @file   gpiote.h
 *  @brief  Enums and memory mapped addresses for configuring GPIOTE peripheral
**/

#ifndef _GPIOTE_H_
#define _GPIOTE_H_

#include "gpio.h"

/// Base address of the GPIOTE peripheral
#define GPIOTE_BASE_ADDR 0x40006000

/**
 * Channel to configure when dealing with the sets of TASKS_OUT, TASKS_SET, TASK_CLR, EVENTS_IN, or CONFIG registers.
 * Each register definition comes in groups of 8, so the same channel should be used consistently when setting values in these registers.
 */
typedef enum {
    Channel0, ///< GPIOTE Channel 0
    Channel1, ///< GPIOTE Channel 1
    Channel2, ///< GPIOTE Channel 2
    Channel3, ///< GPIOTE Channel 3
    Channel4, ///< GPIOTE Channel 4
    Channel5, ///< GPIOTE Channel 5
    Channel6, ///< GPIOTE Channel 6
    Channel7, ///< GPIOTE Channel 7
} gpiote_channel;

/**
 * Mode of the specified channel to be set in the CONFIG register.
 * When in event mode, the channel will generate events based on values read on the pin.
 * When in task mode, the pin will be written to depending on task called and configured polarity.
 */
typedef enum {
    PinDisabled, ///< Pin specified by PSEL will not be acquired by the GPIOTE module.
    Event, ///< The pin specified by PSEL will be configured as an input and the IN[n] event will be generated if operation specified in POLARITY occurs on the pin.
    Task ///< The GPIO specified by PSEL will be configured as an output and triggering the SET[n], CLR[n] or OUT[n] task will perform the operation specified by POLARITY on the pin. When enabled as a task the GPIOTE module will acquire the pin and the pin can no longer be written as a regular output pin from the GPIO module.
} gpiote_mode;

/**
 * Polarity specified in the CONFIG register for a designated channel.
 * Will have different effects depending on the mode selected for the channel.
 */
typedef enum {
    None, ///< Task mode: No effect on pin from OUT[n] task. Event mode: no IN[n] event generated on pin activity.
    LoToHi, ///< Task mode: Set pin from OUT[n] task. Event mode: Generate IN[n] event when rising edge on pin.
    HiToLo, ///< Task mode: Clear pin from OUT[n] task. Event mode: Generate IN[n] event when falling edge on pin.
    Toggle ///< Task mode: Toggle pin from OUT[n]. Event mode: Generate IN[n] when any change on pin.
} gpiote_polarity;

/**
 * OUTINIT value for when GPIOTE channel is configured in Task mode (no effect in Event mode)
 */
typedef enum {
    Low, ///< Initial value is 0 before task is triggered
    High ///< Initial value is 1 before task is triggered
} gpiote_outinit;

/// TASKS_OUT register for specified gpiote_channel
#define GPIOTE_TASKS_OUT_ADDR(channel) (GPIOTE_BASE_ADDR + 0x000 + 4*channel)

/// TASKS_SET register for specified gpiote_channel
#define GPIOTE_TASKS_SET_ADDR(channel) (GPIOTE_BASE_ADDR + 0x030 + 4*channel)

/// TASKS_CLR register for specified gpiote_channel
#define GPIOTE_TASKS_CLR_ADDR(channel) (GPIOTE_BASE_ADDR + 0x060 + 4*channel)

/// EVENTS_IN register for specified gpiote_channel
#define GPIOTE_EVENTS_IN_ADDR(channel) (GPIOTE_BASE_ADDR + 0x100 + 4*channel)

/// Allows for enabling interrupts for values detected on EVENTS_IN
#define GPIOTE_INTENSET_ADDR (GPIOTE_BASE_ADDR + 0x304)

/// Allows disabling interrupts for associated events
#define GPIOTE_INTENCLR_ADDR (GPIOTE_BASE_ADDR + 0x308)

/// Configuration register for the specified channel (allows for specifiying as event or task mode and configuring behavior on issued task or seen event)
#define GPIOTE_CONFIG_ADDR(channel) (GPIOTE_BASE_ADDR + 0x510 + 4*channel)

/**
 * Helper macro for constructed the value for setting all bit fields in the CONFIG register (minus OUTINIT since it is not needed for event mode - can add this functionality later if needed)
 * All passed in values should be members of the respective enums gpiote_mode, gpio_port, or gpio_polarity (with pin being a valid uint8_t for the given port)
 */
 #define GPIOTE_CONFIG_VALUE(mode, pin, port, polarity) ((polarity << 16) | (port << 13) | (pin << 8) | (mode))

/// Interrupt request number in vector table for GPIOTE (will be pended from GPIOTE when GPIO pin state changes as desired)
#define GPIOTE_IRQ 6

#endif

