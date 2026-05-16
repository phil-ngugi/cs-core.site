# Virtualizing Memory

## Memory Types in C
- **Stack**: Managed implicitly by the compiler. Used for local variables and function call frames. Allocation and deallocation are automatic (LIFO).
- **Heap**: Managed explicitly by the programmer (e.g., via `malloc()` and `free()`). Used for data that needs to persist beyond the scope of a single function.
- **malloc()**: If it succeeds, it returns a pointer to the newly allocated space; otherwise, it returns `NULL`.

## Dynamic (Hardware-based) Relocation
- **Base and Bounds Mechanism**: The OS places the starting physical address in a **Base register**. All virtual addresses are added to this base to find the physical address.
- **Hardware Structures**: Base and bounds registers are part of the **MMU (Memory Management Unit)** on the CPU.
- **Bounds Register**: Stores the size limit of the address space to ensure a process cannot access memory outside its allocated range (protection).
- **Free List**: An OS data structure used to track which parts of physical memory are available for allocation.
- **Internal Fragmentation**: Wasted space within an allocated block (e.g., the unused gap between the stack and the heap in a single base/bounds system).

---

## Segmentation
Segmentation solves the problem of internal fragmentation by allowing the address space to be broken into logical segments.

### 1. The Concept
- Instead of one base/bounds pair for the entire process, the MMU has a **pair of base and bounds registers per logical segment** (typically Code, Stack, and Heap).
- This allows the OS to place segments in different parts of physical memory, avoiding the need to allocate the "hole" between the stack and heap.

### 2. Address Translation
- **Segment Selector**: The top bits of the virtual address identify which segment is being accessed (e.g., 00 for Code, 01 for Heap, 10 for Stack).
- **Offset**: The remaining bits represent the offset within that segment.
- **Calculation**: `Physical Address = Base + Offset`.
- **Protection**: If `Offset >= Bounds`, the hardware triggers a **segmentation fault**.

### 3. Advanced Features
- **Backward-Growing Segments**: The Stack grows towards lower addresses. The hardware needs a "grow positive" bit (0 or 1) to correctly calculate offsets for the stack.
- **Support for Sharing**: Protection bits (Read, Write, Execute) allow the OS to share code segments between processes while maintaining security.
- **Fine-grained vs. Coarse-grained**: Coarse-grained uses few segments (Code, Heap, Stack). Fine-grained uses many small segments, often requiring a **Segment Table**.

### 4. New Problems
- **External Fragmentation**: Physical memory becomes a series of little holes of free space because segments are variable-sized.
- **Compaction**: The OS may need to stop running processes and rearrange memory to create a contiguous block of free space, which is very expensive.

---

## Paging 
Paging divides the address space into fixed-sized units to eliminate external fragmentation.

### 1. Basic Mechanism 
- **Pages and Frames**: Virtual memory is divided into **Pages**; physical memory is divided into **Page Frames**.
- **Page Table**: A per-process data structure that stores the mapping of Virtual Page Numbers (VPN) to Physical Frame Numbers (PFN).
- **PTE (Page Table Entry)**: Contains the PFN plus status bits:
    - **Valid Bit**: Is the page in the address space?
    - **Protection Bits**: R/W/X permissions.
    - **Present Bit**: Is the page in RAM or on disk (swapped)?
    - **Dirty Bit**: Has the page been modified?

### 2. Fast Translation with TLBs
- **TLB (Translation Lookaside Buffer)**: A hardware cache of recently used address translations.
- **TLB Hit**: Translation happens at hardware speed without accessing memory.
- **TLB Miss**: The hardware (or OS) must look up the translation in the Page Table, which is much slower.
- **ASID (Address Space Identifier)**: A tag in the TLB that allows entries from different processes to coexist without flushing the cache on every context switch.

### 3. Efficient Page Tables
- **Multi-level Page Tables**: The most common solution to large page tables. It uses a **Page Directory** to point to page table pages. If a whole region of memory is unused, the corresponding page table pages aren't allocated.
- **Inverted Page Tables**: Instead of one table per process, one table tracks which process owns each physical frame.