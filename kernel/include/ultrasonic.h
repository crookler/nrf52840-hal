/** @file   ultrasonic.h
 *  @brief  Function stubs and structs for reading ultrasonic sensor ranges (following this datasheet https://www.handsontec.com/dataspecs/HC-SR04-Ultrasonic.pdf).
**/

#ifndef _ULTRASONIC_H_
#define _ULTRASONIC_H_

#include "arm.h"
#include "gpiote.h"

/// The value of the last measurement (range in cm) obtained from the ultrasonic sensor (max range is roughly 300 cm)
extern volatile uint32_t last_ultrasonic_measurement;

/// Flag for saying if an event has already been serviced dealing with the start of the signal (rising edge - if so the next interrupt should take a different action)
extern volatile uint8_t in_measurement;

/// Amount of time (in uS) until the ultrasonic sensor measurement is said to be invalid
#define ULTRASONIC_TIMEOUT_US 36000

/// GPIO port for the ultrasonic trigger signal
#define ULTRASONIC_TRIGGER_PORT P0

/// GPIO port for the ultrasonic trigger signal
#define ULTRASONIC_TRIGGER_PIN 8

/// GPIO port for the ultrasonic sensor output
#define ULTRASONIC_OUTPUT_PORT P1

/// GPIO port for the ultrasonic sensor output
#define ULTRASONIC_OUTPUT_PIN 9

/// The GPIOTE channel associated with tracking events on the reset button
#define ULTRASONIC_GPIOTE_CHANNEL Channel1

/**
 * Intialize the ultrasonic sensor including linking appropriate GPIO pins and creating GPIOTE interrupts.
 */
void ultrasonic_init();

/**
 * Return a single measurement from the ultrasonic sensor of the current range in cm.
 * This implementation is blocking to allow user threads to accurately gauge execution time of interrupts.
 */
uint32_t ultrasonic_range();

#endif