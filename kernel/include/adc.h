/** @file   adc.h
 *  @brief  Function prototypes, constants, and enums for ADC MMIO addresses, possible configuration values, and status.
**/

#ifndef _ADC_H_
#define _ADC_H_

#include "arm.h"
#include "nvic.h"

/**
 * Base address for the ADC peripheral.
 * Added to all offset values to get to actual MMIO addresses.
 * Should not ever need to be dereferenced explictly (hence no cast to volatile unsigned pointer).
 */ 
#define ADC_BASE_ADDR 0x40007000

/// Offset for the SAADC interrupt handler in the vector table (index for the address of the SAADC_Handler)
#define ADC_IRQ (7)

/**
 * Specifies the analog input to be associated with an ADC channel. 
 * Should be the pin connected to the microphone (unconnected on all channels by default).
 * A4 -> AIN0
 * A5 -> AIN1
 * A0 -> AIN2
 * A1 -> AIN3
 * A3 -> AIN4
 * A5 -> AIN5
 * A2 -> AIN6
 * AREF -> AIN7
 */
typedef enum {
    Unconnected, ///< Channel not connected to a pin
    AnalogInput0, ///< Pin A0
    AnalogInput1, ///< Pin A1
    AnalogInput2, ///< Pin A2
    AnalogInput3, ///< Pin A3
    AnalogInput4, ///< Pin A4
    AnalogInput5, ///< Pin A5
    AnalogInput6, ///< Pin A6
    AnalogInput7, ///< Pin A7
    VDD,///< Connected to VDD (3.3V)
    VDDHDIV5 = 0x0D ///< Connected to VDDH / 5 
} adc_analog_input_source;

/**
 * Specifies the ADC channel to associate with the analog input pin connected to the microphone. 
 * This value is used as the offset for the associated MMIO registers. 
 * Each ADC channel has 4 32-bit configuration registers, so each batch of registers is located at a multiple of 0x10.
 */
typedef enum {
    Channel0, ///< ADC Channel 0
    Channel1, ///< ADC Channel 1
    Channel2, ///< ADC Channel 2
    Channel3, ///< ADC Channel 3
    Channel4, ///< ADC Channel 4
    Channel5, ///< ADC Channel 5
    Channel6, ///< ADC Channel 6
    Channel7  ///< ADC Channel 7
} adc_channel;

/**
 * Gain configuration to be used on the input to the ADC.
 * This gain modifies the input signal before conversion begins by dividing the reference voltage by the gain value (i.e. Reference / gain_control).
 */
typedef enum {
    Gain1_6, ///< Gain = 1/6
    Gain1_5, ///< Gain = 1/5
    Gain1_4, ///< Gain = 1/4
    Gain1_3, ///< Gain = 1/3
    Gain1_2, ///< Gain = 1/2
    Gain1, ///< Gain = 1
    Gain2, ///< Gain = 2
    Gain4 ///< Gain = 4
} adc_gain_control;

/**
 * Specifies the internal reference to compare the incoming signal to.
 */
typedef enum {
    Internal, ///< Internal 0.6V reference
    VDD1_4 ///< VDD / 4 reference
} adc_reference_voltage;

/**
 * Specifies the mode that a channel should operate in.
 * Single-ended ignored PSELN and the negative input is shorted to ground (i.e. just looks at signal in isolation as a positive voltage compared to ground).
 * Differential feeds the difference between the positive and negative signals to the ADC for conversion.
 */
typedef enum {
    Single, ///< Single-ended conversion
    Differential ///< Differential conversion
} adc_read_mode;

/**
 * Specifies the number of bits to use to partition the range between V_LOW and V_HIGH.
 */
typedef enum {
    Resolution8bit, ///< 8-bit resolution
    Resolution10bit, ///< 10-bit resolution
    Resolution12bit, ///< 12-bit resolution
    Resolution14bit ///< 14-bit resolution
} adc_resolution_bits;

/**
 * Specifies the current state of the ADC.
 * The ADC can accept a new conversion request if it is ready, but will complete the current task if it is busy.
 */
typedef enum {
    Ready, ///< No on-going conversions
    Busy, ///< On-going conversion
} adc_status;

/// Macro for putting enum variants (`gain_control`, `reference_voltage`, and `read_mode`) respectively in the correct location in a CONFIG register (with bitwise OR'ing for single condition)
#define ADC_CONFIGURATION_VALUE(gain, reference, mode) ((gain << 8) | (reference << 12) | (mode << 20))

/// MMIO register for triggering ADC to start taking sample until the result buffer in RAM is full (writing `1` issues the event)
#define ADC_TASKS_START_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000000)

/// MMIO register for triggering ADC to take one sample (writing `1` issues the event)
#define ADC_TASKS_SAMPLE_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000004)

/// MMIO register for stopping ongoing conversion in ADC (writing `1` issues the event)
#define ADC_TASKS_STOP_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000008)

/// MMIO register for even saying that the ADC has configured the starting buffer and is ready for conversion
#define ADC_EVENTS_STARTED_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000100)

/// MMIO register for reading the event indicating the ADC has filled up the result buffer. `1` indicates buffer is full.
#define ADC_EVENTS_END_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000104)

/// MMIO register for reading the event indicating the conversion task has given a result and is ready for trasnfer.
#define ADC_EVENTS_DONE_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000108)

/// MMIO register for enabling the ADC to generate interrupts for specific events
#define ADC_INTENSET_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000304)

/// Offset in the INTENSET register to enable the end event interrupt generation
#define ADC_END_EVENT_OFFSET (1)

/// MMIO for determining if the ADC is ready or not (will either be in the middle of a previous conversion or idle). `0` indicates ready.
#define ADC_STATUS_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000400)

/// MMIO for enabling the entire ADC. Writing `1` enables the ADC.
#define ADC_ENABLE_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000500)

/// MMIO for selecting pin for the used channel (channel must be a variant of the `adc_channel` enum)
#define ADC_POSITIVE_PIN_SELECT_ADDR(channel) (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000510 + (0x00000010*channel))

/// MMIO for the CONFIG register of the used channel (channel must be a variant of the `adc_channel` enum)
#define ADC_CONFIG_ADDR(channel) (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000518 + (0x00000010*channel))

/// MMIO for selecting the number of bits to use
#define ADC_RESOLUTION_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x000005F0)

/// Specifies the MMIO for where to place the pointer of where completed sample output values should be written (signed 16-bit int output).
#define ADC_RESULT_PTR_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x0000062C)

/// Specifies the MMIO for the register holding the number of samples at which to emit the END event
#define ADC_RESULTS_MAXCNT_ADDR (volatile uint32_t *)(ADC_BASE_ADDR + 0x00000630)

/**
 * Stub for initializing the ADC to take a provided numbers of samples and place the result in the array at an address of `samples`.
 * Current implementations assume that num_samples is set to 1 and `samples` is a pointer to a single signed 16-bit integer.
 * If multiple samples are desired, adc_sample() should be called in a loop while still configuring the init function to only take a single sample for every trigure of adc_sample().
 */
void adc_init(int16_t *samples, uint32_t num_samples);

#endif