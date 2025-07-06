# Welcome to CSEShell by CL01 Team 1 🐚

CSEShell is a custom-built UNIX command-line shell created by CL01 Team 1! This shell mimics the behaviour of common terminal environments, supporting essential built-in commands (cd, help, exit), dynamic environment control and execution of external programs. We also developed system utilities (file search, dynamic library inspection and daemon processes) to showcase OS-level programming in C. CSEShell is built to handle the navigation of directories, managing environment variables and more with efficiency. 

## How to Compile & Run Shell 

Requirements: Linux environment

Steps to compile & run CSEShell:
```bash
## 1. Open your terminal and navigate to the project directory

## 2. Optional: clean previous builds (if necessary) using command `make clean`
make clean

## 3. Compile the shell and all system programs using command `make`
make

## 4. Start the shell using command `./cseshell`
./cseshell
```

## Built-in Functions 

| **Built-in Function**| **Description**                                                                                                                                                |
| -------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cd`                 | Changes the current directory of the shell to the specified path. If no path is given, it defaults to the user's home directory.                               |
| `help`               | Lists all built-in commands in the shell.                                                                                                                      |
| `exit`               | Exits the shell.                                                                                                                                               |
| `usage`              | Provides a brief usage guide for the shell and its built-in commands.                                                                                          |
| `env`                | Lists all the environment variables currently set in the shell.                                                                                                |
| `setenv`             | Sets or modifies an environment variable for this shell session.                                                                                               |
| `unsetenv`           | Removes an environment variable from the shell.                                                                                                                |
| `batman`             | Displays ASCII art of Batman.                                                                                                                                  |
| `cyclops`            | Displays ASCII art of Cyclops.                                                                                                                                 |
| `squidward`          | Displays ASCII art of Squidward.                                                                                                                               |
| `calc`               | Performs basic arithmetic operations (`+`, `-`, `*`, `/`) e.g. `calc 1+1`.                                                                                     |
| `set color_scheme`   | Changes the font color scheme of the prompt. Options: `dark`, `light`, `solarized`, `blue`, `green`, `red`, `default`. <br> *Example*: `set color_scheme=blue` |
| `set show_timestamp` | Toggles display of the timestamp on the prompt. Use `1` to show, `0` to hide. <br> *Example*: `set show_timestamp=1`                                           |
| `set prompt_format`  | Customizes the prompt style. Use `\u$` for username only, `\w$` for working directory. <br> *Example*: `set prompt_format=whateverCustomPromptYouWant`         |
| `history`            | Displays a list of previously entered commands.                                                                                                                |

## Additional Features

**1. Character ASCII Art** 
  - Accessible through commands: `batman`, `cyclops`, `squidward`
  - *Example*: ‘batman’ outputs Batman in ASCII Art 
  - Adds fun and visually engaging elements to the shell, breaking the monotony of plain text interactions
```bash
batman
```

**2. Basic Calculator**
  - Accessible through the `calc` command
  - *Example*: `calc 1+1` outputs 2
  - Be sure to type your equation without any trailing spaces
  - This feature allows users to perform simple arithmetic operations directly within the terminal, including addition, subtraction, multiplication, division.
  - It also includes basic error handling
  - *Example*: Returns `inf` (which stands for infinity) when a number is divided by zero
```bash
calc
```

**3. Command History**
  - Accessible through `history` command
  - Displays a list of previously executed commands in chronological order
  - Each command is numbered, making it easy for users to identify and reference past inputs
  - Acts as a tool to aid in debugging. Identifies execution patterns or troubleshooting issues by reviewing the command sequence
```bash
history
```

**5. Arrow Key Navigation**
  - Pressing the up or down arrow keys cycles through previously entered commands
  - Users can scroll backward to recall older commands or forward to return to more recent ones
  - Reduces need to manually retype commands, improving efficiency and ease of use

## Sustainability 

**1. Resource Usage Feedback --> Resource Display**

What it does:
- After each command is executed, the shell automatically displays detailed statistics including CPU time (user and system), peak memory usage (RAM) and block I/O operations
Justification:
- By making resource consumption transparent to the user, the shell encourages more mindful usage of system resources
- This helps users identify inefficient commands or scripts, reduce unnecessary computational load, and adopt more optimised workflows
- In doing so, the shell not only fosters better programming practices but also aligns with broader sustainability goals by supporting energy-efficient computing

## Inclusivity 

**1. Customizable Interface --> Colour Change**

What it does:
- Allows users to change the color scheme of the shell prompt using the `set colour_scheme` command
- Includes the colours: `dark`, `light`, `solarized`, `blue`, `green`, `red`, `default`
- This functionality is supported through a dedicated data structure that stores and manages color preferences
```bash
set colour_scheme=red
```
Justification:
- To promote inclusivity, the shell includes a customisable prompt color feature that can be activated using the `set colour_scheme` command
- This allows users to select a visual style that best suits their preferences or accessibility needs, such as improved contrast for better visibility or reduced brightness for light sensitivity
- The implementation uses a dedicated data structure to manage color configurations, enabling flexible and consistent prompt styling
- By supporting visual customization, this feature ensures that users with different visual abilities or preferences can interact with the shell more comfortably, fostering a more inclusive and user-friendly experience

**2. Customizable Interface --> Timestamp Display**

What it does:
- The shell includes a timestamp display feature that can be enabled using the `set show_timestamp` command
- When activated, the shell appends the current time to each command prompt
```bash
set show_timestamp=1
```
Justification:
- This provides users with clear temporal context for their interactions
- This is particularly beneficial for users who rely on time cues for task tracking, time-sensitive operations, or cognitive support
- By offering a simple yet impactful customization that accommodates diverse working styles and needs, the feature enhances the shell’s usability for a broader range of users, reinforcing an inclusive and accessible command-line environment

**3. Customizable Interface --> Prompt Style**

What it does:
- The `set prompt_format` command allows users to customize the text that appears at the beginning of each command line (the shell prompt)
- Whatever the user types after `set prompt_format=` becomes the new prompt, replacing the default format
```bash
set prompt_format=WhateverNameIWant
```
Justification:
- The shell supports prompt customization through the set prompt_format command, allowing users to define their own prompt display text
- This feature enables individuals to tailor the shell interface to suit their personal preferences, cultural norms, or accessibility needs (whether for improved readability, reduced cognitive load, or simple self-expression) 
- By giving users full control over how their prompt appears, this functionality ensures a more welcoming and adaptable environment for diverse users, reinforcing the shell’s commitment to personalization and inclusivity

