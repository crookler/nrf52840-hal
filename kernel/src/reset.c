/** @file   reset.c
 *  @brief  Implementation for tying reset button to local reset and overriding weak GPIOTE interrupt handler
**/

#include "reset.h"
#include "events.h"

/**
 * Enables the reset button by configuring the reset button pin as interrupt, assigning an associated GPIOTE channel, and allowing the NVIC to recognize interrupts from the GPIOTE peripheral.
 */
void reset_enable() {
    // Set as pullup, input, S0S1 configuration
    // Reset signal is connected to ground when button is pushed (so need to hold signal high as default to detect changes)
    // This also means that an interrupt should be generated on a falling edge (i.e. when the resting 1 transitions to 0 with button press)
    gpio_dir direction = Input;
    gpio_drive drive = S0S1;
    gpio_pull pull = Pullup;
    gpio_init(RESET_PORT, RESET_PIN, direction, pull, drive);

    // Configure GPIOTE channel to be tied to reset button state change
    gpiote_mode mode = Event;
    gpiote_polarity polarity = HiToLo; // Create event on falling edge

    // Assign configuration value to proper register
    volatile uint32_t* gpiote_config_register = (volatile uint32_t *)GPIOTE_CONFIG_ADDR(RESET_GPIOTE_CHANNEL);
    *gpiote_config_register = GPIOTE_CONFIG_VALUE(mode, RESET_PIN, RESET_PORT, polarity);

    // Enable interrupts for IN[0](write 1 to set and write 1 to clear)
    volatile uint32_t* gpiote_intenset_register = (volatile uint32_t *)GPIOTE_INTENSET_ADDR;
    *gpiote_intenset_register |= (1 << RESET_GPIOTE_CHANNEL); // Enable interrupts for IN[0] at bit 0

    // Enable GPIOTE interrupts on NVIC (placing 1 in index GPIOTE_IRQ of NVIC_ISER0 since this is the register responsible for the range 0-31)
    volatile uint32_t* nvic_iser0_register = (volatile uint32_t *)NVIC_ISER0_ADDR;
    *nvic_iser0_register |= (1 << GPIOTE_IRQ);
}

