/** @file   systick.h
 *  @brief  Address locations and types for working with the SysTick timer
**/

#ifndef _SYSTICK_H_
#define _SYSTICK_H_

#include "arm.h"

/// Counts the number of times that the current SysTick execution has reached its reset value
extern uint8_t timer_wrap_around;

/// Holds the number of times that the SysTick timer needs to wrap around before it actually handles the interrupt
extern uint8_t timer_wrap_comparison;

typedef struct {
    volatile uint32_t CSR;
    volatile uint32_t RVR;
    volatile uint32_t CVR;
} systick_t;

/**
 * Sets the SysTick clock source.
 * Used to set the CLKSOURCE field in the SysTick CSR.
 */
typedef enum {
    External, ///< Use the implementation defined external reference clock
    Processor ///< Use the internal processor clock
} systick_clksource;

/**
 * Enables or disables the generation of interrupts when counter reaches 0.
 * Used to set the TICKINT field in the SysTick CSR.
 */
typedef enum {
    NoInterrupt, ///< Do not generate interrupts when count is 0
    Exception ///< Change SysTick exception status to pending when count is 0
} systick_tickint;

/**
 * Enables or disables the SysTick counter.
 * Used to set the ENABLE field of the SysTick CSR.
 */
typedef enum {
    SystickDisabled, ///< Counter is disabled
    SystickEnabled ///< Counter is enabled
} systick_enable;

/// Base frequency of the SysTick timer (64 MHz)
#define SYSTICK_BASE_FREQUENCY (64000000)

/// Maximum value of a 24-bit parameter (will result in an error if RELOAD value is above this value)
#define MAX_24_BIT 0x00FFFFFF

/// Memory mapped address of the SYST_CSR register (first of the contiguous group so doubles as base address - intended to assign a systick_t pointer to this address)
#define SYSTICK_BASE_ADDR 0xE000E010

/// Sets the CLKSOURCE and TICKINT values of the SysTick CSR (passed in parameters must variants of clksource and tickint enums respectively)
#define SYSTICK_CSR_CONFIG_VAL(clksource, tickint) ((clksource << 2) | (tickint << 1))

/**
 * Places desired values for reload, interrupt generation, and clock source and then enables the peripheral.
 * If the peripheral was already enabled, this function 
 * Also resets the current value of the counter by writing to SYST_CVR to avoid stale values.
 */
int systick_configure(uint32_t reload, systick_clksource clksource, systick_tickint tickint);

/**
 * Disables the currently running Systick timer.
 * Useful in the case that continuing to service the systick timer interrupt would be damaging or inefficient for the program.
 */
void systick_disable();

/**
 * Performs an active wait using the 64 MHz systick clock to wait for `ms` number of milliseconds.
 * Performs individual 1 ms waits until the total number of milliseconds have been looped over.
 */
void systick_delay(uint32_t ms);

#endif

