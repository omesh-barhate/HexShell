# HexShell

This is a simple implementation of a shell program in C. The shell provides a command-line interface for executing other programs and basic built-in commands. It has been updated to include additional features and improve the user experience.

## Getting Started

To compile the program, run the following command in the terminal:

gcc shell.c -o shell -lncurses


To run the shell, simply execute the compiled binary:

./shell


The shell will display a prompt (`>>`) and wait for user input. The user can enter commands, which will be executed by the shell. The shell provides built-in commands such as `cd` for changing the current working directory and `exit` for exiting the shell.

## Features

The basic shell implementation provides the following features:

- command-line interface with prompt (>>) for user input
- Parsing user input into separate commands and arguments
- Execution of single commands or command pipelines
- Input/output redirection and piping
- Handling of signals such as SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z)
- Support for background process execution using the & symbol
- Basic error handling and reporting

## Usage

The shell supports the following features:

### Built-in Commands

The shell provides two built-in commands:

- cd [directory]: Change the current working directory to the specified directory. If no directory is provided, it changes to the user's home directory.
- exit: Exit the shell.

### Command Execution

The shell can execute external commands by entering their name followed by any required arguments. For example:

>> ls -l

### Command Pipelines
The shell supports executing command pipelines using the `|` symbol. For example:

>> ls -l | grep shell

This command lists files and directories in the current directory and pipes the output to `grep` to search for the word "shell".

### Input/Output Redirection

The shell supports input/output redirection using the `<`, `>`, and `>>` symbols. For example:

>> ls > files.txt

This command lists files and directories in the current directory and redirects the output to a file named "files.txt".

### Background Processes

To execute a command in the background, append an `&` symbol at the end of the command. For example:

>> sleep 10 &

This command executes the `sleep` command in the background, causing the shell to return immediately.


## Limitations

The basic shell implementation has some limitations, including:

- Lack of advanced features such as command history, auto-completion, and environment variable handling
- Limited error handling and reporting
- Not suitable for complex shell scripting

The shell is intended as a basic starting point and can be extended to include additional features based on specific requirements.



