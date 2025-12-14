/** @file   i2c.h
 *  @brief  Function prototypes, constants, and enums for I2C MMIO addresses, possible configuration values, and status.
**/

#ifndef _I2C_H_
#define _I2C_H_

#include "arm.h"
#include "error.h"

/// MMIO address of the first instance of Two-Wire Interface (TWIS0)
#define I2C_BASE_ADDR 0x40003000

/// 7-bit address of the LUX sensor (least significant bit should be appended depending on read/write operation)
#define LUX_BASE_ADDRESS 0x10

/**
 * Specifies the status of the I2C peripheral
 */
typedef enum {
    I2cDisabled, ///< TWIM disabled
    I2cEnabled = 6 ///< TWIM enabled
} i2c_enable;

/// Command to begin Start RX task (read)
#define I2C_TASKS_STARTRX_ADDR (volatile uint32_t *)(0x00000000 + I2C_BASE_ADDR)

/// Command to begin Start TX task (transmit)
#define I2C_TASKS_STARTTX_ADDR (volatile uint32_t *)(0x00000008 + I2C_BASE_ADDR)

/// Command to stop ongoing tasks
#define I2C_TASKS_STOP_ADDR (volatile uint32_t *)(0x00000014 + I2C_BASE_ADDR)

/// Register holding information regarding if the I2C has been stopped
#define I2C_EVENTS_STOPPED_ADDR (volatile uint32_t *)(0x00000104 + I2C_BASE_ADDR)

/// Register holding information if the I2C has encountered an error
#define I2C_EVENTS_ERROR_ADDR (volatile uint32_t *)(0x00000124 + I2C_BASE_ADDR)

/// Event notifying that this is the last byte being read (in comparison to max configuration)
#define I2C_EVENTS_LASTRX_ADDR (volatile uint32_t *)(0x0000015C + I2C_BASE_ADDR)

/// Event notifying that this is the last byte being written (in comparison to the max configuration)
#define I2C_EVENTS_LASTTX_ADDR (volatile uint32_t *)(0x00000160 + I2C_BASE_ADDR)

/// Describes the source of an error event with 3 possible errors relating to data clobbering or receiving a NACK in response to sending an address or data byte to secondary
#define I2C_ERRORSRC_ADDR (volatile uint32_t *)(0x000004C4 + I2C_BASE_ADDR)

/// Trigger for enabling the entire TWIM peripheral
#define I2C_ENABLE_ADDR (volatile uint32_t *)(0x00000500 + I2C_BASE_ADDR)

/// Pin being tied to the clock line
#define I2C_PSEL_SCL_ADDR (volatile uint32_t *)(0x00000508 + I2C_BASE_ADDR)

/// Pin being tied to the data line
#define I2C_PSEL_SDA_ADDR (volatile uint32_t *)(0x0000050C + I2C_BASE_ADDR)

/**
 * Helper macro for placing the provided pin and port in the correct location of the PSEL.SDA and PSEL.SCL registers.
 * Pin must be an instance of the `gpio_pin` enum and port must be an instance of the `gpio_port` enum.
 * Also labels the pin as connected using a 0 shifted into the 31st position.
 */
#define I2C_PIN_ASSIGNMENT(pin, port) ((0 << 31) | (port << 5) | (pin << 0))

/// Pointer for internal buffer into which to read incoming bytes
#define I2C_RXD_PTR_ADDR (volatile uint32_t *)(0x00000534 + I2C_BASE_ADDR)

/// The maximum number of bytes that can be read during an initiated RX operation
#define I2C_RXD_MAXCNT_ADDR (volatile uint32_t *)(0x00000538+ I2C_BASE_ADDR)

/// Pointer to the internal buffer which holds bytes to be written over the data line by the leader
#define I2C_TXD_PTR_ADDR (volatile uint32_t *)(0x00000544 + I2C_BASE_ADDR)

/// The maximum number of bytes that can be written during an initiated TX operation
#define I2C_TXD_MAXCNT_ADDR (volatile uint32_t *)(0x00000548 + I2C_BASE_ADDR)

/// 7-bit follower address that the TWIM protocol should speak to (will automatically append write/read bit depending on operation)
#define I2C_ADDRESS_ADDR (volatile uint32_t *)(0x00000588 + I2C_BASE_ADDR)

/**
 * Stub for initalizing an I2C peripheral instance in leader configuration.
 * Claims ownership of the SCL and SDA pins and clears any stale events.
 */
void i2c_leader_init();

/**
 * Stub for initiating an I2C TX task.
 * Writes `tx_len` number of bytes from the `tx_buf` toward the follower with address `follower_addr`.
 * Returns a negative error code on failure or 0 on success (sucess is based on detecting the LASTTX event without encountering errors).
 */
int i2c_leader_write(uint8_t *tx_buf, uint8_t tx_len, uint8_t follower_addr);

/**
 * Stub for initiating an I2C RX task.
 * Reads `rx_len` number of bytes from the data line connected to `follower_addr` into `rx_buf`.
 * Returns a negative error code on failure or 0 on success (sucess is based on detecting the LASTRX event without encountering errors).
 */
int i2c_leader_read(uint8_t *rx_buf, uint8_t rx_len, uint8_t follower_addr);

/**
 * Triggers an unconditional STOP event when invoked.
 * Does not perform error checking nor clear any associated error registers.
 * This function is not invoked by `i2c_leader_write` or `i2c_leader_read` so must be called manually to indicate the end of an I2C transaction. 
 */
void i2c_leader_stop();

#endif