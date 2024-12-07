#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

extern char **environ;
char delimiters[] = " \t\r\n";

// Global variable to track the current foreground process PID
static pid_t current_foreground_pid = -1;

// Signal handler for SIGINT (Ctrl-C) in the shell
// This ensures the shell itself is not killed by Ctrl-C
void sigint_handler(int sig) {
    if (current_foreground_pid == -1) {
        // If no foreground process is running, just print a newline and re-prompt
        printf("\n");
        fflush(stdout);
    } else {
        // If a foreground process is running, let that process handle it
        // (or be terminated if it uses default action)
        // The shell won't die because we didn't restore default handling here.
        // Do nothing special. Child will see default signal.
        printf("\n");
    }
}

// Signal handler for SIGALRM - to kill long-running foreground processes
void sigalrm_handler(int sig) {
    if (current_foreground_pid > 0) {
        kill(current_foreground_pid, SIGKILL);
        fprintf(stderr, "Process %d timed out and was killed.\n", current_foreground_pid);
    }
    current_foreground_pid = -1;
}

// Print the prompt showing the current working directory
void print_prompt() {
    char cwd[MAX_COMMAND_LINE_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
    } else {
        perror("getcwd error");
    }
    fflush(stdout);
}

void handle_cd(char *path) {
    if (path == NULL) {
        fprintf(stderr, "cd: missing argument\n");
    } else if (chdir(path) != 0) {
        fprintf(stderr, "cd: %s: No such file or directory\n", path);
    }
}

void handle_pwd() {
    char cwd[MAX_COMMAND_LINE_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd error");
    }
}

// Expand environment variables in any argument starting with '$'
void expand_environment_vars(char **args) {
    int i;
    for (i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char *value = getenv(args[i] + 1);
            if (value) {
                args[i] = value;
            } else {
                // If variable not found, replace with empty string
                args[i] = "";
            }
        }
    }
}

void handle_echo(char **args) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
}

void handle_env_command(char **args) {
    // If "env varname" is given, print that variable's value only
    if (args[1] != NULL) {
        char *value = getenv(args[1]);
        if (value) {
            printf("%s\n", value);
        }
        return;
    }

    // Otherwise print all environment variables
    int i;
    for ( i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
}

void handle_setenv_command(char *name_value) {
    if (name_value == NULL) {
        fprintf(stderr, "setenv: missing arguments\n");
        return;
    }
    char *name = strtok(name_value, "=");
    char *value = strtok(NULL, "");
    if (name == NULL || value == NULL) {
        fprintf(stderr, "setenv: invalid format, use name=value\n");
        return;
    }
    if (setenv(name, value, 1) != 0) {
        perror("setenv error");
    }
}

int main() {
    char command_line[MAX_COMMAND_LINE_LEN];
    char *arguments[MAX_COMMAND_LINE_ARGS];

    // Set up signal handlers for the shell
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    while (true) {
        print_prompt();

        if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
            fprintf(stderr, "fgets error\n");
            exit(1);
        }

        if (feof(stdin)) {
            // End-of-file (e.g. Ctrl-D)
            printf("\n");
            break;
        }

        // Strip trailing newline
        size_t len = strlen(command_line);
        if (len > 0 && command_line[len - 1] == '\n') {
            command_line[len - 1] = '\0';
        }

        if (strlen(command_line) == 0) {
            continue; // Ignore empty lines
        }

        // Tokenize input into arguments
        int arg_count = 0;
        char *token = strtok(command_line, delimiters);
        while (token != NULL && arg_count < MAX_COMMAND_LINE_ARGS - 1) {
            arguments[arg_count++] = token;
            token = strtok(NULL, delimiters);
        }
        arguments[arg_count] = NULL;

        if (arguments[0] == NULL) {
            continue;
        }

        // Check for background process
        bool is_background = false;
        if (arg_count > 0 && strcmp(arguments[arg_count - 1], "&") == 0) {
            is_background = true;
            arguments[arg_count - 1] = NULL;
            arg_count--;
        }

        // Expand environment variables
        expand_environment_vars(arguments);

        // Check for I/O redirection (just implementing '>' for demonstration)
        char *out_filename = NULL;
        int out_index = -1;
        int i;
        for (i = 0; i < arg_count; i++) {
            if (strcmp(arguments[i], ">") == 0) {
                if (i + 1 < arg_count) {
                    out_filename = arguments[i + 1];
                    out_index = i;
                    break;
                } else {
                    fprintf(stderr, "No output file specified after '>'\n");
                }
            }
        }
        if (out_index != -1) {
            arguments[out_index] = NULL;
            arg_count = out_index; // Adjust arg_count
        }

        // Built-in Commands
        if (strcmp(arguments[0], "cd") == 0) {
            handle_cd(arguments[1]);
            continue;
        } else if (strcmp(arguments[0], "pwd") == 0) {
            handle_pwd();
            continue;
        } else if (strcmp(arguments[0], "echo") == 0) {
            handle_echo(arguments);
            continue;
        } else if (strcmp(arguments[0], "env") == 0) {
            handle_env_command(arguments);
            continue;
        } else if (strcmp(arguments[0], "setenv") == 0) {
            handle_setenv_command(arguments[1]);
            continue;
        } else if (strcmp(arguments[0], "exit") == 0) {
            break;
        }

        // Not a built-in command: Fork and exec
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
        } else if (pid == 0) {
            // Child process
            // Restore default SIGINT handling here
            signal(SIGINT, SIG_DFL);

            // Handle output redirection if needed
            if (out_filename != NULL) {
                int fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("open error");
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    exit(1);
                }
                close(fd);
            }

            execvp(arguments[0], arguments);
            // If execvp returns, an error occurred
            perror("execvp error");
            fprintf(stderr, "An error occurred.\n");
            exit(1);
        } else {
            // Parent process
            if (is_background) {
                // Don't wait for background processes
                printf("Started background process with PID: %d\n", pid);
            } else {
                // Foreground process: set timer
                current_foreground_pid = pid;
                alarm(10); // Set a 10-second timer
                int status;
                waitpid(pid, &status, 0);
                alarm(0); // Cancel the alarm if process finished in time
                current_foreground_pid = -1;
            }
        }
    }

    return 0;
}
