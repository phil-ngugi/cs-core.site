## Virtualizing CPU
Time sharing allows concurrent processes by giving them the illusion of them owning the CPU
To achieve CPU virtualization we need:
 - Mechanisms - low level methods & protocols for implementing some functionality e.g context switch
 - Policies - e.g scheduling policy - the algorithms for making decisions in an OS

**Processes** are merely OS abstractions of running programs

OS Data structures - The Os, being a program as any other, must maintain some data structures e.g the **process context**, which stores the content of stopped process registers for when they resume. This structure that stores info about processes is also called  Process Control Block (PCB).

Process APis:

exec()  does not create a new process; rather, it transforms the currently running program into a different running program, and reinitializes the process's memory.
using wait() in the parent process will make it possible for the parent to continue after an exec() in the forked child process, and this is precisely how shells work. they exec() and wait() until your program is done running, then they show the prompt.
fork(): You split your program into two.

exec() (in the Child): The child transforms into the new application (e.g., ls). The child's original code is replaced.

wait() (in the Parent): The parent process pauses. It doesn't die; it just sits there waiting for a signal.

 limited - direct execution