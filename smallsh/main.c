#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 512
#define MAX_CHARS 2048


typedef struct Command {
    char *name;
    char *args[MAX_ARGS];
    int numArgs;
    char *inputRedirect;
    char *outputRedirect;
    int background;
} Command;

Command parseCommand(char *input) {
    input[strcspn(input, "\r\n")] = 0;
    Command cmd = {0};
    char *token;
    int i = 0;
    int args = 0;

    // Tokenize input string by whitespace
    token = strtok(input, " ");
    while (token != NULL && i < MAX_ARGS) {
        if (strcmp(token, "<") == 0) {
            cmd.inputRedirect = strtok(NULL, " ");
        } else if (strcmp(token, ">") == 0) {
            cmd.outputRedirect = strtok(NULL, " ");
        } else if (strcmp(token, "&") == 0) {
            cmd.background = 1;
        } else {
            // if (args == 0) {
            //     cmd.name = token;
            //     args = 1;
            // }
            cmd.args[cmd.numArgs] = token;
            cmd.numArgs++;
        }
        token = strtok(NULL, " ");
        i++;
    }

    cmd.args[cmd.numArgs] = NULL;

    // Extract the command name from the first argument
    if (cmd.numArgs > 0) {
        cmd.name = cmd.args[0];
    }

    return cmd;
}


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
            // Empty line, prompt again
            continue;
        } else {
            // Parse and return the command
            Command cmd = parseCommand(inputString);
            return cmd;
        }
    }
}

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

FILE *run_cmd(const char *command, const char *type) {
    int fds[2];
    const char *argv[4] = {"/bin/sh", "-c", command};
    pipe(fds);
    if (fork() == 0) {
        close(fds[0]);
        dup2(type[0] == 'r' ? 0 : 1, fds[1]);
        close(fds[1]);
        execvp(argv[0], argv);
        exit(-1);
    }
    close(fds[1]);
    return fdopen(fds[0], type);
}

typedef struct ShellState {
    pid_t backgroundProcesses[MAX_ARGS];
    int numBackgroundProcesses;
} ShellState;

void executeCommand(Command cmd, ShellState *state) {
    // int pipe_fd_1[2];
    // pipe(pipe_fd_1);
      int link[2]; 
    // fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // printf("Hello");

        if (cmd.inputRedirect) {
            int fd = open(cmd.inputRedirect, O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        }
        if (cmd.outputRedirect) {
            int fd = open(cmd.outputRedirect, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd);
        }
        if (!(cmd.outputRedirect || cmd.inputRedirect)) {
            dup2 (link[1], STDOUT_FILENO);
            close(link[0]);
            close(link[1]);
        }

        if (execvp(cmd.name, cmd.args) == -1) {
            //fflush(stdout);
            perror("execvp");
            exit(1);
        } else {
            //printf("Hello2");
            fflush(stdout);
            exit(0);
        }
    } else {
        // close(pipe_fd_1[1]);
        // dup2(pipe_fd_1[0], STDIN_FILENO);
        // close(pipe_fd_1[0]);
        // close(link[1]);
        // dup2 (link[1], STDOUT_FILENO);
        if (!cmd.background) {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(1);
            }
        } else {
            state->backgroundProcesses[state->numBackgroundProcesses] = pid;
            state->numBackgroundProcesses++;
            printf("background pid is %d\n", pid);
        }
    }
}


// struct Status {
//     int exitStatus;
//     int termSignal;
// };

// void handle_SIGTSTP(int signo);
// void handle_SIGINT(int signo);
// void flushInputBuffer();
// void printPrompt();
// void parseInput(char* input, struct Command* cmd);
// bool isComment(char* line);
// void expandPID(char* orig, char* dest);
// bool isBackgroundCommand(char* command);
// int runCommand(struct Command* cmd, struct Status* status);
// void executeBuiltInCommand(struct Command* cmd, struct Status* status);
// void executeNonBuiltInCommand(struct Command* cmd, struct Status* status);
// void checkBackgroundProcesses();



int main(int argc, char* argv[]) {

    ShellState state = {0};
    
    while (1) {
        Command* cmd = malloc(sizeof(Command));
        *cmd = getInputLoop();
        // prettyPrintCommand(*cmd);

        // initialize shell state
        // struct ShellState state;
        

        executeCommand(*cmd, &state);
    }

    return 0;
//   // initialize shell state
//   struct ShellState state;
//   state.backgroundPids = malloc(MAX_BACKGROUND_PROCESSES * sizeof(pid_t));
//   state.numBackgroundPids = 0;
//   state.exitStatus = 0;
//   state.isForegroundOnly = false;

//   // register signal handlers
//   struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};
//   SIGINT_action.sa_handler = handle_SIGINT;
//   SIGTSTP_action.sa_handler = handle_SIGTSTP;
//   sigfillset(&SIGINT_action.sa_mask);
//   sigfillset(&SIGTSTP_action.sa_mask);
//   SIGINT_action.sa_flags = 0;
//   SIGTSTP_action.sa_flags = 0;
//   sigaction(SIGINT, &SIGINT_action, NULL);
//   sigaction(SIGTSTP, &SIGTSTP_action, NULL);

//   // enter shell loop
//   while (true) {
//     // display prompt
//     printf(": ");
//     fflush(stdout);

//     // read line of input
//     char* input = NULL;
//     size_t inputSize = 0;
//     ssize_t numChars = getline(&input, &inputSize, stdin);
//     if (numChars == -1) { // end of file or error occurred
//       clearerr(stdin);
//       free(input);
//       break;
//     }

//     // remove trailing newline character, if any
//     if (input[numChars - 1] == '\n') {
//       input[numChars - 1] = '\0';
//     }

//     // parse command
//     struct Command command;
//     memset(&command, 0, sizeof(struct Command));
//     parseCommand(input, &command);

//     // execute command
//     executeCommand(&command, &state);

//     // free dynamically allocated memory
//     free(input);
//     freeCommand(&command);
//   }

//   // free dynamically allocated memory
//   free(state.backgroundPids);

  return 0;
}

