
# Lafayette Shell (lsh) - Custom Unix Shell in C

## Project Overview
The Lafayette Shell (lsh) is a custom Unix shell implemented in C. The goal of this project was to develop a fully functional shell that could handle basic Unix commands, I/O operations, and parallel command execution. This project was developed on an Ubuntu environment running on Oracle VM VirtualBox on a Windows 11 machine, using CLion and GCC.

## Features
- **Basic Functionality**: 
  - Implemented core shell functionality including input/output handling and executing commands using a fork-and-exec pattern.
  - Built-in commands are supported with dedicated functions for each.
  - Process handling and error checking mechanisms were put in place to prevent zombie processes.
  
- **Tokenization and Parsing**:
  - Implemented a tokenization system to handle commands with multiple arguments.
  - Preprocessing of input to correctly handle special symbols, enabling uniform tokenization, redirection, and parallel execution.
  - Commands and arguments are organized into a `ParsedInput` structure to differentiate between redirection requirements and external commands.

- **Parallel Command Execution**:
  - Enabled parallel execution of commands through careful parsing and forking.
  - Used arrays to manage concurrent execution, ensuring the parent shell process waits for all child processes using `waitpid` for synchronized execution.

## Development Environment
- **Operating System**: Ubuntu (via Oracle VM VirtualBox on Windows 11)
- **Tools Used**: CLion IDE, GCC (GNU Compiler Collection)

## Implementation Details
1. **Initial Setup**:
   - The project began with setting up basic I/O functionality and a simple fork-and-exec pattern for command execution.
   - The loop function was implemented to continuously read input from the user or a batch file.
   
2. **Core Functionality**:
   - Built-in commands were added early on, as they are straightforward yet critical for shell operations.
   - Process handling was introduced to manage child processes and prevent zombie processes.
   
3. **Advanced Features**:
   - Focused on implementing a robust tokenization and parsing system to handle complex command structures.
   - Input is preprocessed to manage special symbols, then tokenized into commands and arguments.
   - A `ParsedInput` structure was introduced to organize the command line input for further processing.
   
4. **Parallel Execution**:
   - Implemented parallel command execution by parsing commands and forking processes accordingly.
   - Managed concurrent execution using arrays and ensured the parent shell process waited for all children with `waitpid`.
   
5. **Testing and Debugging**:
   - The shell was rigorously tested using both provided tests and custom commands to cover edge cases.
   - An iterative testing approach was adopted to refine the shell's functionality.

## Learning Outcomes
This project deepened my understanding of Unix-based system calls, C programming, and string parsing. Working on this shell taught me the complexities of system-level programming and the need for nuanced approaches to solve different types of problems.

## How to Run
1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/lafayette-shell.git
   cd lafayette-shell
   ```
2. **Compile the Shell**:
   ```bash
   gcc -o lsh shell.c
   ```
3. **Run the Shell**:
   ```bash
   ./lsh
   ```

## Future Work
Potential improvements include adding support for more complex shell features such as piping, advanced redirection, and signal handling.
