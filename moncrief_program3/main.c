#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>

char *str_replace(char *orig, char *rep, char *with); // Helper function

// Define the constants for the maximum number of arguments and characters
#define MAX_ARGS 512
#define MAX_CHARS 2048

// Define the constants for the maximum number of background processes
#define MAX_BACKGROUND_PROCESSES 512

// Global process ID state
pid_t proc_pid;

// Define the struct for a command
typedef struct Command {
    char *name;
    char *args[MAX_ARGS];
    int numArgs;
    char *inputRedirect;
    char *outputRedirect;
    int background;
} Command;


// Function for parsing a command string into a Command struct
Command parseCommand(char *input) {
    // Set the last character of the input to null
    input[strcspn(input, "\r\n")] = 0;
    Command cmd = {0};
    char *token;
    int i = 0;
    int args = 0;

    // Get the process ID as string
    char *buff = malloc(10);
    sprintf(buff, "%d", proc_pid);

    // Tokenize input string by whitespace
    token = strtok(input, " ");
    while (token != NULL && i < MAX_ARGS) {
        // Check for special characters
        if (strcmp(token, "<") == 0) {
            // Set the input redirect
            cmd.inputRedirect = strtok(NULL, " ");
        } else if (strcmp(token, ">") == 0) {
            // Set the output redirect
            cmd.outputRedirect = strtok(NULL, " ");
        } else if (strcmp(token, "&") == 0) {
            // Set the background flag
            cmd.background = 1;
        } else {
            // Add the argument to the command
            token = str_replace(token, "$$", buff);
            cmd.args[cmd.numArgs] = token;
            cmd.numArgs++;
        }
        // Get the next token
        token = strtok(NULL, " ");
        i++;
    }

    // Set the last argument to null
    cmd.args[cmd.numArgs] = NULL;

    // Extract the command name from the first argument
    if (cmd.numArgs > 0) {
        cmd.name = cmd.args[0];
    }

    return cmd;
}



// Routine that collects the user input
Command getInputLoop() {

    char* inputString = NULL;
    ssize_t bytesRead = 0;
    size_t inputStringSize = 0;

    while (1) {
        // Print the prompt
        printf(": ");

        // Read input from stdin
        bytesRead = getline(&inputString, &inputStringSize, stdin);

        // Check for error or EOF
        if (bytesRead == -1) {
            perror("Error reading input");
            exit(1);
        } else if (bytesRead == 0 || strcmp(inputString, "\n") == 0) {
            // Empty line, return null command
            Command cmd = {0};
            cmd.name = NULL;
            return cmd;
        } else if (inputString[0] == '#') {
            // Comment, return null command
            Command cmd = {0};
            cmd.name = NULL;
            return cmd;
        } else {
            // Parse and return the command
            Command cmd = parseCommand(inputString);
            return cmd;
        }
    }
}

// Helper function for printing a command struct
void prettyPrintCommand(Command cmd) {
    // Print the command and arguments
    printf("Command: %s\n", cmd.name);
    printf("Arguments: ");
    for (int i = 0; i < cmd.numArgs; i++) {
        printf("%s ", cmd.args[i]);
    }
    printf("\n");

    // Print input and output files
    if (cmd.inputRedirect) {
        printf("Input file: %s\n", cmd.inputRedirect);
    }
    if (cmd.outputRedirect) {
        printf("Output file: %s\n", cmd.outputRedirect);
    }

    // Print whether command should be run in the background
    printf("Run in background: %d\n", cmd.background);
}




// Struct for storing the state of the shell
typedef struct ShellState {
    pid_t backgroundProcesses[MAX_BACKGROUND_PROCESSES];
    int numBackgroundProcesses;
    int exitStatus;
    int exitSignal;
} ShellState;

// sig handler for SIGINT
void sig_handler(int signo) {
    if (signo == SIGINT) {
        fflush(stdout);
    }
}


// Function that runs the command struct
void executeCommand(Command cmd, ShellState *state) {

    // Fork process but keep fd link
    int link[2]; 
    fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process

        // Set up input and output redirection
        if (cmd.inputRedirect) {
            // Open the input file
            int fd = open(cmd.inputRedirect, O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            // Redirect stdin to the input file
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        } else if (cmd.background) { // if background, redirect to /dev/null
            // Open /dev/null
            int fd = open("/dev/null", O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            // Redirect stdin to /dev/null
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        }

        // Set up output redirection for output
        if (cmd.outputRedirect) {
            // Open the output file
            int fd = open(cmd.outputRedirect, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            // Redirect stdout to the output file
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        } else if (cmd.background) { // if background, redirect to /dev/null
            // Open /dev/null
            int fd = open("/dev/null", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            // Redirect stdout to /dev/null
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
        }
        // If background, ignore SIGINT
        if (cmd.background == 0) {
            signal(SIGINT, sig_handler);
        }
        
        // Always ignore SIGTSTP
        signal(SIGTSTP, SIG_IGN);

        // Execute the command
        if (execvp(cmd.name, cmd.args) == -1) {
            perror("execvp");
            exit(1);
        } else {
            // fflush(stdout);
            exit(0);
        }
        exit(0);
    } else {
        // Parent process
        if (!cmd.background) {
            // Wait for the child process to finish
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(1);
            }
            // Check the exit status or signal of the child process
            if (WIFEXITED(status)) {
                state->exitStatus = WEXITSTATUS(status);
                state->exitSignal = WTERMSIG(status);
            } else if (WIFSIGNALED(status)) {
                state->exitStatus = WEXITSTATUS(status);
                state->exitSignal = WTERMSIG(status);
                printf("terminated by signal %d\n", state->exitSignal);
            }
        } else {
            // Add the child process to the list of background processes
            state->backgroundProcesses[state->numBackgroundProcesses] = pid;
            state->numBackgroundProcesses++;
            printf("background pid is %d\n", pid);
            fflush(stdout);
        }
    }
}

// Function that handles the list of background processes
void handle_processes(ShellState *state) {
    // Check if any background processes have finished
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
    
        // Skip if there is no process
        if (state->backgroundProcesses[i] == 0) {
            continue;
        }
        assert(state->backgroundProcesses[i] != 0);

        // Check if the process has finished
        int status;
        pid_t pid = waitpid(state->backgroundProcesses[i], &status, WNOHANG);
        if (pid == -1) {
            // Error
            perror("waitpid");
            exit(1);
        } else if (pid > 0 && (pid == state->backgroundProcesses[i] || WIFEXITED(status))) {
            // Process is no longer running. Need to display status
            printf("background pid %d is done: ", pid);
            if (WIFEXITED(status)) {
                // Process exited normally
                printf("exit value %d", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                // Process was terminated by a signal
                printf("terminated by signal %d", WTERMSIG(status));
            }
            printf("\n");
            fflush(stdout);
            state->backgroundProcesses[i] = 0; // Remove the process from the list
        } else {
            // Process is still running
        }
    }
}

// Function that kills all background processes
void kill_all_child_processes(ShellState *state) {
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {

        // Skip if there is no process
        if (state->backgroundProcesses[i] == 0) {
            continue;
        }
        assert(state->backgroundProcesses[i] != 0);

        // Kill the process
        pid_t pid = state->backgroundProcesses[i];
        printf("killing pid %d\n", pid);
        fflush(stdout);
        kill(pid, SIGTERM);

    }
}

// foreground-only mode state
bool allow_background = true;

// Signal handler for SIGTSTP for main function
void sig_handler3(int signo) {

    // Check which signal was received
    if (signo == SIGTSTP) {

        // Toggle foreground-only mode
        allow_background = !allow_background;
        if (allow_background) {
            printf("\nExiting foreground-only mode\n: ");
        } else {
            printf("\nEntering foreground-only mode (& is now ignored)\n: ");
        }

        // Print the prompt
        fflush(stdout);
    }
}

int main(int argc, char* argv[]) {

    // Ignore SIGINT and bind SIGTSTP to sig_handler3
    proc_pid = getpid();
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, sig_handler3);

    // Initialize the shell state
    ShellState state = {0};
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
        state.backgroundProcesses[i] = 0;
    }

    // Enter main loop
    while (1) {

        // Check if any background processes have finished and print the status before yielding to the user
        handle_processes(&state);
        fflush(stdout);

        // Get command from user
        Command* cmd = malloc(sizeof(Command)); // Allocate memory for the command
        *cmd = getInputLoop();                  // Get the command from the user
        // prettyPrintCommand(*cmd); // DEBUG

        // Check if user just hit return
        if (cmd->name == NULL) {
            continue;
        }

        // If the state is in foreground-only mode, ignore the background flag
        if (!allow_background) {
            cmd->background = false;
        }

        // Check if the command is a built-in command (exit, cd, status)

        // Built-in command: exit
        if (strcmp(cmd->name, "exit") == 0) {
            kill_all_child_processes(&state);
            break;
        }
        // Built-in command: cd
        if (strcmp(cmd->name, "cd") == 0) {
            // If no argument is given, change to the home directory
            if (cmd->args[1] == NULL) {
                chdir(getenv("HOME"));
            } else {
                // Change to the specified directory
                chdir(cmd->args[1]);
            }
            continue;
        }
        // Built-in command: status
        if (strcmp(cmd->name, "status") == 0) {
            // Print the exit status or terminating signal of the last foreground process
            if (state.exitSignal != 0) {
                printf("terminated by signal %d\n", state.exitSignal);
            } else {
                printf("exit value %d\n", state.exitStatus);
            }
            fflush(stdout);
            continue;
        }

        executeCommand(*cmd, &state);
        
    }


    // Return the exit status of the last foreground process
    return state.exitStatus;
}
















// Helper function I found for replacing a substring with another string 
char *str_replace(char *orig, char *rep, char *with) {
    char *result; 
    char *ins;
    char *tmp;
    int len_rep;
    int len_with;
    int len_front;
    int count;
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep;
    }
    strcpy(tmp, orig);
    return result;
}