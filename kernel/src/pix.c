/** @file   pix.c
 *  @brief  Implementation of pix specific details for use in PWM.
**/

#include "pix.h"
#include "pwm.h"
#include "printk.h"

/**
 * This array will be of length PIX_ENCODE_LENGTH+PIX_RESET_DELAY (i.e. it will have a number of cycles equal to the needed number of color codes)
 */
static uint16_t pix_duty_cycles[PIX_ENCODE_LENGTH+PIX_RESET_DELAY] = { 0 };

/**
 * Call configuration values for the PWM peripheral and place PIX constants into the correct locations.
 * Enable the peripheral after configuring channel, sequence, and global values.
 */
void pix_init() {
    // Call configuration functions for PWM peripheral
    int rv_channel = pwm_channel_init(Channel0, PIX_PORT, PIX_PIN);
    int rv_sequence = pwm_sequence_init(Sequence0, pix_duty_cycles, PIX_ENCODE_LENGTH+PIX_RESET_DELAY, 0, 0);
    int rv_global = pwm_global_init(DIV_1, Up, PIX_COUNTERTOP); // DIV_1 and Up counting are defaults

    // Check if any initialization failed and print 
    if (rv_channel || rv_sequence || rv_global) {
        printk("PIX PWM initialization failed: Channel %d, Sequence %d, Global, %d\n", rv_channel, rv_sequence, rv_global);    
    }

    // Zero the neopixels initially
    for (uint32_t pix_index = 0; pix_index < PIX_ENCODE_LENGTH; pix_index++) {
        pix_duty_cycles[pix_index] = PIX_LOW_ENCODING;
    }

    // Pass in a manual duty cycle delay for the reset value (going to 80us instead of using end delay)
    // This should never change between pix_color_set calls (this is just for safety to ensure reset value is present)
    for (uint32_t reset_index = PIX_ENCODE_LENGTH; reset_index < (PIX_ENCODE_LENGTH+PIX_RESET_DELAY); reset_index++) {
        pix_duty_cycles[reset_index] = PIX_COUNTERTOP+1; // Hold the value low for a value greater than countertop so that the rising edge value would never transition to 1
    }
}

/**
 * Determines which pix in the chained neopixels should have its values changed (only touching the neopixel sitting at the 24-bit grouping at `pix_index`).
 * Places duty cycle constants to encode logical high/low in the static `pix_duty_cycles` array (overwritten at pix_index location for each call to this function).
 * Does not affect the duty cycles for pins other than the specified `pix_index`.
 */
void pix_color_set(uint8_t r, uint8_t g, uint8_t b, uint32_t pix_index) {
    // Check if index if out of range
    if (pix_index >= PIX_NUM) {
        return;
    }

    // Figure out where the 24-bit grouping belonging to pin_index resides (this will be the offset into pix_duty_cycles)
    uint32_t pix_offset = 24*pix_index;

    // Write green values to static array (from index 0 to index 7 offset by the specified pin_num)
    for (uint32_t green_index = 0; green_index < 8; green_index++) {
        // Check if 1 in this index position and if so write the high PWM encoding (otherwise write the low PWM encoding)
        if (g & (1 << (7-green_index))) {
            pix_duty_cycles[pix_offset+green_index] = PIX_HIGH_ENCODING;
        } else {
            pix_duty_cycles[pix_offset+green_index] = PIX_LOW_ENCODING;
        }
    }

    // Write red values to static array (from index 8 to index 15 offset by the specified pin_num)
    for (uint32_t red_index = 8; red_index < 16; red_index++) {
        // Check if 1 in this index position and if so write the high PWM encoding (otherwise write the low PWM encoding)
        if (r & (1 << (15-red_index))) {
            pix_duty_cycles[pix_offset+red_index] = PIX_HIGH_ENCODING;
        } else {
            pix_duty_cycles[pix_offset+red_index] = PIX_LOW_ENCODING;
        }
    }

    // Write blue values to static array (from index 16 to index 23)
    for (uint32_t blue_index = 16; blue_index < 24; blue_index++) {
        // Check if 1 in this index position and if so write the high PWM encoding (otherwise write the low PWM encoding)
        if (b & (1 << (23-blue_index))) {
            pix_duty_cycles[pix_offset+blue_index] = PIX_HIGH_ENCODING;
        } else {
            pix_duty_cycles[pix_offset+blue_index] = PIX_LOW_ENCODING;
        }
    }
}

/**
 * Loads the specified sequence with the current values of pix_duty_cycles.
 * Any calls to pix_color_set keep the pointer in the PTR register and the length in the CNT register constant for the specified sequence.
 * Send sequence as 8-bit green, 8-bit red, then 8-bit blue with high bits sent first for each of the `PIX_NUM` neopixels.
 * This function should only be called after the sequence has been correctly tailored using pix_color_set (incomplete writes of sequence close together may result in undefined behavior). 
 */
void pix_load_sequence() {
    // Load the sequence to execute with these configured duty cycles
    pwm_load_sequence(Sequence0);
}