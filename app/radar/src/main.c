/** @file   main.c
 *  @brief  main program for "model radar" user application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "userutil.h"
#include "usyscall.h"

/// Number of LEDs composing the ring around the radar (determines fidelity of indicators)
#define NUM_RING_LEDS 24

/// Number of degrees allocated to each LED (higher number of LEDs indicates that more information can be given per degree instead of batched)
#define DEGREES_PER_LED (360 / NUM_RING_LEDS)

/// Lowest number of steps corresponding to a whole number of degrees (ratio is 256 steps to 45 degrees for the accumulator)
#define STEP_RATIO 256

/// Number of degrees corresponding to lowest whole number of steps (ratio is 256 steps to 45 degrees when steps per revolution is 2048)
#define DEGREE_RATIO 45

/// Maximum number of characters that the command buffer can store (should be well more than any needed command)
#define COMMAND_BUFFER_SIZE 64

/// Default speed of the radar revolutions in RPM
#define DEFAULT_SPEED_RPM 10

/// Maximum radar speed in RPM
#define MAXIMUM_SPEED_RPM 10

/// Minimum radar speed in RPM
#define MINIMUM_SPEED_RPM 1

/// Default range of the sensor in cm
#define DEFAULT_RANGE_CM 50

/// Maximum radar speed in RPM
#define MAXIMUM_RANGE_CM 100

/// Minimum radar speed in RPM
#define MINIMUM_RANGE_CM 30

/// The current angle (in degrees) that the stepper motor is facing (should only be updated by the stepper_thread but will be read by other threads)
static volatile unsigned int current_angle = 0;

/// The last measurement obtained from the ultrasonic sensor (updated by sensor thread but read-only in other threads)
static volatile unsigned int last_ultrasonic_measurement = 0;

/// The current range (in cm) that the radar is making detections at 
static volatile unsigned int current_range = DEFAULT_RANGE_CM;

/// Boolean flag to show if the radar is currently active or not (only set when the radar is actually spinning and taking measurements)
static volatile unsigned char radar_active = 0;

/// Boolean flag to indicate calibration mode is active
static volatile unsigned char calibration_mode = 0;

/// The index of the LED that corresponds to an angle of 0 degrees
static unsigned int zero_degrees_led = 16;

/**
 * Define a buffer to place any read characters from RTT (masked under read call at the user level).
 * Values will be accumulated in this buffer until either the size of the buffer is hit or a carriage return is seen (from user enter)
 */
static char command_buffer[COMMAND_BUFFER_SIZE];

/**
 * Next index to be overwritten in the command buffer. This will be used to reset the valid portions of the command buffer after a flush.
 * Once a previous command has been handled or the end of the buffer is reached, the command buffer is flushed by resetting this index.
 */
static size_t command_index = 0;

/**
 * Array of string representation (in lower case) of supported commands. 
 * This is treated as the ultimate source of truth when determining what the user command is.
 */
static char* supported_commands[] = {"calibrate", "start", "stop", "speed", "range", "reset", "help", "exit"};

/**
 * Guards metadata within the system that has well understood timing gurantees.
 * Only global variables that have concrete access patterns are guarded by locks to ensure synchronization.
 * Global variables that are updated sporadically / require polling rely on ownership model.
 */
typedef struct {
    lock_t* measurement_lock;
} non_polled_system_locks;

/**
 * Prints the supported commands.
 * Invoked whenever the user applications begins or the 'help' command is typed
 */
void display_commands() {
    printf("Supported commands are as follows (case insensitive):\n"
            "%s: Show 0 degrees light and spin radar dish in arc until user input confirms dish is aligned with 0 degrees\n"
            "%s: Start radar detection\n"
            "%s: Stop radar detection\n"
            "%s <rpm>: Change radar speed to the specified revolutions per minute (subject to rejection)\n"
            "%s <cm>: Change radar range to the specified centimeters (subject to rejection)\n"
            "%s: Reset range and speed to default values\n"
            "%s: Show list of commands again\n"
            "%s: Terminate the application\n\n",
            supported_commands[0], supported_commands[1], supported_commands[2], 
            supported_commands[3], supported_commands[4], supported_commands[5], 
            supported_commands[6], supported_commands[7]);
}

/**
 * Called after a carriage return character has been found in the user input.
 * Compares the lowercase version of the command to the string representations of the supported command and performs necessary actions for each command (mainly updating global variables).
 * For commands that specify a field, the command and arguement are parsed separately and then acted upon (right now only expecting a single arguement at most).
 */
void handle_user_command(char* cmd) {
    // Parse command and argument (only some commands have arguements so value may be left empty)
    char cmd_word[COMMAND_BUFFER_SIZE];
    int value;
    int num_parsed = sscanf(cmd, "%s %d", cmd_word, &value);
    
    // Compare the passed in command to the lower case string representations of the acceptable commands
    if (strcmp(cmd_word, supported_commands[0]) == 0) {
        // Calibrate
        // Enter calibration mode and stop the radar if it was previously active
        printf("Entering calibration mode - Press ENTER when aligned with 0 degree LED\n");
        calibration_mode = 1;
        radar_active = 0;

        // Turn off all LEDs except for the zero_degrees_led (could previously have been enabled by the radar so need to zero everything first)
        // Then set the zero degrees LED to yellow to alert the user this is where the stepper motor should be aligned
        for (size_t led_index = 0; led_index < NUM_RING_LEDS; led_index++) {
            neopixel_set(0, 0, 0, led_index);
        }
        neopixel_set(255, 255, 0, zero_degrees_led);
        neopixel_load();
        
        // Wait for user to press enter (can have any input followed by a carriage return)
        // Other input is not stored just simply looking for enter key
        // Block this thread until user finishes calibration
        char temp_buffer[1] = { 0 };
        while (1) {
            read(0, temp_buffer, 1); 
            if(temp_buffer[0] == '\r') {
                current_angle = 0;
                calibration_mode = 0;
                
                // Turn off calibration LED
                neopixel_set(0, 0, 0, zero_degrees_led);
                neopixel_load();
                break;
            }
        }
    } else if (strcmp(cmd_word, supported_commands[1]) == 0) {
        // Start
        radar_active = 1;
    } else if (strcmp(cmd_word, supported_commands[2]) == 0) {
        // Stop
        radar_active = 0;
    } else if (strcmp(cmd_word, supported_commands[3]) == 0) {
        // Speed <rpm>
        // See if an arguement was successfully parsed
        if (num_parsed == 2) {
            // Check if the param is within the acceptable bounds
            if (value >= MINIMUM_SPEED_RPM && value <= MAXIMUM_SPEED_RPM) {
                set_stepper_speed(value);
            } else {
                printf("Speed %d out of range [%d-%d] RPM - No update\n", value, MINIMUM_SPEED_RPM, MAXIMUM_SPEED_RPM);
            }
        } else {
            printf("Invalid param specified - Usage: speed <rpm>\n");
        } 
    } else if (strcmp(cmd_word, supported_commands[4]) == 0) {
        // Range <cm>
        // See if an arguement was successfully parsed
        if (num_parsed == 2) {
            // Check if the param is within the acceptable bounds
            if (value >= MINIMUM_RANGE_CM && value <= MAXIMUM_RANGE_CM) {
                current_range = value;
            } else {
                printf("Range %d is out of range [%d-%d] cm - No update\n", value, MINIMUM_RANGE_CM, MAXIMUM_RANGE_CM);
            }
        } else {
            printf("Invalid param specified - range <cm>\n");
        }
    } else if (strcmp(cmd_word, supported_commands[5]) == 0) {
        // Reset
        // Put range and speed back to defaults
        current_range = DEFAULT_RANGE_CM;
        set_stepper_speed(DEFAULT_SPEED_RPM);        
    } else if (strcmp(cmd_word, supported_commands[6]) == 0) {
        // Help
        display_commands();
    } else if (strcmp(cmd_word, supported_commands[7]) == 0) {
        // Exit
        // Turn off all LEDs then end the program
        for (size_t led_index = 0; led_index < NUM_RING_LEDS; led_index++) {
            neopixel_set(0, 0, 0, led_index);
        }
        neopixel_load();
        exit(0);
    } else {
        printf("Unknown command received: %s\n", cmd);
    }
}

/**
 * Process any user input until the command_buffer no longer contains a carriage return character.
 * Continuously searches the new input for a carriage return and handles a user command each time a carriage return is seen.
 * Moves any remaining characters to the beginning of the buffer after no more carriage returns are found in the processed input.
 */
void process_user_input() {
    // Read any possible bytes from stdin up until the end of the buffer 
    int bytes_read = read(0, command_buffer+command_index, COMMAND_BUFFER_SIZE-command_index);
    if (bytes_read <= 0) return;
    size_t last_command_end = 0;

    // Search for the carriage return in the new input (up until command_index+bytes_read)
    for (size_t index = command_index; index < (command_index+bytes_read); index++) {
        if (command_buffer[index] == '\r') {
            // Replace carriage return with null terminator (so strcmp works)
            // Convert string prior to null terminator to lowercase
            command_buffer[index] = '\0';
            for (size_t replacement_index = last_command_end; replacement_index < index; replacement_index++) {
                command_buffer[replacement_index] = tolower((unsigned char)command_buffer[replacement_index]);
            }
            handle_user_command(command_buffer+last_command_end);
            last_command_end = index+1;
        }
    }

    // Clean up buffer if any commands were found (shift any remaining characters to the beginning of the buffer)
    if (last_command_end > 0) {
        for (size_t index = last_command_end; index < (command_index+bytes_read); index++) {
            command_buffer[index-last_command_end] = command_buffer[index];
        }
        command_index = command_index + bytes_read - last_command_end;
    } else if ((command_index+bytes_read) >= COMMAND_BUFFER_SIZE) {
        printf("No command seen in command buffer so far - Discarding input\n");
        command_index = 0;
    } else {
        command_index += bytes_read;
    }
}

/**
 * This thread is responsible for monitoring for user input and processing actions as they come in.
 * It follows a polling model rather than have a bounded worst case execution period / computation time (since user input is a sporadic event).
 * The overall control of the application is dictated by this thread.
 * Other threads merely maintain the last state configured by commands received by this thread.
 */
void user_thread(UNUSED void* arg) {
    display_commands();
    
    // Want user input thread to be somewhat responsive but do not want it to take priority over threads controlling hardware
    // Only have it perform one call to process_user_input per iteration (variable amount of work depending on how much user input needs to be processed)
    while (1) {
        process_user_input();
        thread_yield();
    }
}

/**
 * This thread is responsible for maintaining a consistent motion for the stepper motor.
 * This thread also follows the polling model since speed is configurable (do not want to user kernel-level timer and period to control movement - would have been possible to just rely on periodic nature without timer but this couples kernel functionality to closely to the periodic task).
 * Also updates the current value of the angle global variable for other tasks to accurately assess where the measurement is originating.
 * This current implementation is blocking for the stepper_move command to better profile the execution time of kernel_level interrupts.
 */
void stepper_thread(UNUSED void* arg) {
    /// Current sweep direction (1 for forward 0 to 180 and 0 for backward 180 to 0 - set when radar is active and the arched sweep in occurring)
    unsigned char sweep_forward = 1;
    
    // Accumulates error over successive stepper motor moves (since one degree is not a clean number of steps)
    // Need 256 steps every 45 degrees (apply steps based on how much error is in the accumulator)
    unsigned int accumulator = 0; 

    while (1) {
        // Only move if radar is active or in calibration mode
        if (radar_active) {
            // Check if the stepper is currently moving forward (if yes move forward until direction needs to be reversed)
            if (sweep_forward) {
                // Set the stepper to advance by 1 degree
                // Add an additional step if there is enough error leftover in the accumulator from the last time an angle was swept out
                move_stepper((STEP_RATIO + accumulator) / DEGREE_RATIO);
                accumulator = (STEP_RATIO + accumulator) % DEGREE_RATIO;
                current_angle += 1;
                
                // Reverse direction once angle reaches 180 (actually check at 179 to prevent an additional LED from being included in calculations)
                if (current_angle == 179) {
                    sweep_forward = 0;  
                }
            } else {
                // Set the stepper to move backwards by one LED length
                move_stepper(-((STEP_RATIO + accumulator) / DEGREE_RATIO));
                accumulator = (STEP_RATIO + accumulator) % DEGREE_RATIO;
                current_angle -= 1;
                
                // Reverse the direction once angle has reached 0
                if (current_angle == 0) {
                    sweep_forward = 1;  // Reverse direction
                }
            }
        } else if (calibration_mode) {
            // During calibration mode rotate the stepper motor unconditionally (full 360 degrees rotation instead of just sweeping back and forth)
            // Don't use accumulator for calibration mode since the angle is going to be reset anyway
            move_stepper(5);
            current_angle += 1;
            accumulator = 0;
            
            // Keep angle bounded
            if (current_angle >= 360) {
                current_angle -= 360;
            }
        } else {
            // Radar not active so just yield for this cycle
            thread_yield();
        }
    }
}


/**
 * This thread is responsible for taking sensor measurements and updating the range of the `current_range` global variable.
 * Only this thread is responsible for updating the value of the last_ultrasonic_measurement variable (although other threads are allowed to reference it).
 * Both the sensor thread and indicator thread can be reliably profiled (instead of polling model), so updates to the measurement are made under a mutex.
 */
void sensor_thread(void* arg) {
    // Set up lock for synchronizing with the indicator thread
    non_polled_system_locks* locks = (non_polled_system_locks *)arg;
    unsigned int new_measurement = 0;

    while (1) {
        // Take ultrasonic measurement if the radar is active
        if (radar_active) {
            // Don't hold the lock while polling (use local variable)
            new_measurement = ultrasonic_read();

            // Update measusmrent global variable behind lock
            lock(locks->measurement_lock);
            last_ultrasonic_measurement = new_measurement;
            unlock(locks->measurement_lock);
        }

        // If radar is not active or measurement finished early, repeatedly yield to next cycle (prevents corruption of echo from trigger and keeps measurement periodic)
        thread_yield();
    }
}

/**
 * This thread is responsible for showing indications that a "radar" (ultrasonic) detection has been made at the current angle.
 * The ring LED will be activated to show that the current angle has an object closer than the defined threshold.
 * Reads from the last measurement are protected by the measurement lock (synchronizing with the sensor thread).
 */
void indicator_thread(void* arg) {
    // Set up lock for synchronizing with the sensor thread
    non_polled_system_locks* locks = (non_polled_system_locks *)arg;

    // If the radar is active update the LED currently faced by the radar to a new value
    // The same LED can be repeatedly updated depending on the speed of the radar (i.e. if an object move quickly into range while still facing the same LED)
    size_t led_index = 0; // Currently active LED based on current angle
    size_t last_led_index = 0; // Index of the last active LED

    // Sweep through the LEDs in the 180 degree arc and latch an LED if at an point a sensor measurement is within the range threshold (i.e. make measurements sticky)
    // An LED will stay high until this LED is measured again and no detections are made during its turn
    while (1) {
        if (radar_active) {
            // Calculate which LED corresponds to this angle (0-180 maps to 12 LEDs)
            // LED index wraps from zero_degrees_led
            led_index = (zero_degrees_led + current_angle / DEGREES_PER_LED) % NUM_RING_LEDS;
            
            // Check if this light should be lit based on if the last ultrasonic_measurement was less than the range threshold
            // Make the update with a gurantee that the most recent measurement is up to date
            lock(locks->measurement_lock);
            if (last_ultrasonic_measurement < current_range) {
                // Object detected within range so make this LED red
                neopixel_set(255, 0, 0, led_index);
            } else if (led_index != last_led_index) {
                // No object (turn off the LED - may already be off but just ensure that is true)
                // The LED is only turned off once when the light is first serviced (sticky detections - any detection after this will keep the light on)
                neopixel_set(0, 0, 0, led_index);
            }
            unlock(locks->measurement_lock);

            // Update last LED index to the led_index that was just served and load sequence
            last_led_index = led_index;
            neopixel_load();
        } else if (!calibration_mode) {
            // Neither active or in calibration mode
            // Turn off all LEDs when not active and not calibrating
            for (size_t index = 0; index < NUM_RING_LEDS; index++) {
                neopixel_set(0, 0, 0, index);
            }
            neopixel_load();
        }
        
        // Yield until the next cycle to avoid loading neopixel sequence too often (causing strange behavior)
        // Update last LED index to the led_index that was just served
        thread_yield();
    }
}

/** 
 * Main function for starting the user, stepper, indicator, and sensor threads.
 * Follows convention of creating timing constraints for tasks, defining threads, initializing mutexes, and starting the multitasking sequence.
 **/
int main(UNUSED int argc, UNUSED char *argv[]) {
    int ret;

    // Profiling justifications:
    // user_thread: Want the system to be responsive to user input (but not more responsive than the stepper motor) - 30 ms seems like a reasonable polling time for user input and 4 ms computational time seems like enough to perform any command (most of the time it will just yield anyway).
    // stepper_thread: At max speed, the stepper motor go through 6 steps in 6 * (60 * 1000 / 2048 / 10) ~ 18 ms. I set the polling frequency to slightly longer than this since there is other work to do (and I do not want it to completely monopolize the system).
    // sensor_thread: Set period equal to 80 ms (little more leeway from what the sensor recommonds) - The sensor which recommends a sensing period of 60 ms to avoid corrupting the echo line) - High execution time allows the system to idly poll the measurement while waiting for 36 ms in the worst case of timeout (should normally not need this entire time).
    // indicator_thread: Same period as the sensor_thread with the idea being that they will pass the measurement lock back and forth (both yielding for each measurement made) - Make sensor thread the higher priority in the event of a tie (want updated data before showing indication).
    uint32_t num_threads = 4, stack_size = 2048, num_mutexes = 1;
    uint32_t C[4] = {20, 5, 80, 360};
    uint32_t T[4] = {300, 40, 800, 800};
    void *threads[4] = {&user_thread, &stepper_thread, &sensor_thread, &indicator_thread};

    // Set up memory protection and request space for the threads
    ret = multitask_request(num_threads, stack_size, NULL, THREAD_PROTECT, num_mutexes);
    if(ret < 0) {
        puts("multitask_request failed");
        exit(1);
    }

    // Allocate locks struct
    non_polled_system_locks* locks = malloc(sizeof(non_polled_system_locks));
    if(locks == NULL) {
        puts("failed to malloc space for lock struct");
        exit(1);
    }

    // Highest priority thread is sensor_thread (id of 2 - higher priority than indicator_thread with tie break)
    // This lock has timing gurantees that it will not yield / exit while being held
    locks->measurement_lock = lock_init(2); 
    if(locks->measurement_lock == NULL) {
        puts("failed to correctly initialize locks");
        exit(1);
    }

    // Define threads with the above profiling and IDs equal to the index (also passing in the locks arg for threads that need it)
    for(uint32_t index = 0; index < num_threads; index++) {
        ret = thread_define(index, threads[index], (void *)locks, C[index], T[index]);
        if(ret < 0) {
            printf("thread_define failed for thread %lu\n", index);
            exit(1);
        }    
    }

    // Start preemptive scheduler at 10000 Hz
    // This is faster than most traditional scheduler and will incur a bit more overhead (however this keeps polling results responsive - especially the motor)
    ret = multitask_start(10000);
    if(ret < 0) {
        puts("multitask_start failed");
        exit(1);
    }

    // Should never reach here
    return -1;
}
