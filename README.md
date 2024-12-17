# Mini Shell

**Author:** [Assaf Yaron]  

---

## Table of Contents

1. [Overview](#overview)  
2. [Features](#features)  
3. [Code Structure](#code-structure)  
4. [System Calls and Signals](#system-calls-and-signals)  
5. [Usage](#usage)  
6. [Compilation Instructions](#compilation-instructions)  
7. [Error Handling](#error-handling)  
8. [Design Choices](#design-choices)  

---

## 1. Overview

This program implements a **Mini Shell** that supports fundamental shell functionalities, including:

- Running commands (e.g., \`ls\`, \`sleep 5\`)  
- Executing commands in the **background** (e.g., \`sleep 5 &\`)  
- Piping output between two commands (e.g., \`cat file.txt | grep pattern\`)  
- Input redirection (e.g., \`grep word < input.txt\`)  
- Output redirection (e.g., \`ls > output.txt\`)  

The program makes use of process management via \`fork()\`, signal handling, and inter-process communication using **pipes**.  

---

## 2. Features

This shell supports the following operations:

### 2.1 Regular Command Execution
- Executes a program and waits for its completion.  
- Example:  
  \`\`\`
  ls -l
  \`\`\`

### 2.2 Background Execution
- Executes a program in the background using the \`&\` symbol.  
- Example:  
  \`\`\`
  sleep 5 &
  \`\`\`

### 2.3 Single Piping
- Pipes the output of one command to the input of another using the \`|\` symbol.  
- Example:  
  \`\`\`
  cat file.txt | grep pattern
  \`\`\`

### 2.4 Input Redirection
- Redirects input to a command using the \`<\` symbol.  
- Example:  
  \`\`\`
  grep word < input.txt
  \`\`\`

### 2.5 Output Redirection
- Redirects output to a file using the \`>\` symbol.  
- If the file exists, it is **truncated**. If it doesn't exist, it is created with mode \`0600\`.  
- Example:  
  \`\`\`
  ls > output.txt
  \`\`\`

---

## 3. Code Structure

The program comprises the following main components:

### 3.1 \`prepare()\`
- Prepares the shell before the first command execution.
- Configures signal handling for:
  - **SIGCHLD:** Reaps background child processes to prevent zombies.
  - **SIGINT:** Ignores \`Ctrl+C\` for the parent shell.  

### 3.2 \`process_arglist(int count, char **arglist)\`
- Processes the parsed user input and executes the corresponding command.  
- Determines the **case** based on the input:  
  - \`0\`: Regular command execution  
  - \`1\`: Background execution (\`&\`)  
  - \`2\`: Piping (\`|\`)  
  - \`3\`: Input redirection (\`<\`)  
  - \`4\`: Output redirection (\`>\`)  

### 3.3 \`finalize()\`
- Finalizes the shell and cleans up resources (if necessary).  

### 3.4 Helper Functions
- **\`get_pipe_index\`**: Finds the index of the \`|\` symbol.  
- **\`return_case\`**: Identifies the type of shell command based on input.  
- **\`execute_*\`**: Functions for each case:  
  - \`execute\`: Regular command execution  
  - \`execute_amp\`: Background execution  
  - \`execute_stin\`: Input redirection  
  - \`execute_stout\`: Output redirection  
  - \`execute_pipe\`: Piping  

---

## 4. System Calls and Signals

The program makes extensive use of the following **system calls** and **signals**:

### System Calls
- **\`fork()\`**: To create child processes.  
- **\`execvp()\`**: To execute external programs.  
- **\`waitpid()\`**: To wait for child processes to finish (supports non-blocking mode for zombies).  
- **\`pipe()\`**: To create pipes for inter-process communication.  
- **\`dup2()\`**: To redirect input/output file descriptors.  
- **\`open()\`**: To open files for input/output redirection.  

### Signals
- **SIGCHLD**: Handled using \`sigaction()\` to reap terminated background processes.  
- **SIGINT**: Ignored by the parent shell using \`signal(SIGINT, SIG_IGN)\`.  

---

## 5. Usage

1. Compile the program (see [Compilation Instructions](#6-compilation-instructions)).  
2. Run the shell:  
   \`\`\`
   ./shell
   \`\`\`
3. Enter commands. Examples:
   - Regular command: \`ls -l\`  
   - Background execution: \`sleep 10 &\`  
   - Piping: \`cat file.txt | grep word\`  
   - Input redirection: \`grep word < input.txt\`  
   - Output redirection: \`ls > output.txt\`  

4. To exit the shell, press \`Ctrl+D\`.  

---

## 6. Compilation Instructions

To compile the shell, use the following command:

\`\`\`bash
gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c
\`\`\`

---

## 7. Error Handling

The program handles errors gracefully:
- Invalid system calls print proper error messages.  
- Errors do not crash the shell.  

---

## 8. Design Choices

- **Single Fork:** Background processes are handled using one `fork`.  
- **Pipe Abstraction:** Reuses `arglist` to avoid extra memory allocation.    
