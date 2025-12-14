/** @file   i2c.c
 *  @brief  Implementation of I2C peripheral configuration and independent calls to init, write, or read.
**/

#include "i2c.h"
#include "events.h"
#include "gpio.h"
#include "arm.h"

/**
 * Initializes the TWIM0 peripheral with the provided SCL and SDA pins. 
 * Configures both pins with native Pullup resistors and standard 0 disconnected 1 according to specification.
 * Leaves frequency at default 250 KHz to avoid driving the lux sensor quickly (under the 400 KHz that the sensor supports).
 * All events monitored by by calls to i2c_leader_write, i2c_leader_write, and i2c_leader_stop are reset to avoid stale values.
 */
void i2c_leader_init() {
    // Enable GPIO pins with pull up resistors (active low - natively high by default) to match the convention of passive 1's and active 0's.
    // Both pins initialized with S0D1 and internal Pullup resistors.
    gpio_port scl_port = P0;
    gpio_port sda_port = P0;
    uint8_t scl_pin = 11;
    uint8_t sda_pin = 12;

    gpio_init(scl_port, scl_pin, Input, Pullup, S0D1); // SCL (P0.11)
    gpio_init(sda_port, sda_pin, Input, Pullup, S0D1); // SDA (P0.12)

    // Map configured pins to correct locations in I2C peripheral
    volatile uint32_t* psel_scl_register = I2C_PSEL_SCL_ADDR;
    volatile uint32_t* psel_sda_register = I2C_PSEL_SDA_ADDR;
    *psel_scl_register = I2C_PIN_ASSIGNMENT(scl_pin, scl_port);
    *psel_sda_register = I2C_PIN_ASSIGNMENT(sda_pin, sda_port);

    // Enable the peripheral
    volatile uint32_t* enable_register = I2C_ENABLE_ADDR;
    *enable_register = I2cEnabled;

    // Clear any stale event flags so they can reliably be used later
    volatile uint32_t* events_lasttx_register = I2C_EVENTS_LASTTX_ADDR;
    volatile uint32_t* events_lastrx_register = I2C_EVENTS_LASTRX_ADDR;
    volatile uint32_t* events_stopped_register = I2C_EVENTS_STOPPED_ADDR;
    volatile uint32_t* events_error_register = I2C_EVENTS_ERROR_ADDR;
    volatile uint32_t* errorsrc_register = I2C_ERRORSRC_ADDR;
    *events_lasttx_register = NotGenerated;
    *events_lastrx_register = NotGenerated;
    *events_stopped_register = NotGenerated;
    *events_error_register = NotGenerated;
    *errorsrc_register = 0b111; // Clear error flags (writing 1 to clear from spec)
}

/**
 * Places `tx_buf` into the TXD.PTR register to identify source for data bytes being written and places `tx_len` as length of `tx_buf` into TXD.MAXCNT register.
 * Configures the initiated write to target with address `follower_addr`.
 * Triggers an I2C_STARTTX task and returns with a negative error code in the event of error or a 0 in the event of the LASTTX event being fired.
 * The specification indicates that the STOP task should be triggered once the LASTTX event is received, so this function returns and is expected to either immediately call `i2c_leader_stop` or trigger a restart.
 */
int i2c_leader_write(uint8_t *tx_buf, uint8_t tx_len, uint8_t follower_addr) {
    // Return early if the transmit buffer pointer is invalid
    if (tx_buf == NULL) {
        return I2C_INVALID_BUFFER_ERROR_CODE;
    }

    // Place follower address into ADDRESS register
    // This should be the 7 bit address (will have an additional 0 appended to end to signify a write operation)
    volatile uint32_t* address_register = I2C_ADDRESS_ADDR;
    *address_register = follower_addr;

    // Configure TXD.PTR and MAXCNT to provide the location and amount of the data to be sent once secondary device has acknowledged address (and write bit)
    volatile uint32_t* txd_ptr_register = I2C_TXD_PTR_ADDR;
    volatile uint32_t* txd_maxcnt_register = I2C_TXD_MAXCNT_ADDR;
    *txd_ptr_register = (uint32_t)tx_buf;
    *txd_maxcnt_register = tx_len;

    // Start by triggering STARTTX task
    volatile uint32_t* tasks_starttx_register = I2C_TASKS_STARTTX_ADDR;
    *tasks_starttx_register = TRIGGER;

    // Monitor for errors and last_tx event 
    // Assume that starting to transmit last byte is indicative of success condition
    volatile uint32_t* events_error_register = I2C_EVENTS_ERROR_ADDR;
    volatile uint32_t* errorsrc_register = I2C_ERRORSRC_ADDR;
    volatile uint32_t* events_lasttx_register = I2C_EVENTS_LASTTX_ADDR;
    while (1) {
        if (*events_error_register) {
            // Non zero implies there is an error code signifying either a new byte has overwritten data not yet transferred to read buffer or that the address byte or a data byte written from the leader was not acknowledged by a follower
            // Return the error code for the associated error in ERRORSRC (2^0 is overrun error, 2^1 is address nack, and 2^2 is data nack)
            int error_code = *errorsrc_register;
            *events_error_register = NotGenerated;
            *errorsrc_register = 0b111; // Clear error flags (writing 1 to clear from spec)
            switch (error_code) {
                case 1:
                    return I2C_OVERRUN_ERROR_CODE;
                case 2:
                    return I2C_ADDRESS_NACK_ERROR_CODE;
                case 4:
                    return I2C_DATA_NACK_ERROR_CODE;
            }
        } else if (*events_lasttx_register) {
            // Last byte to be transmitted is beginning so return (stop should be called after this)
            *events_lasttx_register = NotGenerated;
            return SUCCESS;
        }
    }
}

/**
 * Places `rx_buf` into the RXD.PTR register to identify destination for incoming bytes (with `rx_len` as length of `rx_buf` into RXD.MAXCNT register).
 * Configures the initiated read to listen to address `follower_addr`.
 * Triggers an I2C_STARTRX task and returns with a negative error code in the event of error or a 0 in the event of the LASTRX event being fired.
 * The specification indicates that the STOP task should be triggered once the LASTRX event is received, so this function returns and is expected to either immediately call `i2c_leader_stop` or trigger a restart.
 */
int i2c_leader_read(uint8_t *rx_buf, uint8_t rx_len, uint8_t follower_addr) {
    // Return early if the transmit buffer pointer is invalid
    if (rx_buf == NULL) {
        return I2C_INVALID_BUFFER_ERROR_CODE;
    }

    // Place follower address into ADDRESS register
    // This should be the 7 bit address (will have an additional 1 appended to end to signify a read operation)
    volatile uint32_t* address_register = I2C_ADDRESS_ADDR;
    *address_register = follower_addr;

    // Configure RXD.PTR and MAXCNT to provide the destimation and amount data to be read once secondary device has acknowledged address (and read bit)
    volatile uint32_t* rxd_ptr_register = I2C_RXD_PTR_ADDR;
    volatile uint32_t* rxd_maxcnt_register = I2C_RXD_MAXCNT_ADDR;
    *rxd_ptr_register = (uint32_t)rx_buf;
    *rxd_maxcnt_register = rx_len;

    // Start by triggering STARTRX task
    volatile uint32_t* tasks_startrx_register = I2C_TASKS_STARTRX_ADDR;
    *tasks_startrx_register = TRIGGER;

    // Monitor for errors and last_rx event 
    // Assume that starting to read last byte is indicative of success condition
    volatile uint32_t* events_error_register = I2C_EVENTS_ERROR_ADDR;
    volatile uint32_t* errorsrc_register = I2C_ERRORSRC_ADDR;
    volatile uint32_t* events_lastrx_register = I2C_EVENTS_LASTRX_ADDR;
    while (1) {
        if (*events_error_register) {
            // Non zero implies there is an error code signifying either a new byte has overwritten data not yet transferred to read buffer or that the address byte or a data byte written from the leader was not acknowledged by a follower
            // Return the error code for the associated error in ERRORSRC (2^0 is overrun error, 2^1 is address nack, and 2^2 is data nack)
            int error_code = *errorsrc_register;
            *events_error_register = NotGenerated;
            *errorsrc_register = 0b111; // Clear error flags (writing 1 to clear from spec)
            switch (error_code) {
                case 1:
                    return I2C_OVERRUN_ERROR_CODE;
                case 2:
                    return I2C_ADDRESS_NACK_ERROR_CODE;
                case 4:
                    return I2C_DATA_NACK_ERROR_CODE;
            }
        } else if (*events_lastrx_register) {
            // Last byte to be read is beginning so return (stop should be called after this)
            *events_lastrx_register = NotGenerated;
            return SUCCESS;
        }
    }
}

/**
 * Triggers a STOP task for the leader and blocks until the stop is successful.
 * This function will always trigger a stop command when invoked and does not perform additional error checking on any bytes read/written since it was invoked.
 * It may be necessary to check for one last error condition since both the read and write functions returned when the last byte was being written/read.
 */
void i2c_leader_stop() {
    volatile uint32_t* tasks_stop_register = I2C_TASKS_STOP_ADDR;
    volatile uint32_t* events_stopped_register = I2C_EVENTS_STOPPED_ADDR;
    *tasks_stop_register = TRIGGER;
    
    // Wait for stop event before returning
    BUSY_LOOP(*events_stopped_register == NotGenerated);
    *events_stopped_register = NotGenerated;
}
