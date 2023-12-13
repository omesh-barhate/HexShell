#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/time.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 64
#define MAX_BACKGROUND_PROCESSES 64

pid_t background_processes[MAX_BACKGROUND_PROCESSES];
int num_background_processes = 0;

int redirect_input(const char* filename) {
    //  redirect standard input to it
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }
    if (dup2(fileno(file), STDIN_FILENO) == -1) {
        perror("dup2");
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

int redirect_output(const char* filename) {
    // redirect standard output to it
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }
    if (dup2(fileno(file), STDOUT_FILENO) == -1) {
        perror("dup2");
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

int append_output(const char* filename) {
    //  appending and redirect standard output to it
    FILE* file = fopen(filename, "a");
    if (file == NULL) {
        perror("fopen");
        return -1;
    }
    if (dup2(fileno(file), STDOUT_FILENO) == -1) {
        perror("dup2");
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

void restore_io() {
    // Restore standard input and output to their default values
    dup2(STDIN_FILENO, fileno(stdin));
    dup2(STDOUT_FILENO, fileno(stdout));
}

void handle_signal(int sig) {
    switch (sig) {
        case SIGINT:
            printf("\nReceived SIGINT signal. Terminating the current process.\n");
            exit(EXIT_FAILURE);
            break;
        case SIGTSTP:
            printf("\nReceived SIGTSTP signal. Stopping the current process.\n");
            // TODO: Implement process suspension logic here
            break;
        default:
            printf("\nReceived signal %d. Handling not implemented.\n", sig);
            break;
    }
}


int execute_pipeline(char* commands[MAX_ARGS][MAX_ARGS], int num_commands) {
    int pipes[num_commands - 1][2];
    pid_t pids[num_commands];
    int i;

    // Create pipes
    for (i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return -1;
        }
    }

    // Fork and execute commands
    for (i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process

            // Set up input redirection for the first command
            if (i == 0) {
                if (commands[i][1] != NULL) {
                    if (redirect_input(commands[i][1]) == -1) {
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                // Set up input redirection from the previous pipe
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(pipes[i - 1][0]);
            }

            // Set up output redirection for the last command
            if (i == num_commands - 1) {
                if (commands[i][2] != NULL) {
                    if (redirect_output(commands[i][2]) == -1) {
                        exit(EXIT_FAILURE);
                    }
                }
            } else {
                // Set up output redirection to the current pipe
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(pipes[i][1]);
            }

            // Close all pipes
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (execvp(commands[i][0], commands[i]) == -1) {
                perror("execvp");
                _exit(EXIT_FAILURE);  // Use _exit to avoid flushing buffers and calling atexit functions
            }
        } else if (pid < 0) {
            perror("fork");
            return -1;
        } else {
            pids[i] = pid;
        }
    }

    for (i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    int status;
    for (i = 0; i < num_commands; i++) {
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return -1;
        }
    }

    return 0;
}

void background_status() {
    int i;
    for (i = 0; i < num_background_processes; i++) {
        pid_t bg_pid = background_processes[i];
        int status;
        int result = waitpid(bg_pid, &status, WNOHANG);

        if (result == -1) {
            perror("waitpid");
        } else if (result == 0) {
            printf("Background process %d is still running.\n", (int)bg_pid);
        } else {
            printf("Background process %d has terminated.\n", (int)bg_pid);
            // TODO : implentation of cleanup can be done
        }
    }
}

volatile sig_atomic_t timeout_flag = 0;

void timeout_handler(int signum) {
    timeout_flag = 1;
}

void timed_command(char* command, int timeout_seconds, char* args[]) {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout_seconds);

        if (execvp(command, args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        alarm(0);

        if (timeout_flag) {
            printf("Timeout reached. Terminating process with PID %d.\n", pid);
              if (kill(pid, SIGTERM) == -1) {
                perror("kill");
            }

            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
            }
            exit(EXIT_FAILURE);
        }
    } else {
        perror("fork");
        exit(EXIT_FAILURE); 
    }
}


int main() {
    char input[MAX_INPUT_LENGTH];
    char ***commands = malloc(MAX_ARGS * sizeof(char **));
    for (int i = 0; i < MAX_ARGS; i++) {
        commands[i] = malloc(MAX_ARGS * sizeof(char *));
    }

    int num_commands;
    pid_t pid;
    int status;

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    // Shell loop
    while (1) {
       // Prompt user for input
        printf(">> ");
        fflush(stdout);

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // Handle EOF or error
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        char *token = strtok(input, "|\n");
        num_commands = 0;
       while (token != NULL && num_commands < MAX_ARGS) {
            // Use dynamic memory allocation for command arguments
            char **temp = malloc(MAX_ARGS * sizeof(char *));
            if (temp == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            char *arg_token = strtok(token, " \t");
            int i = 0;
            while (arg_token != NULL && i < MAX_ARGS - 1) {
                temp[i++] = strdup(arg_token);
                if (temp[i - 1] == NULL) {
                    perror("strdup");
                    exit(EXIT_FAILURE);
                }
                arg_token = strtok(NULL, " \t");
            }
            temp[i] = NULL;

            memcpy(commands[num_commands], temp, MAX_ARGS * sizeof(char *));
            num_commands++;
            token = strtok(NULL, "|\n");
        }


        if (strcmp(commands[0][0], "bgstatus") == 0) {
            background_status();
            continue;
        } else if (strcmp(commands[0][0], "timeout") == 0) {
            if (commands[0][1] != NULL && commands[0][2] != NULL) {
                int timeout_seconds = atoi(commands[0][1]);

                timed_command(commands[0][2], timeout_seconds, commands[0] + 2);
            } else {
                printf("Usage: timeout <seconds> <command>\n");
            }
            continue;
        } else if (strcmp(commands[0][0], "exit") == 0) {
            break;
        } 

        pid = fork();
        if (pid == 0) {
            if (num_commands > 1) {
                if (redirect_input(commands[0][1]) == -1) {
                    exit(EXIT_FAILURE);
                }
            }

            if (num_commands > 1) {
                if (redirect_output(commands[num_commands - 1][2]) == -1) {
                    exit(EXIT_FAILURE);
                }
            }

            if (num_commands == 1) {
                if (execvp(commands[0][0], commands[0]) == -1) {
                    perror("execvp");
                    _exit(EXIT_FAILURE);
                }
            } else {
                if (execute_pipeline(commands, num_commands) == -1) {
                    _exit(EXIT_FAILURE);
                }
            }

        } else if (pid < 0) {
            perror("fork");
        } else {
            if (commands[num_commands - 1][0][0] != '&') {
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            } else {
                background_processes[num_background_processes++] = pid;
            }

            int i = 0;
            for (int i = num_background_processes - 1; i >= 0; i--) {
                pid_t bg_pid = background_processes[i];
                int result = waitpid(bg_pid, &status, WNOHANG);
                if (result == -1) {
                    perror("waitpid");
                } else if (result > 0) {
                    for (int j = i; j < num_background_processes - 1; j++) {
                        background_processes[j] = background_processes[j + 1];
                    }
                    num_background_processes--;
                }
            }

        }
        for (int i = 0; i < num_commands; i++) {
            for (int j = 0; commands[i][j] != NULL; j++) {
                free(commands[i][j]);
            }
            free(commands[i]);
        }
    
    }
    return 0;
}

