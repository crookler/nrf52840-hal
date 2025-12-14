/** @file   reset.h
 *  @brief  Wrapper header for including very tailored functionality for allowing reset button to request a local reset.
**/

#ifndef _RESET_H_
#define _RESET_H_

#include "gpiote.h"

/// GPIO port for the reset button
#define RESET_PORT P0

/// GPIO pin for the reset button
#define RESET_PIN 18

/// The GPIOTE channel associated with tracking events on the reset button
#define RESET_GPIOTE_CHANNEL Channel0

/// Memory mapped location for the application interurupt and control register (used to set a bit that will request a reset from software)
#define AIRCR_ADDR 0xE000ED0C

/// Key value that must be written to the AIRCR register (else the write is ignored - must do this so a system reset can be requested)
#define AIRCR_VECTKEY 0x05FA

/// Index in the AIRCR register that if set to 1 will request a local reset
#define AIRCR_SYSRESETREQ_INDEX 2

/// Shift amount so that the 16-bit vectkey is in the upper halfword of the AIRCR register when writing
#define AIRCR_VECTKEY_INDEX 16

/** 
 * Memory mapped address of NVIC_ISER0 (each ISER holds a maximum of 32 bits for enabling/disabling interrupt at m+32*n where n is the register number and m is the offset).
 * GPIOTE interrupt request number is 6 (from product specification and vector table construction) so will need to set bit 6 of NVIC_ISER0.
 */
#define NVIC_ISER0_ADDR 0xE000E100

/**
 * Configures the GPIO pin connected to the reset button as input.
 * Sets a GPIOTE channel to emit events on changes to the button GPIO pin state.
 * Enables interrupts associated with the GPIOTE peripheral on the NVIC.
 */
void reset_enable();

#endif

