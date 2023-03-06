# Basic Shell Implementation

This is a simple implementation of a shell program in C. The shell provides a command-line interface for executing other programs and basic built-in commands.

## Getting Started

To compile the program, run the following command in the terminal:

gcc shell.c -o shell


To run the shell, simply execute the compiled binary:

./shell


The shell will display a prompt (`>>`) and wait for user input. The user can enter commands, which will be executed by the shell. The shell also provides two built-in commands: `cd` and `exit`.

## Features

The basic shell implementation provides the following features:

- A simple command-line interface for executing other programs
- Support for parsing user input into separate arguments
- Built-in commands for changing the current working directory (`cd`) and exiting the shell (`exit`)
- Forking and executing commands in a child process
- Waiting for child processes to complete using the `waitpid` function
- Handling signals such as `SIGINT` (Ctrl-C) and `SIGTSTP` (Ctrl-Z)

## Limitations

The basic shell implementation has several limitations, including:

- Lack of support for advanced features such as input/output redirection, piping, and background processes
- No support for command history or auto-completion
- Limited error handling and error reporting



