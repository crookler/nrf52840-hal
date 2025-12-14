/** @file   printk.c
 *  @brief  Implementation of debugger print functionality with helpers.
**/

#include "printk.h"

/// Underlying maximum size of the printk buffer (independent of current index)
#define PRINTK_BUFFER_SIZE  64

/**
 * Struct representing the printk buffer.
 * Includes builtin members for raw characters, the length, write index, and a return value.
 */
typedef struct {
    char *s; ///< Underlying buffer with characters
    uint32_t length; ///< Current length of the underlying buffer (no physical memory size but just valid region)
    uint32_t w_idx; ///< Next position to write character
    int rv; ///< Return value from the last print
} printk_buffer;

/**
 * Helper function for appending a given char to the provided buffer (useful when populating the print buffer by calling in a loop).
 */
static void printk_append_char(printk_buffer* f, char c) {
    uint32_t w = f->w_idx;
    if(w < f->length) {
        *(f->s + w) = c;
        w++;
        f->w_idx = w;
        f->rv++;
    }
    if(w == f->length) {
        if(rtt_write(f->s, w) != w)
            f->rv = -1;
        else
            f->w_idx = 0;
    }
}

/**
 * Helper function for appending a numberical character (in the specified base) to the end of the provided buffer.
 */
static void printk_append_unsigned(printk_buffer* f, uint32_t num, uint32_t base) {
    static const char chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    uint32_t number = num;
    uint32_t width = 1;
    uint32_t basepow = 1;
    uint32_t div;
    
    while(number >= base) {
        number = (number / base);
        width++;
        basepow *= base;
    }
    
    do {
        div = num / basepow;
        num -= div * basepow;
        printk_append_char(f, chars[div]);
        basepow /= base;
    } while(f->rv >= 0 && basepow > 0);
}

/**
 * Print a formatted string including escape sequences and format specifiers.
 */
int printk(const char *fs, ...) {
    char buffer[PRINTK_BUFFER_SIZE];
    printk_buffer f = {buffer, PRINTK_BUFFER_SIZE, 0, 0};
    char c = *fs;
    int v;

    __builtin_va_list param_list;
    __builtin_va_start(param_list, fs);

    while((c != 0) && (f.rv >= 0)) {
        fs++;
        if(c == '%') {
            c = *fs;
            if(c == 'c') {
                v = __builtin_va_arg(param_list, int);
                printk_append_char(&f, (char)v);
            } else if(c == 'd') {
                v = __builtin_va_arg(param_list, int);
                if(v < 0) {
                    v = -v;
                    printk_append_char(&f, '-');
                }
                if(f.rv >= 0)
                    printk_append_unsigned(&f, (uint32_t)v, 10);
            } else if(c == 'u') {
                v = __builtin_va_arg(param_list, int);
                printk_append_unsigned(&f, (uint32_t)v, 10);
            } else if(c == 'x' || c == 'X' || c == 'p') {
                v = __builtin_va_arg(param_list, int);
                printk_append_unsigned(&f, (uint32_t)v, 16);
            } else if(c == 's') {
                const char *str = __builtin_va_arg(param_list, const char *);
                c = *str;
                while(c != 0 && f.rv >= 0) {
                    if(c != 0)
                        printk_append_char(&f, c);
                    str++;
                    c = *str;
                }
            } else if(c == '%') {
                printk_append_char(&f, '%');
            }
            fs++;
        } else
            printk_append_char(&f, c);
        c = *fs;
    }

    if(f.rv > 0) {
        if(f.w_idx != 0)
            rtt_write(buffer, f.w_idx);
        f.rv += (int)f.w_idx;
    }
    __builtin_va_end(param_list);
    return f.rv;
}

