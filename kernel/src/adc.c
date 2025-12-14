/** @file   adc.c
 *  @brief  Implementation of ADC configuration and sampling.
**/

#include "adc.h"
#include "arm.h"
#include "events.h"

/**
 * Configures a single channel in Single-channel Single Conversion mode (only one channel connected).
 * Sets `samples` as the pointer to place sampled values and `num_samples` as the max number of sameples to take in continuous conversion mode.
 * MAX9814 outputs 2V (peak-to-peak) with bias of 1.25V (expecting values between 0.25V and 2.25V so adjust gain and reference accordingly to map range to 0V to VDD).
 * SAADC automatically outputs 16-bit signed output values (signed extended to 16 bits - so these are the values placed in the array of `samples`).
 */
void adc_init(int16_t *samples, uint32_t num_samples) {
    // Return early if the samples pointer is invalid
    if (samples == NULL) {
        return;
    }

    // Set values to with which to actually read the ADC (at some point would probably be good to pass these as a parameter)
    // Keeps defaults for resistor connections (positive and negative), aquisition time, and burst mode
    // Silkscreen analog pins to do not map directly to AIX
    // A4 -> AIN0
    // A5 -> AIN1
    // A0 -> AIN2
    // A1 -> AIN3
    // A3 -> AIN4
    // A5 -> AIN5
    // A2 -> AIN6
    // AREF -> AIN7
    adc_analog_input_source analog_input = AnalogInput0;
    adc_channel channel = Channel0;
    adc_gain_control gain = Gain1_4;
    adc_reference_voltage reference = Internal; // Means can measure values with Input Range = 0.6/(1/4) = 2.4 fits just outside of the max expected value from the MIC (formula from reference)
    adc_read_mode mode = Single;
    adc_resolution_bits resolution = Resolution12bit;

    // Get MMIO addresses based on the above configurations
    volatile uint32_t* positive_select_register = ADC_POSITIVE_PIN_SELECT_ADDR(channel);
    volatile uint32_t* config_register = ADC_CONFIG_ADDR(channel);
    *positive_select_register = analog_input; // Tie configured analog input to the positive input of ADC
    *config_register |= ADC_CONFIGURATION_VALUE(gain, reference, mode); // Write configuration for channel (this only sets fields that I do not want to keep at default)

    // Set MMIO configurations that are independent of analog input pin and selected channel
    *ADC_RESOLUTION_ADDR = resolution;// Number of resolution bits
    *ADC_RESULT_PTR_ADDR = (uint32_t)samples; // Assign pointer to result (just treat it as a raw uint32_t to get around casting complaining)
    *ADC_RESULTS_MAXCNT_ADDR = num_samples; // Number of samples before signaling end

    // Enable ADC interrupts for END event and notify NVIC to listen for them
    *ADC_INTENSET_ADDR |= (1 << ADC_END_EVENT_OFFSET);
     volatile uint32_t* nvic_iser0_register = (volatile uint32_t *)NVIC_ISER0_ADDR;
    *nvic_iser0_register |= (1 << ADC_IRQ);

    // Enable SAADC now that configuration is complete and clear any event flags that may be present
    // Do not need to clear the task start and sample registers
    *ADC_EVENTS_END_ADDR = NotGenerated; // Clear events
    *ADC_ENABLE_ADDR = TRIGGER;
    *ADC_TASKS_START_ADDR = TRIGGER; // Start collecting samples according to TIMER0
}

#include "i2c.h"
#include "pix.h"

/**
 * Interrupt handler for the SAADC peripheral.
 * The intention is that this interrupt handler is invoked whenever the result value from the ADC has been transferred to RAM (following an END event).
 * Then, an additional start task will be issued to wrap the `samples` configured in `adc_init` buffer back to the beginning.
 * This interrupt handler will not fire for every ADC conversion but only acts when `num_samples` samples have been collected. 
 * Once the expected number of samples have been collected, this handler will look at the array in RAM and perform FFT to get red, green, and blue colors (also setting the Neopixel color appropriately)
 */
void SAADC_Handler() {
    
}