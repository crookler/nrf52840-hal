/** @file   timer.h
 *  @brief  Contains constants and structs used to configure the TIMER0 peripheral
**/

#ifndef _TIMER_H_
#define _TIMER_H_

#include "arm.h"
#include "events.h"
#include "nvic.h"

/// Specifies the number of interrupts that should be handled by the TIMER0 handler after a start task is issued
extern volatile uint32_t timer0_num_interrupts_after_start;

/// Specifies the number of interrupts that have already been handled since the last start task was issued for TIMER0
extern volatile uint32_t timer0_num_interrupts_already_handled;

/// Base address for the TIMER0 instance of the timer peripheral (currently used by stepper motor)
#define TIMER0_BASE_ADDR 0x40008000

/// Interrupt request number in vector table for TIMER0 (will be pended from TIMER0 when timer matches the value in a capture compare register)
#define TIMER0_IRQ (8)

/// Base address for the TIMER1 instance of the timer peripheral (currently used by ultrasonic sensor)
#define TIMER1_BASE_ADDR 0x40009000

/// Interrupt request number in vector table for TIMER1 (will be pended from TIMER1 when timeout value is found)
#define TIMER1_IRQ (9)

/// The base frequency of each timer peripheral is 16 MHz (which can be subdivided by setting the prescaler register)
#define TIMER_BASE_FREQUENCY (16000000)

/**
 * Capture compare registers available to each instance of the timer peripheral.
 * TIMER3 and TIMER4 have two addition CC registers but all instances have at least these 4.
 * Will be used as arguement type when indexing into groups of CC registers (using macros).
 */
typedef enum {
    CC0, ///< Capture compare register 0
    CC1, ///< Capture compare register 1
    CC2, ///< Capture compare register 2
    CC3 ///< Capture compare register 3
} timer_cc;

/// MMIO address for issuing the start task (starting timer countdown)
#define TIMER_TASKS_START_ADDR(timer_addr) (timer_addr + 0x000)

/// MMIO address for issuing the stop task (stopping timer countdown)
#define TIMER_TASKS_STOP_ADDR(timer_addr) (timer_addr + 0x004)

/// MMIO address for issuing the clear task (clearing current timer value - resetting back to 0)
#define TIMER_TASKS_CLEAR_ADDR(timer_addr) (timer_addr + 0x00C)

/// MMIO address for issuing capture task (loading current value of timer at instant of trigger into cc register)
#define TIMER_TASKS_CAPTURE_ADDR(timer_addr, cc) (timer_addr + 0x040 + 4*cc)

/// Event register for when the current timer value matches the value in CC[i] (arguement cc must be of type timer_cc)
#define TIMER_EVENTS_COMPARE_ADDR(timer_addr, cc) (timer_addr + 0x140 + 4*cc)

/// Allows enabling the generation of interrupts for when values in timer and CC[i] are equal
#define TIMER_INTENSET_ADDR(timer_addr) (timer_addr + 0x304)

/// The number of open right-aligned indices in the intenset register before bits start to enable interrupts
#define TIMER_INTENSET_INDEX_OFFSET (16)

/// Set the prescalar for the 16 MHz to be used in the timer (valid values are 0-9 with the timer being divided by 2^prescaler)
#define TIMER_PRESCALER_ADDR(timer_addr) (timer_addr + 0x510)

/// Capture compare register MMIO address where compare values are loaded (must be of type timer_cc - only the numbers of bits in the BITMODE register will be used with a default of 16)
#define TIMER_CC_ADDR(timer_addr, cc) (timer_addr + 0x540 + 4*cc)


/**
 * Configures the used TIMER0 peripheral with appropriate prescaling values and prepares it to start generating interrupts for the stepper control signals.
 * CC[0] is used to hold the comparison value associated with the `freq` of event occurence.
 * When the timer value is equal to the comparison value, a TIMER0 interrupt is configured to be generated (with the NVIC being configured to listen for it).
 */
void timer0_init(int freq);

/**
 * Starts the timer to begin counting and triggering interrupts according to the `freq` last specified in an initialization call.
 * The timer will continue until a call to timer_stop is made.
 */
void timer0_start();

/**
 * Stops the timer from increasing its internal counter but does not reset the configuration values specified in the last call to `timer_init`.
 */
void timer0_stop();

/**
 * Configures the TIMER1 peripheral for use in tracking the delay between a source ultrasonic pulse and the detection of that pulse on return. 
 * This timer will begin counting until 36ms timeout point is reached (after which no object is assumed to be detected).
 * The timer will simply count until a detection is made by the GPIOTE channel (after which the current value of the timer will be captured and assessed for time elapsed).
 */
void timer1_init();

/**
 * Invoke TIMER1 peripheral to begin counting up.
 */
void timer1_start();

/**
 * Immediately stop TIMER1 from counting futher.
 */
void timer1_stop();

#endif