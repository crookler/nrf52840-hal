/** @file   pix.h
 *  @brief  Neopixel specific configuration values to use with PWM (following timings / protocol for this chip https://www.digikey.com/htmldatasheets/production/1876263/0/0/1/sk6812-datasheet.html?gad_source=1&gad_campaignid=120565755&gbraid=0AAAAADrbLlhSB6PwtzGYZOxIahRlOitZr&gclid=Cj0KCQiAosrJBhD0ARIsAHebCNrL5hliexnJs8HBbyLdp7V7NrYbmRgTa0UHymB8GQOTiCCL8xL1N9oaAlTjEALw_wcB&gclsrc=aw.ds).
**/

#ifndef _PIX_H_
#define _PIX_H_

#include "arm.h"

/// Port tied to the Neopixel in hardware (must be an instance of gpio_port)
#define PIX_PORT P0

/// Pin tied to the Neopixel in hardware (must be an instance of uint8_t)
#define PIX_PIN 6

/// Number of neopixel lights chained together at the pin/port specified above (pix protocol will automatically pass the next values on to the next light until reset signal is hit)
#define PIX_NUM 24

/// Number of duty cycles passed to the PWM peripheral for a color configuration (8 each for red, green, and blue for each of the specified pins)
#define PIX_ENCODE_LENGTH (24 * PIX_NUM)

/// Number of cycles to count to for a pix period assuming a default configuration of 16 Mhz (1.25 uS)
#define PIX_COUNTERTOP 20

/**
 * Compare value for the duty cycle of a logic high bit for the Neopixel.
 * Duty cycle values are loaded as 16-bit halfwords with 1 bit for the polarity and a 15-bit compare value.
 * A 1 MSB configures this duty cycle as falling edge (i.e. start high), and a compare value of 14 holds the clock initially high for 0.875us (within the acceptable range for 1 encoding assuming clock speed of 16 MHz)
 * */
#define PIX_HIGH_ENCODING (1 << 15) | 14

/**
 * Compare value for the duty cycle of a logic low bit for the Neopixel.
 * A 1 MSB configures this duty cycle as falling edge (i.e. start high), and a compare value of 5 holds the clock initially high for 0.3125us (within the acceptable range for 0 encoding assuming clock speed of 16 MHz)
 * */
#define PIX_LOW_ENCODING (1 << 15) | 5

/**
 * Compare value for the RESET value after the RGB values have been sent to the Neopixel.
 * This value is the number of PWM periods (not raw timer cycles) to wait after the completion of a traditional sequence (to match the 80us low reset value).
 * This will output a 64*(20*1/(16 Mhz)) = 80us delay after the end of the 24-bit normal sequence.
 */
#define PIX_RESET_DELAY 64

/**
 * Calls the appropriate configuration functions for PWM based on the constants for the Neopixel hardware setup.
 */
void pix_init();

/**
 * 0 bit is encoded as T0H = 0.3us and T0L = 0.9us.
 * 1 bit is encoded as T1H = 0.6us and T1L = 0.6us.
 * Reset code is a sequence of low voltages for more than 80us after all 24-bit sequences have been sent to all Neopixels in series (hold low after every single 24 bit sequence if just driving one Neopixel).
 * Period will be equal to TH + TL = 1.25us (from datasheet) so configure PWM to have this period and an end delay equal to reset value (since sequence will always end low when using falling edge).
 * `pin_index` indicates which pin in the sequence 0:PIX_NUM-1 (starting with 0) should be assigned with these values (when driving one light always specify pix_index as 0).
 */
void pix_color_set(uint8_t r, uint8_t g, uint8_t b, uint32_t pix_index);

/**
 * Display the current values for each of the attached neopixels.
 * Will display the held colors for all neopixels (even those not overwritten), so this should only be called after calls to pix_color_set have clarified the wanted colors for all neopixels
 */
void pix_load_sequence();

#endif