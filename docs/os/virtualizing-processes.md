# CPU Virtualization

## 1. Virtualization Fundamentals
* **Time Sharing**: The OS runs one process, then another, creating the illusion of many CPUs.
* **Mechanism**: Low-level details of how it time sharing is actually achieved (e.g., Context Switch).
* **Policy**: High-level details of the logic behind what to run (e.g., Scheduler).

## 2. Process Management
* **PCB (Process Control Block)**: The "Object" representing a process in the Kernel.
* **Context**: The saved registers of a stopped process.
* **Process States**:
    - **Running**: Currently on the CPU.
    - **Ready**: Waiting for its turn.
    - **Blocked**: Waiting for I/O.

## 3. Security & Control (LDE)
* **Direct Execution**: Running at hardware speed.
* **Limited**: Using CPU Rings to restrict "User Mode" apps.
* **Traps**: How User Mode asks Kernel Mode for help via System Calls.
* **Timer Interrupt**: The hardware mechanism that prevents a process from hogging the CPU forever.

## 4. The Unix Workflow
* **Fork**: Clones the process.
* **Exec**: Replaces the program.
* **Wait**: Synchronizes Parent with Child.