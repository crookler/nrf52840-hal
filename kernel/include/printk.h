/** @file   printk.h
 *  @brief  Prototypes for printk (print to debugger as a wrapper of `rtt.h`)
 */ 

#ifndef _PRINTK_H_
#define _PRINTK_H_

#include "rtt.h"

/**
 * Stub for printing a formatting string (with escape sequences).
 */
int printk(const char *fs, ...);

#endif

