/** @file   events.h
 *  @brief  Common event paradigm used by multiple peripherals including constant to indicate starting a task and enum showing event status.
**/

#ifndef _EVENTS_H_
#define _EVENTS_H_

/// Define constant to show this is a task that is being triggered (for programmer's perspective)
#define TRIGGER 1

/**
 * Generic peripheral event.
 * Can be used in any peripheral that relays interrupts/signals to the controller.
 */
typedef enum {
    NotGenerated, ///< Event not yet fired
    Generated, ///< Event fired
} event;

#endif