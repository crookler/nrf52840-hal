/** @file   gpio.c
 *  @brief  Implementation of GPIO configuration and read/writing.
**/

#include "arm.h"
#include "gpio.h"

/**
 * Implementation of the gpio_init function stub in gpio.h.
 * Takes arugements for port number, pin number, input/output direction, pull/no pull, and drive type.
 * Checks if the pin number if valid for the provided port, and if valid, writes to the appropriate CNF register using memory mapped addresses.
 * Macro definitions match value definitions in CNF register, but additional shift constants are needed to move values to correct offset in CNF register.
 */
int gpio_init(gpio_port port, uint8_t pin, gpio_dir direction, gpio_pull pull, gpio_drive drive) {
    if (!VALID_PORT(port, pin)) {
        return GPIO_INVALID_PORT_ERROR_CODE; // Return error code if the pin is not valid for the given port
    }

    volatile uint32_t* cnf_register = (volatile uint32_t *)(PORT_BASE_ADDRESS(port) + PIN_CNF_BASE_OFFSET + 4*pin); // Select correct cnf register as base offset + 0x700 + 4*pin (cnf registers are word aligned and in order)
    *cnf_register = GPIO_CONFIGURATION_VALUE(direction, pull, drive); // Shift parameters into correct place in CNF then logical OR configurations and write to address (single write)
    return SUCCESS; // Follow convention of 0 indicating normal operation (no errors)
}

/**
 * Implementation of gpio_set function stub in gpio.h.
 * Writes a single bit to OUTSET register at offset of 'pin' (does not affect status of other pins).
 * Return early if pin/port combination is invalid
 */
void gpio_set(gpio_port port, uint8_t pin) {
    if (!VALID_PORT(port, pin)) 
        return; // Return if invalid index

    volatile uint32_t* outset_register = (volatile uint32_t *)(PORT_BASE_ADDRESS(port) + OUTSET_OFFSET);
    *outset_register = 1 << pin; // Place 1 at pin index in OUTSET register
}

/**
 * Implementation of gpio_clr function stub in gpio.h.
 * Writes a single bit to OUTCLR register at offset of 'pin' (does not affect status of other pins).
 * Return earlier if pin/port combination is invalid
 */
void gpio_clr(gpio_port port, uint8_t pin) {
    if (!VALID_PORT(port, pin)) 
        return; // Return if invalid index

    volatile uint32_t* outclr_register = (volatile uint32_t *)(PORT_BASE_ADDRESS(port) + OUTCLR_OFFSET);
    *outclr_register = 1 << pin; // Place 1 at pin index in OUTCLR register
}

/** 
 * Implementation of gpio_read function stub in gpio.h.
 * Reads the single bit value at offset of 'pin' in IN register.
 * Return type is always digital high/low unless there is an error with the port/pin combination (in which case INVALID_PORT_ERROR_CODE is returned which is not a valid logic value).
 */
uint8_t gpio_read(gpio_port port, uint8_t pin) {
    if (!VALID_PORT(port, pin)) 
        return GPIO_INVALID_PORT_ERROR_CODE; // Return if invalid index

    volatile uint32_t* in_register = (volatile uint32_t *)(PORT_BASE_ADDRESS(port) + IN_OFFSET);
    uint32_t register_content = *in_register;
    return DIGITAL_READ_BITMASK(register_content, pin);
}