# Scheduling: The Core Concepts

Scheduling is the OS policy that decides which process to run next when multiple processes are ready. These notes cover Chapters 7, 8, and 9 from *Operating Systems: Three Easy Pieces*.

## 1. Scheduling Metrics
To evaluate a scheduling policy, we look at two primary performance metrics:
- **Turnaround Time**: The time at which the job completes minus the time at which the job arrived in the system ($T_{turnaround} = T_{completion} - T_{arrival}$).
- **Response Time**: The time from when the job arrives to when it is first scheduled ($T_{response} = T_{firstrun} - T_{arrival}$).

### Basic Policies
- **FIFO (First-In, First-Out)**: Simple but suffers from the **Convoy Effect**, where short jobs get stuck behind a long-running job.
- **SJF (Shortest Job First)**: Minimizes average turnaround time by running the shortest job first, but it is non-preemptive.
- **STCF (Shortest Time-to-Completion First)**: A preemptive version of SJF. If a new job arrives with a shorter remaining time than the current job, the OS preempts the current job.
- **Round Robin (RR)**: Instead of running jobs to completion, RR runs a job for a **time slice** (scheduling quantum) and then switches to the next job in the queue. This optimizes **response time** but is terrible for turnaround time.

---

## 2. Multi-Level Feedback Queue (MLFQ) 
The MLFQ is a complex scheduler that aims to optimize both turnaround time (by running short jobs first) and response time (by being interactive). It learns about the behavior of a process as it runs.

### The Five Rules of MLFQ:
1. **Rule 1**: If Priority(A) > Priority(B), A runs (B doesn't).
2. **Rule 2**: If Priority(A) = Priority(B), A & B run in RR.
3. **Rule 3**: When a job enters the system, it is placed at the highest priority.
4. **Rule 4**: Once a job uses up its time allotment at a given level (regardless of how many times it has given up the CPU), its priority is reduced (it moves down one queue).
5. **Rule 5**: After some time period $S$, move all the jobs in the system to the topmost queue (**Priority Boost**). This prevents starvation for long-running CPU-bound jobs.

---

## 3. Proportional Share Scheduling
Instead of optimizing for turnaround or response time, these schedulers try to guarantee that each job gets a certain percentage of CPU time.

### Lottery Scheduling
- **Tickets**: Every process holds a certain number of tickets. The more tickets a process has, the higher its share of the resource.
- **The Lottery**: The OS picks a winning ticket number and runs the process holding that ticket. Over time, the distribution of CPU time matches the distribution of tickets.
- **Ticket Mechanisms**: 
    - **Currency**: Allows a user to allocate tickets among their own jobs in their own "currency."
    - **Transfer**: A process can temporarily hand off tickets to another (useful in client/server models).

### Stride Scheduling
- A deterministic version of lottery scheduling. Each job has a **stride** (inverse of its ticket count). 
- Every time a job runs, its **pass value** is incremented by its stride. 
- The scheduler always picks the job with the lowest pass value to run next.