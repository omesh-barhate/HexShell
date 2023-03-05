#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 64


int main() {
  char input[MAX_INPUT_LENGTH];
  char *args[MAX_ARGS];
  pid_t pid;
  int status;

  // Shell loop
  while (1) {
    // Prompt user for input
    printf(">> ");
    fflush(stdout);

    // Read user input
    fgets(input, MAX_INPUT_LENGTH, stdin);

    // Parse user input into separate arguments
    char *token = strtok(input, " \t\n");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
      args[i++] = token;
      token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    // Check for built-in commands
    if (strcmp(args[0], "cd") == 0) {
      // Handle cd command
      if (chdir(args[1]) != 0) {
        perror("cd");
      }
      continue;
    } else if (strcmp(args[0], "exit") == 0) {
      // Handle exit command
      exit(0);
    }

    // Fork process and execute command
    pid = fork();
    if (pid == 0) {
      // Child process
      if (execvp(args[0], args) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
      }
    } else if (pid < 0) {
      // Error forking
      perror("fork");
    } else {
      // Parent process
      do {
        waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
  }

  return 0;
}

