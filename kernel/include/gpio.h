/** @file   gpio.h
 *  @brief  Function prototypes, constants, and enums for GPIO MMIO addresses, possible configuration values, and status.
**/

#ifndef _GPIO_H_
#define _GPIO_H_

#include "arm.h"
#include "error.h"

/// Base address for all operations related to P0 (with pins 0-31 defined)
#define PORT_0_BASE 0x50000000

/// Base address for all operations for P1 (with pins 0-15 defined)
#define PORT_1_BASE 0x50000300

/// Determine which base address to use based on which enum variant of `gpio_port` is being used
#define PORT_BASE_ADDRESS(port) ((port == P0) ? PORT_0_BASE : PORT_1_BASE)

/// Determine if the pin (uint8_t) is valid based on the enum variant of `gpio_port` being used
#define VALID_PORT(port, pin) (((port == P0) && (pin <= 31)) || ((port == P1) && (pin <= 15)))

/// Address offset of PIN_CNF[0]. Other pins are defined as word multiples from this location
#define PIN_CNF_BASE_OFFSET 0x00000700

/// Macro for putting enum variants (`gpio_dir`, `gpio_pull`, and `gpio_drive`) respectively in the correct location in a CNF register (with bitwise OR'ing for single condition)
#define GPIO_CONFIGURATION_VALUE(direction, pull, drive) ((direction << 0) | (direction << 1) | (pull << 2) | (drive << 8))

/// OUTSET register offset (added to base depending on port being worked with)
#define OUTSET_OFFSET 0x00000508

/// OUTCLR register offset in port
#define OUTCLR_OFFSET 0x0000050C

/// IN register offset in port
#define IN_OFFSET 0x00000510

/// Return the bit at an index of 'pin' (returns either 0b0 or 0b1)
#define DIGITAL_READ_BITMASK(register_contents, pin) ((register_contents >> pin) & 1)

/**
 * Enum for the possible ports available for GPIO.
 * The nRF52840 has two GPIO ports (P0 and P1).
 */
typedef enum {
    P0, ///< GPIO Port 0 
    P1  ///< GPIO Port 1
} gpio_port;

/**
 * Enum for the whether the GPIO pin is configured as sourcing current or sinking current (output or input).
 * Input configurations will place their values in the `IN` registers and output configurations can be turned on with `OUTSET` and off with `OUTCLR`.
 */
typedef enum {
    Input, ///< Pin should be configured as input (i.e. performing digital read operations)
    Output ///< Pin should be configured as output (i.e. able to drive pin digitally high/low from code)
} gpio_dir;

/**
 * Enum for the internal pull of the GPIO pin.
 * No pull does not connect an internal resistor, pulldown connects pin to logical low value when not being driven, pullup connects pin to logical high value when not being driven
 */
typedef enum {
    Pullnone, ///< Pin does not have a pull
    Pulldown, ///< Pin pulled low by default
    Pullup = 3 ///< Pin pull high by default
} gpio_pull;

/**
 * Drive types for the two logical values of a GPIO output pin.
 * Every combination of standard drive, high drive, and disconnected is represented for digital low and digital high (except for both being disconnected)
 */
typedef enum {
    S0S1, ///< Standard digital low, standard digital high
    H0S1, ///< Strong digital low, standard digital high
    S0H1, ///< Standard digital low, strong digital high
    H0H1, ///< Strong digital low, strong digital high
    D0S1, ///< Disconnected digital low, standard digital high
    D0H1, ///< Disconnected digital low, strong digital high
    S0D1, ///< Standard digital low, disconnected digital high
    H0D1 ///< Strong digital low, disconnected digital high
} gpio_drive;

/**
 * Initialization function for a given GPIO pin.
 * Call this function with values to place in CNF register before any attempt to set output or read input.
 * Will return an error if the pin is not in the valid range for the given port.
 */
 int gpio_init(gpio_port port, uint8_t pin, gpio_dir direction, gpio_pull pull, gpio_drive drive);

/**
 * Set the output of `pin` in GPIO port `port` to high.
 * This will execute nothing if pin is invalid.
 */
void gpio_set(gpio_port port, uint8_t pin);

/**
 * Set the output of `pin` in GPIO port `port` to low.
 * This will execute nothing if pin is in an invalid range for port.
 */
void gpio_clr(gpio_port port, uint8_t pin);

/**
 * Will return the digital logical value (i.e. 0b0 or 0b1) corresponding to the value currently on `pin` in GPIO port `port`.
 * Can return an invalid port error code instead of digital logic if pin is invalid.
 */
uint8_t gpio_read(gpio_port port, uint8_t pin);

#endif
