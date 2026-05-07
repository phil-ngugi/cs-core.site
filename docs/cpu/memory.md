# How the CPU and Memory Work Together

Understanding computer architecture is easier if you visualize the CPU as a reader and memory as a massive library of numbered bookshelves. To move information between them, the computer uses a specialized communication system known as the **System Bus**.

---

## 1. The System Bus: The Three Highways
The CPU connects to memory components through three distinct sets of electrical wires, each with a specific job.

### A. The Address Bus (Unidirectional)
* **The "Where":** This bus is used by the CPU to specify exactly which memory location it wants to access.
* **Direction:** It is **unidirectional**. Information only flows **from the CPU to the Memory**.
* **Example:** If the CPU needs data from address 322, it sets the Address Bus wires to the binary equivalent of 322. The memory chips "listen" to this bus to see if they are being called.

### B. The Control Bus (Signaling)
* **The "What":** This bus carries signals that tell the memory what operation to perform.
* **Key Signals:** The most common are **RD (Read)** and **WR (Write)**.
* **Active Low:** These wires are often "active low," meaning the CPU pulls the voltage to 0 to trigger the command.

### C. The Data Bus (Bidirectional)
* **The "Information":** This bus carries the actual data being transferred.
* **Direction:** It is **bidirectional**. The CPU reads data from memory and writes data back to it.
* **Bus Width:** This is what defines a "64-bit" or "32-bit" computer. A 64-bit data bus can move 64 bits of information in a single cycle, making it much faster than an 8-bit bus.

---

## 2. Address Decoding: The Traffic Controller
Since RAM, ROM, and other devices are all connected to the same buses, there must be a way to prevent them from all talking at once (a data collision).

* **The Range:** Each physical chip is assigned a specific range of addresses (e.g., ROM might be addresses 0–32,767).
* **The Decoder:** This is a logic circuit that monitors the Address Bus. When it detects an address belonging to a specific chip, it sends a **Chip Select (CS)** or **Chip Enable (CE)** signal to that chip.
* **Z-State:** If a chip is not selected, its pins enter a "High Impedance" (Z-state), effectively disconnecting itself from the bus so it doesn't interfere with other chips.

---

## 3. Types of Memory
Memory is organized both physically (hardware) and logically (how the CPU sees it).

### Physical Memory
| Type | Full Name | Behavior | Purpose |
| :--- | :--- | :--- | :--- |
| **ROM** | Read-Only Memory | Non-volatile (Persistent) | Holds "Firmware" (BIOS) needed to boot the PC. |
| **RAM** | Random Access Memory | Volatile (Erased on power off) | Temporary storage for running apps and data. |
| **Registers** | N/A | Inside the CPU | Instant-access storage for immediate calculations. |

### Logical Memory Mapping
The CPU views all memory as one continuous **Address Space**. 
* **Firmware Area:** Usually mapped to Address 0 so the CPU knows exactly where to start reading instructions the moment it is powered on.
* **Video RAM (VRAM):** A specific logical range where data corresponds to pixels on your screen. Writing to these addresses changes the display.
* **I/O Ports:** In many systems, hardware like keyboards or LEDs are "memory-mapped," meaning the CPU treats them as if they were just another memory address.

---

## Summary of a "Read" Cycle
1.  **Address:** CPU puts the address (e.g., 322) on the **Address Bus**.
2.  **Decode:** The **Address Decoder** enables the specific chip that owns that address.
3.  **Command:** CPU pulls the **RD** line low on the **Control Bus**.
4.  **Transfer:** The enabled chip places the data from that address onto the **Data Bus**.
5.  **Receive:** The CPU reads the data from the Data Bus and stores it in an internal register.

---
*Source: [How do computers work? CPU, ROM, RAM, address bus, data bus, control bus, address decoding.](https://youtu.be/4knBXkN1GEU)*