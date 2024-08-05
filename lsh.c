// Lasha Geladze, CS406 Project 1.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


// Global variables for the path
#define MAX_ARGS 64
char *search_path[1024];
int path_count = 0;

char error_message[30] = "An error has occurred\n";

typedef struct {
    char *command[MAX_ARGS]; // Array of strings for each command and its arguments
    char *file;              // Redirection file, if any
    int redirect;            // 0: no redirection, 1: redirection, -1: error
} ParsedInput;

/**
 * Loop goes into the shell's main loop and reads commands from the specified input stream,
 * parses them, and executes until EOF or an error.
 *
 * @param stream The input stream. stdin for interactive mode or a file for batch mode.
 */
void loop(FILE *stream);

/**
 * Executes a parsed command if it is a built-in command (exit, cd, path). Otherwise,
 * a fork should be processed.
 *
 * @param parsed The parsed command.
 * @return 1 if the command was executed as a built-in command, 0 otherwise.
 */
int execute_command(ParsedInput parsed);

/**
 * Function handling the execution of parallel commands split by '&'.
 * Each command is executed in its own process.
 *
 * @param line The command line; can contain multiple commands.
 */
void execute_parallel_commands(char *line);

/**
 * Forks a new process to execute a non-built-in command. If necessary, handles I/O redirection.
 *
 * @param parsed The parsed command.
 */
void fork_process(ParsedInput parsed);

/**
 * Implements the 'exit' built-in command.
 *
 * @param parsed The parsed command.
 */
void builtin_exit(ParsedInput parsed);

/**
 * Implements the 'cd' built-in command.
 *
 * @param parsed The parsed command.
 */
void builtin_cd(ParsedInput parsed);

/**
 * Implements the 'path' built-in command.
 *
 * @param parsed The parsed command.
 */
void builtin_path(ParsedInput parsed);

/**
 * Parses a single command line by identifying the command, arguments, and redirection.
 *
 * @param cmdLine The command line.
 * @return A ParsedInput struct containing the parsed command, arguments, and redirection info.
 */
ParsedInput parse_input(char *cmdLine);

/**
 * Preprocesses a command line for redirection, tab and parallel symbols.
 *
 * @param cmdLine The command line .
 * @return A new string with added spaces around '>' and '&'; tabs converted to space.
 * (requiring free() after use).
 */
char* preprocess_cmdline(char* cmdLine);


int main(int argc, char **argv) {

    //initialize the default search path
    search_path[0] = strdup("/bin");
    path_count = 1;

    // Batch mode
    if (argc == 2) {
        FILE *batchFile = fopen(argv[1], "r");

        //Exit if the batch file cannot be opened
        if (batchFile == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        //pass batch file to loop to be processed
        loop(batchFile);
        fclose(batchFile);
    } else if (argc == 1) { // Interactive mode
        loop(stdin); // Use stdin if there is no batch
    } else {
        //too many arguments
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    //Free allocated memory for paths
    for (int i = 0; i < path_count; ++i) {
        free(search_path[i]);
    }

    return EXIT_SUCCESS;
}

void loop(FILE *stream) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t nread;

    while (1) {
        if (stream == stdin) {
            printf("lsh> ");
            fflush(stdout);
        }

        nread = getline(&line, &bufsize, stream);
        if (nread == -1) break; // Exit loop on EOF or read error

        line[strcspn(line, "\n")] = 0; // Remove newline

        // Preprocess command line
        char* preprocessedLine = preprocess_cmdline(line);
        if (!preprocessedLine) { // Check for error
            write(STDERR_FILENO, error_message, strlen(error_message));
            continue; //Skip this if preprocessing failed
        }

        // Check for parallel commands
        if (strstr(preprocessedLine, "&") != NULL) {
            execute_parallel_commands(preprocessedLine);
        } else {
            // Parse the processed command line
            ParsedInput parsed = parse_input(preprocessedLine);
            if (!execute_command(parsed)) {
                fork_process(parsed);
            }
        }

        //free memory
        free(preprocessedLine);
        free(line);
        line = NULL;
    }
}

void execute_parallel_commands(char *line) {
    char *commands[MAX_ARGS]; // Array to hold split commands
    int cmdCount = 0; // num of commands

    // split cmd line by '&'
    char *nextCmd = strtok(line, "&");

    // keep splitting until we hit NULL or max args
    while (nextCmd != NULL && cmdCount < MAX_ARGS - 1) {
        commands[cmdCount++] = nextCmd; // store command
        nextCmd = strtok(NULL, "&"); // get next command
    }

    pid_t pids[cmdCount]; // array to keep track of child PIDs

    for (int i = 0; i < cmdCount; i++) {
        ParsedInput parsed = parse_input(commands[i]); // parse each cmd
        if (parsed.command[0] != NULL) { // check if cmd isn't empty
            pid_t pid = fork(); // create a new process
            if (pid == 0) { // child
                if (!execute_command(parsed)) {
                    fork_process(parsed); // run if not a builtin cmd
                }
                exit(EXIT_SUCCESS); // exit cleanly after executing
            } else if (pid > 0) {
                pids[i] = pid; // save child pid for later
            } else {
                // if fork() failed, print error
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
    }
    // wait for all child processes to finish
    int status;
    for (int i = 0; i < cmdCount; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], &status, 0); // wait for each child
        }
    }
}


int execute_command(ParsedInput parsed) {
    // check if the command is empty before proceeding
    if (parsed.command[0] == NULL) {
        if (parsed.redirect == 1 && parsed.file != NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        return 1;
    }

    // run a specific built-in process
    if (strcmp(parsed.command[0], "exit") == 0) {
        builtin_exit(parsed);
        return 1;
    } else if (strcmp(parsed.command[0], "cd") == 0) {
        builtin_cd(parsed);
        return 1;
    } else if (strcmp(parsed.command[0], "path") == 0) {
        builtin_path(parsed);
        return 1;
    }
    return 0; // Not a built-in command
}


void fork_process(ParsedInput parsed) {

    // if there is an error in redirect, print and return
    if (parsed.redirect == -1) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }

    pid_t pid = fork();
    int status;

    if (pid == 0) { // Child process
        if (parsed.redirect && parsed.file) {
            //Handle redirect
            int fd = open(parsed.file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                //error opening a file
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(EXIT_FAILURE);
            }
            // Handle error and standard output
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        // Attempt to execute the command
        int cmd_found = 0;
        for (int i = 0; i < path_count && !cmd_found; ++i) {
            char fullPath[1024];
            // Construct actual path
            snprintf(fullPath, sizeof(fullPath), "%s/%s", search_path[i], parsed.command[0]);
            if (access(fullPath, X_OK) == 0) {
                execv(fullPath, parsed.command);
                cmd_found = 1; // execv does not return if successful
            }
        }

        // Throw error if no command found
        if (!cmd_found) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        exit(EXIT_FAILURE); // Exit if command not found or failed to execute
    } else if (pid < 0) { // Error forking
        write(STDERR_FILENO, error_message, strlen(error_message));
    } else { // Parent process waits
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

char* preprocess_cmdline(char* cmdLine) {
    int length = strlen(cmdLine);
    char* processedCmdLine = malloc(2 * length + 1); // Allocate space
    if (!processedCmdLine) return NULL; // Allocation failed

    int j = 0;
    for (int i = 0; i < length; i++) {
        // Handle tab, > and &
        if (cmdLine[i] == '\t') { //replace tab with space
            processedCmdLine[j++] = ' ';
        } else if (cmdLine[i] == '>' || cmdLine[i] == '&') {
            if (j > 0 && processedCmdLine[j - 1] != ' ') {
                processedCmdLine[j++] = ' ';
            }
            processedCmdLine[j++] = cmdLine[i];
            if (i < length - 1 && cmdLine[i + 1] != ' ') {
                processedCmdLine[j++] = ' ';
            }
        } else {
            processedCmdLine[j++] = cmdLine[i];
        }
    }
    processedCmdLine[j] = '\0'; // terminate
    return processedCmdLine;
}

ParsedInput parse_input(char *cmdLine) {
    //cmd line has been preprocessed
    ParsedInput parsed = { .file = NULL, .redirect = 0 };
    int tokenCount = 0;
    char *nextToken = strtok(cmdLine, " "); // tokenize directly
    while (nextToken != NULL) {
        if (strcmp(nextToken, ">") == 0) {
            parsed.redirect = 1;
            nextToken = strtok(NULL, " "); // Attempt to get file name
            if (nextToken != NULL) {
                parsed.file = strdup(nextToken);
                // Check for additional tokens after the file
                nextToken = strtok(NULL, " ");
                if (nextToken != NULL) {
                    // Additional tokens found so indicating error
                    parsed.redirect = -1;
                    free(parsed.file);
                    parsed.file = NULL;
                    break;
                }
                break; // No additional tokens found, break
            } else {
                parsed.redirect = -1; //no file, so error
                break;
            }
        } else {
            parsed.command[tokenCount++] = strdup(nextToken);
        }
        nextToken = strtok(NULL, " ");
    }
    parsed.command[tokenCount] = NULL; // Null-terminate

    //free(cmdLine) managed outside

    return parsed;
}


void builtin_exit(ParsedInput parsed) {
    if (parsed.command[1] != NULL) { //check for additional arguments
        write(STDERR_FILENO, error_message, strlen(error_message));
    } else {
        exit(EXIT_SUCCESS);
    }
}

void builtin_cd(ParsedInput parsed) {
    if (parsed.command[1] == NULL || chdir(parsed.command[1]) != 0) {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

void builtin_path(ParsedInput parsed) {
    //Clear existing
    for (int i = 0; i < path_count; ++i) {
        free(search_path[i]);
    }
    path_count = 0;

    //Set new path
    for (int i = 1; parsed.command[i] != NULL; i++) {
        search_path[path_count++] = strdup(parsed.command[i]);
        if (path_count >= sizeof(search_path) / sizeof(search_path[0])) break; // Prevent overflow
    }
}
