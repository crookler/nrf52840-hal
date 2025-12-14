/** @file   rtt.c
 *  @brief  Implementation of RTT protocol for debugger.
**/

#include "rtt.h"
#include "arm.h"

extern rtt_control_block __rtt_start;
static char up[RTT_UP_BUFFER_SIZE];
static char down[RTT_DOWN_BUFFER_SIZE];
static rtt_control_block *cb;

/**
 * Maps the allocated rtt_buffer block created during linking into the format of a rtt_control_block struct.
 * Initializes values for the read and write indices and flags of the up and down buffers.
 * Sets an identifier value so the debugger can immediately recognize this section of memory as the rtt_control_block.
 */
void rtt_init() {
    cb = &__rtt_start;
    cb->num_up_buffers = 1;
    cb->up_buffer.name = "Terminal";
    cb->up_buffer.p = up;
    cb->up_buffer.buffer_size = RTT_UP_BUFFER_SIZE;
    cb->up_buffer.w_idx = 0;
    cb->up_buffer.r_idx = 0;
    cb->up_buffer.flags = 2;

    cb->num_down_buffers = 1;
    cb->down_buffer.name = "Terminal";
    cb->down_buffer.p = down;
    cb->down_buffer.buffer_size = RTT_DOWN_BUFFER_SIZE;
    cb->down_buffer.w_idx = 0;
    cb->down_buffer.r_idx = 0;
    cb->down_buffer.flags = 2;

    cb->id[5] = '2'; cb->id[6] = 'R'; cb->id[7] = cb->id[8] = 'T'; cb->id[9] = '\0';
    cb->id[0] = cb->id[2] = 'I'; cb->id[1] = 'N'; cb->id[3] = '6'; cb->id[4] = '4';
    cb->id[10] = cb->id[11] = cb->id[12] = cb->id[13] = cb->id[14] = cb->id[15] = '\0';
    data_mem_barrier();
}

/** 
 * Populates the up_buffer by copying `len` bytes from the character array `src` (for consumption by debugger).
 * This implementation is blocking (will wait for the debugger to read all written bytes before overwriting any bytes the debugger has not yet written).
 * Returns the number of bytes written to up_buffer (should equal len).
 * Idea is to reserve w_idx = r_idx as identifier for empty and set w_idx = r_idx -1 as condition for full to avoid full/empty ambiguity issue.
**/
uint32_t rtt_write(const char *src, uint32_t len) {
    // Return 0 as safe default if src is invalid
    if (src == NULL) {
        return 0;
    }

    // Create pointer to up_buffer and get current value of write_index
    // Write index is deterministic (can just keep local copy and write to struct at end of loop)
    // r_idx is volatile and requires a fresh dereference each time
    rtt_up_buffer* up_buffer = &cb->up_buffer;
    uint32_t write_index = up_buffer->w_idx;
    const uint32_t buffer_size = up_buffer->buffer_size; // Create local copy of buffer size so dereferencing isn't needed each time (constant)
    uint32_t conflict_index; // Index to compare against r_idx to assess potential clobbering
    uint32_t src_character = 0; // Index for loop (will be returned for error checking)

    for (; src_character < len; src_character++) {
        // Set conflict index as potential index to overwrite
        // If write_index is overwritten when conflict_index is the same as r_idx, the problem of not being able to tell if full or empty occurs (since w_idx becomes r_idx)
        // Prevent write_index from becoming conflict_index until conflict_index is not r_idx (i.e. sacrifice one byte to keep distinction clear)
        // r_idx needs to be dereferenced each time since it's volatile
        conflict_index = (write_index + 1) >= buffer_size ? 0 : write_index+1; 
        BUSY_LOOP(up_buffer->r_idx == conflict_index);
        
        // Finished blocking so write src_character from src to write_index position in p
        // Update actual content in buffer before updating write index 
        up_buffer->p[write_index] = src[src_character];
        data_mem_barrier(); // Need data to be written before the upstream knows that there is a new byte (i.e. if write_index were written first then debugger may read junk)
        write_index = conflict_index; // Advance local write index

        // Write updated write_index to control block struct so debugger knows new information is available to read
        up_buffer->w_idx = write_index;
        data_mem_barrier(); // Ensure memory operations in previous loop complete before continuing to next
    }
    
    return src_character;
}

/** 
 * Populates the `dst` buffer with content from the down_buffer in the rtt_control block.
 * The debugger is responsible for writing bytes to this buffer, and any time w_idx > r_idx (wrapping), the controller can read values without fear of reading junk.
 * All data has been consumed if w_idx = r_idx, but the controller cannot read data at w_idx = r_idx because it may be junk (i.e. not yet updated since this is also the starting condition).
 * Returns the number of bytes copied from down_buffer to dst.
 **/
uint32_t rtt_read(char *dst, uint32_t len) {
    // Return 0 as safe default if dst is invalid
    if (dst == NULL) {
        return 0;
    }

    // Create pointer to down_buffer and get current value of read_index
    // Read index is deterministic (can just keep local copy and write to struct at end of loop)
    // w_idx is volatile and requires a fresh dereference each time
    rtt_down_buffer* down_buffer = &cb->down_buffer;
    uint32_t read_index = down_buffer->r_idx;
    const uint32_t buffer_size = down_buffer->buffer_size; // Create local copy of buffer size so dereferencing isn't needed each time (constant)
    uint32_t read_character = 0; // Index for loop (will be returned for error checking)

    // Copy len bytes from down_buffer to destination
    for (; read_character < len; read_character++) {
        // Wait until the write index has advanced beyond read_index (gurantees that there is content to read)
        BUSY_LOOP(down_buffer->w_idx == read_index);
        
        // Finished blocking so copy received character from down_buffer to dst
        dst[read_character] = down_buffer->p[read_index];
        data_mem_barrier(); // Make sure this read takes place before updating the value (i.e. don't report the index increase unless the value has actually been read)
        read_index = (read_index + 1) >= buffer_size ? 0 : read_index+1;  // Advance read index

        // Update read index only after data has been read
        down_buffer->r_idx = read_index;
        data_mem_barrier(); // Ensure memory operations in previous loop complete before continuing to next
    }
    
    return read_character;
}

/** 
 * Computes the wrapping difference between the write_index and the read_index in the down_buffer.
 * This difference is the number of indices that can be safely consumed without reading junk.
 * Returns the number of indices that read_index lags behind write_index.
**/
uint32_t rtt_peek() {
    rtt_down_buffer* down_buffer = &cb->down_buffer;
    uint32_t read_index = down_buffer->r_idx;
    uint32_t write_index = down_buffer->w_idx; // w_idx can update while evaluating conditions so just dereference once at start of checks (could be out of date but will not break anything)

    // Check wrapping semantics
    if (write_index >= read_index) { // No wrapping has occurred case (either empty or write_index has advanced)
        return write_index - read_index; 
    } else { // Wrapping has occurred
        return down_buffer->buffer_size - read_index + write_index;
    }
}

