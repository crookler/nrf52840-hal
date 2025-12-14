/** @file   launch_main.c
 *  @brief  Collect user arguements and launch user main (while handling cleanup if return occurs).
**/

#include <stdlib.h>
#include "usrarg.h"

/// Pull user defined main into scope
extern int main(int argc, char *argv[]);

/// Equivalent to exec with the user args populated in usrarg.h by gen_argv.sh
void __attribute__((noinline)) launch_main() {
    int status = main(user_argc, user_argv);
    exit(status);
}