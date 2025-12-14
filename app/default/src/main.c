/** @file   main.c
 *  @brief  main program for "default" user application
 */

#include "userutil.h"

/** @brief main function to test user space entry */
int main(UNUSED int argc, UNUSED char *argv[]) {
    while(1) {
        for(int i=1000000; i>=0; i--);
        breakpoint();
    }
    return 0;
}

