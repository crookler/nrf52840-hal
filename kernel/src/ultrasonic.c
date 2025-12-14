/** @file   ultrasonic.c
 *  @brief  Provides functionality for creating the protocol for the HC-SR04 ultrasonic range sensor.
**/

#include "ultrasonic.h"
#include "nvic.h"
#include "events.h"
#include "timer.h"

/// The value of the last measurement (range in cm) obtained from the ultrasonic sensor (max range is roughly 300 cm)
volatile uint32_t last_ultrasonic_measurement = 0;

/// Flag for saying if an event has already been serviced dealing with the start of the signal (rising edge - if so the next interrupt should take a different action)
volatile uint8_t in_measurement = 0;

/**
 * Configure the ultrasonic sensor by setting up GPIO pins.
 * Initialize GPIOTE configuration to allow reading when a measurement is complete.
 */
void ultrasonic_init() {
    // Configure trigger pin to be able to send pulses
    gpio_dir direction = Output;
    gpio_drive drive = S0S1;
    gpio_pull pull = Pullnone;
    gpio_init(ULTRASONIC_TRIGGER_PORT, ULTRASONIC_TRIGGER_PIN, direction, pull, drive);

    // Set echo pin as Pullnone, input, S0S1 configuration
    // Do not try to pull the pin since it is driven solely by the ultrasonic sensor
    direction = Input;
    drive = S0S1;
    pull = Pullnone;
    gpio_init(ULTRASONIC_OUTPUT_PORT, ULTRASONIC_OUTPUT_PIN, direction, pull, drive);

    // Initialize associated timer
    timer1_init();

    // Configure GPIOTE channel to be tied to ultrasonic sensor
    gpiote_mode mode = Event;
    gpiote_polarity polarity = Toggle; // Create events on both rising edge (start timer) and falling edge (measurement complete)

    // Assign configuration value to proper register
    volatile uint32_t* gpiote_config_register = (volatile uint32_t *)GPIOTE_CONFIG_ADDR(ULTRASONIC_GPIOTE_CHANNEL);
    *gpiote_config_register = GPIOTE_CONFIG_VALUE(mode, ULTRASONIC_OUTPUT_PIN, ULTRASONIC_OUTPUT_PORT, polarity);

    // Enable interrupts for IN[ULTRASONIC_GPIOTE_CHANNEL] (write 1 to set and write 1 to clear)
    volatile uint32_t* gpiote_intenset_register = (volatile uint32_t *)GPIOTE_INTENSET_ADDR;
    *gpiote_intenset_register |= (1 << ULTRASONIC_GPIOTE_CHANNEL); // Enable interrupts for IN[ULTRASONIC_GPIOTE_CHANNEL]

    // Enable GPIOTE interrupts on NVIC (placing 1 in index GPIOTE_IRQ of NVIC_ISER0 since this is the register responsible for the range 0-31)
    volatile uint32_t* nvic_iser0_register = (volatile uint32_t *)NVIC_ISER0_ADDR;
    *nvic_iser0_register |= (1 << GPIOTE_IRQ);

    // Reset flags and events
    last_ultrasonic_measurement = 0;
    in_measurement = 0;
    volatile uint32_t* gpiote_events_in_register = (volatile uint32_t *)GPIOTE_EVENTS_IN_ADDR(ULTRASONIC_GPIOTE_CHANNEL);
    *gpiote_events_in_register = NotGenerated;
    *(volatile uint32_t *)TIMER_TASKS_CLEAR_ADDR(TIMER1_BASE_ADDR) = TRIGGER;
    *(volatile uint32_t *)TIMER_EVENTS_COMPARE_ADDR(TIMER1_BASE_ADDR, CC0) = NotGenerated;
}

/**
 * Read the current range of the ultrasonic sensor.
 * Trigger a measurement to begin by writing very briefly (10 uS) to the trigger pin.
 * Reset last_ultrasonic_measurement global variable to a holding value and wait for GPIOTE interrupt to calculate and set the calculated range.
 * Timer interrupt may set an infinite range if the timer has already gone past when the echo pulse was expected to return.
 * Need to allow about 10 ms between calls to this function (use yield at user level if calling by threads).
 */
uint32_t ultrasonic_range() {
    // Reset measurements and flags
    gpio_clr(ULTRASONIC_TRIGGER_PORT, ULTRASONIC_TRIGGER_PIN);
    last_ultrasonic_measurement = 0;
    in_measurement = 0;

    // Flash trigger pulse (10 uS)
    gpio_set(ULTRASONIC_TRIGGER_PORT, ULTRASONIC_TRIGGER_PIN);
    COUNTDOWN(640); // At 64MHz system clock, this is 64 ticks for one microsecond (need to wait 10 uS at a minimum assuming each instruction takes one tick - branching may slightly exceed this which may be better for safety)
    gpio_clr(ULTRASONIC_TRIGGER_PORT, ULTRASONIC_TRIGGER_PIN);

    // Wait until a measurement has been determined (should be nonzero since 0.3m is the minimum reliable range of the sensor)
    while (last_ultrasonic_measurement == 0) continue;
    return last_ultrasonic_measurement;
}