/** @file   rtt.h
 *  @brief  Function prototpyes and struct definitions for writing/reading to the up and down buffer of the debugger.
**/

#ifndef _RTT_H_
#define _RTT_H_

#include "arm.h"

/// Max number of characters that can be placed in the rtt_up buffer at a time (i.e. from controller to host)
#define RTT_UP_BUFFER_SIZE      256

/// Max number of characters that can be placed in the rtt_down buffer at a time (i.e. from host to controller)
#define RTT_DOWN_BUFFER_SIZE    16

/**
 * Struct specififying the layout of an up buffer for the RTT protocol.
 * The controller can deterministically update the w_idx (not volatile), but the host is responsible for updating r_idx (which is not in the control of the code so volatile).
 */
typedef struct {
    const char *name; ///< Name of the up_buffer
    char *p; ///< Pointer to the character array ring buffer to write to
    uint32_t buffer_size; ///< Size of the character array ring buffer (determining wrapping)
    uint32_t w_idx; ///< Index of the next character to be overwritten by the controller
    volatile uint32_t r_idx; ///< Index of the next character to be read by the host
    uint32_t flags; ///< Specifies operating mode of protocol
} rtt_up_buffer;

/**
 * Struct mirror the layout of the rtt_up_buffer but with the volatility of r_idx and w_idx flipped.
 * r_idx is deterministically updated by the controller, but the host has the ability to update w_idx arbitrarily (hence volatile).
 */
typedef struct {
    const char *name; ///< Name of the down_buffer
    char *p; ///< Pointer to the character array ring buffer to be read from
    uint32_t buffer_size; ///< Size of the character array ring buffer (determining wrapping)
    volatile uint32_t w_idx; ///< Index of the next character to be overwritten by the host
    uint32_t r_idx; ///< Index of the next character to be read by the controller
    uint32_t flags; ///< Specifies operating mode of protocol
} rtt_down_buffer;

/**
 * Struct for specifying the layout of the allocated rtt_control_block created in linking.
 * Creates space for an up buffer, a down buffer, an identifier for finding the control block, and counts for the numbers of buffers (assumed to both be 1 in this implementation).
 */
typedef struct {
    char id[16]; ///< Identifier to find the rtt_control_block
    uint32_t num_up_buffers; ///< Default 1 (only 1 up buffer)
    uint32_t num_down_buffers; ///< Default 1 (only 1 down buffer)
    rtt_up_buffer up_buffer; ///< Instance of a a single rtt_up_buffer struct
    rtt_down_buffer down_buffer; ///< Instance of a single rtt_down_buffer struct
} rtt_control_block;

/**
 * Stub for initializing the RTT protocol.
 * Will be responsible for creating the up and down buffers and configuring all of the values within the buffer structs. 
 */
void rtt_init();

/**
 * Stub for a write from the controller to the host.
 * The controller will write exactly `len` bytes from the provided `src` array into the `p` character array of the up buffer.
 * This function will block until `len` bytes have been written from `src` to `p`.
 */
uint32_t rtt_write(const char *src, uint32_t len);

/**
 * Stub for a read by the controller of information from the host/
 * The controller will read exactly `len` bytes from the `p` character array of the down buffer into the provided `dst` array.
 * This function will block until `len` bytes have been read from `p` to `dst`.
 */
uint32_t rtt_read(char *dst, uint32_t len);

/**
 * Stub for a helper function to see the (wrapping) difference between the r_idx and the w_idx of the down buffer.
 * Does not modify the down buffer but simply says how many bytes are immediately available for consumption from the host (at the instant of invocation).
 */
uint32_t rtt_peek();

#endif