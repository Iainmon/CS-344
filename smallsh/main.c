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

#define MAX_ARGS 512
#define MAX_CHARS 2048

#define MAX_BACKGROUND_PROCESSES 512

pid_t proc_pid;

typedef struct Command {
    char *name;
    char *args[MAX_ARGS];
    int numArgs;
    char *inputRedirect;
    char *outputRedirect;
    int background;
} Command;


char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

Command parseCommand(char *input) {
    input[strcspn(input, "\r\n")] = 0;
    Command cmd = {0};
    char *token;
    int i = 0;
    int args = 0;

    char *buff = malloc(10);
    sprintf(buff, "%d", proc_pid);

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
            token = str_replace(token, "$$", buff);
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
    pid_t backgroundProcesses[MAX_BACKGROUND_PROCESSES];
    int numBackgroundProcesses;
    int exitStatus;
    int exitSignal;
} ShellState;


void sig_handler(int signo) {
    if (signo == SIGINT) {
        fflush(stdout);
        // sprintf(buff,"terminated by signal %d\n", signo);
        // exit(0);
    }
}

void sig_handler2(int signo) {
    if (signo == SIGTSTP) {
        printf("terminated by signal %d\n", signo);
        fflush(stdout);
    }
}
void executeCommand(Command cmd, ShellState *state) {
    // int pipe_fd_1[2];
    // pipe(pipe_fd_1);
      int link[2]; 
    // fflush(stdout);
    fflush(stdout);
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
        } else if (cmd.background) {
            int fd = open("/dev/null", O_RDONLY);
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
        } else if (cmd.background) {
            int fd = open("/dev/null", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("open");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
        }

        if (cmd.background == 0) {
            signal(SIGINT, sig_handler);
        }

        signal(SIGTSTP, SIG_IGN);
        // if (!(cmd.outputRedirect || cmd.inputRedirect)) {
        //     dup2 (link[1], STDOUT_FILENO);
        //     close(link[0]);
        //     close(link[1]);
        // }

        // fflush(stdout);
        // setpgid(0, 0);
        if (execvp(cmd.name, cmd.args) == -1) {
            perror("execvp");
            exit(1);
        } else {
            // fflush(stdout);
            exit(0);
        }
        exit(0);
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

            if (WIFEXITED(status)) {
                state->exitStatus = WEXITSTATUS(status);
                state->exitSignal = WTERMSIG(status);
            } else if (WIFSIGNALED(status)) {
                state->exitStatus = WEXITSTATUS(status);
                state->exitSignal = WTERMSIG(status);
                printf("terminated by signal %d\n", state->exitSignal);

            }
        } else {
            state->backgroundProcesses[state->numBackgroundProcesses] = pid;
            state->numBackgroundProcesses++;
            printf("background pid is %d\n", pid);
            fflush(stdout);
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

void handle_processes(ShellState *state) {
    // int status;
    // pid_t pid;
    // while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    //     printf("background pid %d is done: ", pid);
    //     if (WIFEXITED(status)) {
    //         printf("exit value %d ", WEXITSTATUS(status));
    //     } else if (WIFSIGNALED(status)) {
    //         printf("terminated by signal %d ", WTERMSIG(status));
    //     }
    //     printf("\n");
    //     fflush(stdout);
    // }

    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
            if (state->backgroundProcesses[i] == 0) {
                continue;
            }
            assert(state->backgroundProcesses[i] != 0);
            int status;
            pid_t pid = waitpid(state->backgroundProcesses[i], &status, WNOHANG);
            if (pid == -1) {
                perror("waitpid");
                exit(1);
            } else if (pid > 0 && (pid == state->backgroundProcesses[i] || WIFEXITED(status))) {
                printf("background pid %d is done: ", pid);
                if (WIFEXITED(status)) {
                    printf("exit value %d", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("terminated by signal %d", WTERMSIG(status));
                }
                printf("\n");
                fflush(stdout);
                state->backgroundProcesses[i] = 0;
            } else {
                // Process is still running
                // fflush(stdout);

            }
        }
}

bool allow_background = true;

void sig_handler3(int signo) {
    if (signo == SIGTSTP) {
        allow_background = !allow_background;
        if (allow_background) {
            printf("\nExiting foreground-only mode\n: ");
        } else {
            printf("\nEntering foreground-only mode (& is now ignored)\n: ");
        }
        fflush(stdout);
    }
}

int main(int argc, char* argv[]) {


    // if (signal(SIGINT, sig_handler) == SIG_ERR || signal(SIGTSTP, sig_handler) == SIG_ERR) {
    //     perror("Can't catch SIGINT or SIGTSTP\n");
    // }

    proc_pid = getpid();
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, sig_handler3);


    ShellState state = {0};
    for (int i = 0; i < MAX_BACKGROUND_PROCESSES; i++) {
        state.backgroundProcesses[i] = 0;
    }

    
    while (1) {

    
        handle_processes(&state);
        fflush(stdout);
        Command* cmd = malloc(sizeof(Command));
        *cmd = getInputLoop();
        // prettyPrintCommand(*cmd);
        if (!allow_background) {
            cmd->background = false;
        }

        if (strcmp(cmd->name, "exit") == 0) {
            exit(0);
        }
        if (strcmp(cmd->name, "cd") == 0) {
            if (cmd->args[1] == NULL) {
                chdir(getenv("HOME"));
            } else {
                chdir(cmd->args[1]);
            }
            continue;
        }
        if (strcmp(cmd->name, "status") == 0) {
            if (state.exitSignal != 0) {
                printf("terminated by signal %d\n", state.exitSignal);
            } else {
                printf("exit value %d\n", state.exitStatus);
            }
            fflush(stdout);
            continue;
        }

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

