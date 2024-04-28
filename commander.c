#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MAX_COMMAND_LENGTH 100

typedef struct {
    int type;
    // and anything else we need for command
} Command;

typedef struct {
    //ID, state, arrival time, etc.
} Process;


void execute_command(Command *cmd) {
    switch (cmd->type) {
        case 1:
            // Update system timer
            // Check if any process needs to change state based on the timer
            // Handle 'Q' command
            break;
        case 2:
            // Remove a process from the blocked queue and move it to the ready queue
            // Handle 'U' command
            break;
        case 3:
            // Print out current system state (state of all processes, queue statuses, etc)
            // Handle 'P' command
            break;
        case 4:
            // Calculate and print the average turnaround time
            // Terminate the system
            // Handle 'T' command
            exit(0);
            break;
        default:
            printf("Invalid command\n");
            break;
    }
    
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    Command cmd;

    while (1) {
        // Read a command from standard input
        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            break; // End of input
        }

        // Parse the command and feed the Command struct
        if (strlen(input) == 2 && input[0] == 'Q') {
            cmd.type = 1;
        } else if (strlen(input) == 2 && input[0] == 'U') {
            cmd.type = 2;
        } else if (strlen(input) == 2 && input[0] == 'P') {
            cmd.type = 3;
        } else if (strlen(input) == 2 && input[0] == 'T') {
            cmd.type = 4;
        } else {
            printf("Invalid command\n");
            continue;
        }

        execute_command(&cmd);
    }

    return 0;
}