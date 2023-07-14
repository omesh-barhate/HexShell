#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ncurses.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 64
#define MAX_BACKGROUND_PROCESSES 64

pid_t background_processes[MAX_BACKGROUND_PROCESSES];
int num_background_processes = 0;

int redirect_input(const char* filename) {
    // Open the file for reading and redirect standard input to it
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
    // Open the file for writing and redirect standard output to it
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
    // Open the file for appending and redirect standard output to it
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
    if (sig == SIGINT) {
        printw("\nReceived SIGINT signal, terminating the current process.\n");
        refresh();
        exit(EXIT_FAILURE);
    } else if (sig == SIGTSTP) {
        printw("\nReceived SIGTSTP signal, stopping the current process.\n");
        refresh();
        // TODO: Implement process suspension logic here
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
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            // Error forking
            perror("fork");
            return -1;
        } else {
            // Parent process
            pids[i] = pid;
        }
    }

    // Close all pipes in the parent process
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

int main() {
    char input[MAX_INPUT_LENGTH];
    char *commands[MAX_ARGS][MAX_ARGS];
    int num_commands;
    pid_t pid;
    int status;

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    // Shell loop
    while (1) {
        // Prompt user for input
        printw(">> ");
        refresh();

        // Read user input
        getstr(input);
	input[strcspn(input, "\n")] = '\0';

        // Parse user input into separate commands
        char *token = strtok(input, "|\n");
        num_commands = 0;
        while (token != NULL && num_commands < MAX_ARGS) {
            char *arg_token = strtok(token, " \t");
            int i = 0;
            while (arg_token != NULL && i < MAX_ARGS - 1) {
                commands[num_commands][i++] = arg_token;
                arg_token = strtok(NULL, " \t");
            }
            commands[num_commands][i] = NULL;

            num_commands++;
            token = strtok(NULL, "|\n");
        }

        // Check for built-in commands
        if (strcmp(commands[0][0], "cd") == 0) {
            // Handle cd command
            if (chdir(commands[0][1]) != 0) {
                perror("cd");
            }
            continue;
        } else if (strcmp(commands[0][0], "exit") == 0) {
            // Handle exit command
            break;
        }

        // Fork process and execute commands
        pid = fork();
        if (pid == 0) {
            // Child process

            // Set up input redirection for the first command
            if (num_commands > 1) {
                if (redirect_input(commands[0][1]) == -1) {
                    exit(EXIT_FAILURE);
                }
            }

            // Set up output redirection for the last command
            if (num_commands > 1) {
                if (redirect_output(commands[num_commands - 1][2]) == -1) {
                    exit(EXIT_FAILURE);
                }
            }

            if (num_commands == 1) {
                // Execute a single command
                if (execvp(commands[0][0], commands[0]) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Execute a pipeline of commands
                if (execute_pipeline(commands, num_commands) == -1) {
                    exit(EXIT_FAILURE);
                }
            }
        } else if (pid < 0) {
            // Error forking
            perror("fork");
        } else {
            // Parent process
            if (commands[num_commands - 1][0][0] != '&') {
                // Wait for foreground process to finish
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            } else {
                // Background process, add PID to the array
                background_processes[num_background_processes++] = pid;
            }

            // Check for completed background processes
            int i = 0;
            while (i < num_background_processes) {
                pid_t bg_pid = background_processes[i];
                int result = waitpid(bg_pid, &status, WNOHANG);
                if (result == -1) {
                    // Error
                    perror("waitpid");
                } else if (result > 0) {
                    // Process completed, remove PID from the array
                    int j;
                    for (j = i; j < num_background_processes - 1; j++) {
                        background_processes[j] = background_processes[j + 1];
                    }
                    num_background_processes--;
                } else {
                    i++;
                }
            }
        }
    }

    // Clean up ncurses
    endwin();

    return 0;
}

