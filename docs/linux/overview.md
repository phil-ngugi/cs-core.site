# Some Overview

This document provides a technical overview of how the Linux Kernel manages processes, memory, and system resources.

## 1. Kernel Threads & Processes
The kernel manages execution through threads. While standard processes run in user space, **Kernel Threads** are special entities that look like processes but reside and operate entirely within **Kernel Space**.
* **Examples:** `kthreadd` (the parent of all kernel threads) and `kblockd` (manages block I/O).
* **Purpose:** These threads handle background system tasks like memory management, disk synchronization, and interrupt handling without needing a user-space environment.

## 2. Main Memory (RAM)
The RAM is the "Warehouse" of the computer, acting as the primary storage for all volatile data.
* **Residence:** All running processes, the kernel itself, and various buffers reside here.
* **I/O Flow:** All data from peripheral devices (keyboard, disk, network) flows through RAM before being processed by the CPU.
* **CPU Interaction:** The CPU acts as an operator that strictly reads instructions/data from memory and writes results back to memory.

## 3. Context Switching & Scheduling
To provide the illusion of multitasking, the kernel performs **Preemptive Context Switching**.
1.  **Timer Interrupt:** A hardware timer expires, triggering a hardware interrupt that forces the CPU to switch from User Mode to Kernel Mode.
2.  **State Saving:** The kernel saves the process "Context" (Registers, Program Counter, Stack Pointer) into the **Process Control Block (PCB)**.
3.  **Scheduling:** The kernel selects the next process and sets a new **timeslice** (quantum).
4.  **Restore & Switch:** The kernel loads the new process's state, flips the CPU mode bit back to **User Space**, and hands over control.

## 4. System Calls (Syscalls)
Syscalls are the "Software Interrupts" that allow user programs to request services from the kernel.
* `fork()`: Creates a near-exact clone of the current process.
* `exec(program)`: Replaces the current process's memory space with a new program.
* **The Init Ancestry:** Except for the `init` process (PID 1), every process on a Linux system is a result of a `fork()`.
* **Pseudo-devices:** The kernel provides interfaces like `/dev/random` (blocking, high entropy) and `/dev/urandom` (non-blocking, algorithmic) to expose kernel-level entropy to user-space programs.

## 5. The Shell & Streams
The Shell is a **User Space** program that acts as the interface between the human and the Kernel.
* **Standard Streams:** Processes communicate via 1D sequences of bytes called streams:
    * `stdin` (0): Input (Keyboard/File)
    * `stdout` (1): Output (Terminal/File)
    * `stderr` (2): Error messages.
* **Example:** Running `cat` without arguments connects `stdin` to your terminal. The kernel buffers your keystrokes until you hit enter, at which point `cat` reads the stream and writes it back to `stdout`.

## 6. Essential Linux Commands
| Command | Description |
| :--- | :--- |
| `diff f1 f2` | Compare two files for differences. |
| `pwd -P` | Show the physical path (ignores symbolic links). |
| `file <name>` | Determine the file type/format. |
| `find dir -name x` | Search for files by name in a directory. |
| `kill <pid>` | Send a termination signal to a process. |
| `kill -STOP <pid>` | Freeze a process (suspend execution). |
| `kill -CONT <pid>` | Resume a frozen process. |
| `command &` | Run a process in the background. |

## 7. Shell Navigation Shortcuts (Readline)
| Shortcut | Action |
| :--- | :--- |
| **CTRL-B** | Move cursor left. |
| **CTRL-F** | Move cursor right. |
| **CTRL-P** | Previous command in history. |
| **CTRL-N** | Next command in history. |
| **CTRL-A** | Move cursor to beginning of line. |
| **CTRL-E** | Move cursor to end of line. |
| **CTRL-W** | Erase preceding word. |
| **CTRL-U** | Erase from cursor to beginning of line. |
| **CTRL-K** | Erase from cursor to end of line. |
| **CTRL-Y** | Paste (Yank) erased text. |