# Mini Shell

**Author:** Assaf Yaron  

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

## Overview

This program implements a **Mini Shell** that supports fundamental shell functionalities, including:

- Running commands (e.g., `ls`, `sleep 5`)  
- Executing commands in the **background** (e.g., `sleep 5 &`)  
- Piping output between two commands (e.g., `cat file.txt | grep pattern`)  
- Input redirection (e.g., `grep word < input.txt`)  
- Output redirection (e.g., `ls > output.txt`)  

The program makes use of process management via `fork()`, signal handling, and inter-process communication using **pipes**.  

---

## Features

This shell supports the following operations:

### Regular Command Execution
- Executes a program and waits for its completion.  
- Example:  
  ```bash
  ls -l
