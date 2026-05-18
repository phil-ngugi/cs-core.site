# Concurrency: The Challenge of Multiple Threads
## 1. The Abstraction: The Thread 
- **Definition**: A thread is similar to a process but shares the same address space (code, heap, and static data). Each thread has its own **Program Counter (PC)** and **Stack**.
- **Context Switching**: Like processes, threads require a context switch. The OS saves the state to a **Thread Control Block (TCB)**.
- **The Problem: Uncontrolled Scheduling**: When multiple threads access shared data, the result depends on which thread runs when. This leads to:
    - **Race Condition**: Multiple threads entering a critical section at the same time.
    - **Indeterminacy**: The output of the program is not consistent across runs.
    - **Mutual Exclusion**: The requirement that only one thread can execute a critical section at a time.

---

## 2. Locks
Locks are used to provide mutual exclusion. A programmer wraps a critical section with `lock()` and `unlock()`.

### Hardware Support for Locks
Building efficient locks requires hardware primitives:
- **Test-and-Set**: Atomically writes a 1 to memory and returns the old value.
- **Compare-and-Swap**: Updates memory only if the current value matches an expected value.
- **Load-Linked / Store-Conditional**: A pair of instructions used to build lock-free updates.

### Spin Locks vs. Sleeping
- **Spin Locks**: The thread busy-waits (loops) until the lock is available. This wastes CPU cycles.
- **Sleeping/Yielding**: Using OS support (like Solaris `park()` or Linux `futex`), a thread can put itself to sleep and be woken up when the lock is free, saving CPU.

---

## 3. Condition Variables 
Sometimes a thread needs to wait for a specific *condition* to be true before proceeding (e.g., a parent waiting for a child).

- **Definition**: An explicit queue that threads can put themselves on when some state is not as desired.
- **Wait & Signal**: 
    - `wait()`: Puts the thread to sleep and **releases the lock** atomically.
    - `signal()`: Wakes up one waiting thread.
- **The Rule**: Always use a `while` loop to check the condition after waking up, not an `if` statement (Mesa semantics).

---

## 4. Semaphores
Introduced by Dijkstra, a semaphore is an integer value that can be used for both locks and condition variables.

- **sem_wait()**: Decrements the value. If the value is negative, the thread waits.
- **sem_post()**: Increments the value and wakes a waiting thread.
- **Binary Semaphore**: A semaphore initialized to 1 (acts as a Lock).
- **Classic Problems**:
    - **Producer/Consumer**: Coordinating buffer access between producers and consumers.
    - **Reader-Writer Locks**: Allowing multiple readers but exclusive writers.
    - **Dining Philosophers**: A classic deadlock illustration.

---

## 5. Common Concurrency Bugs
- **Non-Deadlock Bugs**: 
    - *Atomicity Violations*: A code region intended to be atomic is interrupted.
    - *Order Violations*: Thread A assumes Thread B has already finished a task.
- **Deadlock**: A cycle where Thread 1 holds Lock A and waits for B, while Thread 2 holds Lock B and waits for A.
    - **Four Necessary Conditions**: 
        1. Mutual Exclusion
        2. Hold-and-wait
        3. No preemption
        4. Circular wait