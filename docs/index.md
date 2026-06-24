# Roadmap: Understand Computers & Software

**Goal:** Write and understand any type of software without relying on abstractions, and explain in depth how each computer component works.

**How to use this:** The reading is scaffolding — the *projects are the actual learning*. Skipping labs to read faster feels like progress but isn't. Time estimates assume serious part-time study (~10–15 hrs/week); they're for sequencing and expectation-setting, not deadlines.

---

## The Path

| Stage | Focus | Primary resource(s) | Project that proves mastery | Rough time |
|-------|-------|---------------------|------------------------------|------------|
| 0 | Transistors & physical switching | Ben Eater logic videos; Intel "Transistor Explained"; TechSpot series | Build an AND/NAND gate from individual transistors (breadboard or simulator) and explain why it switches | 1–2 weeks |
| 1 | Digital logic → working CPU | Nand2Tetris projects 1–5; Ben Eater 8-bit computer | Your CPU runs a program; explain every wire during one fetch-decode-execute, including clock edge and reset line | 6–10 weeks |
| 2 | Software side of that machine | Nand2Tetris projects 6–12 (assembler, VM, compiler, OS) | Working assembler + small compiler targeting your own CPU; trace high-level code down to gates | 4–6 weeks |
| 3 | C & how real code meets hardware | K&R; CS:APP (do all labs) | Predict a C function's assembly, stack frame, and cache behavior; write your own memory allocator (Malloc Lab) | 3–4 months |
| 4 | Real assembly & architecture | x86-64 / ARM64 / RISC-V; Patterson & Hennessy "Computer Organization and Design" | Write non-trivial assembly by hand; explain pipelining, hazards, memory hierarchy without notes | 2–3 months |
| 5 | Operating systems | OSTEP (free); OSDev wiki (optional) | Write a shell + thread library/scheduler; ideally boot a minimal kernel handling one interrupt; explain power-on to running program | 3–4 months |
| 6 | Compilers & linkers | "Crafting Interpreters" (free); Dragon Book (depth) | Write an interpreter and a compiler emitting real assembly/bytecode; understand linking and loading | 2–3 months |
| 7 | Advanced microarchitecture & real components | Hennessy & Patterson "Quantitative Approach"; Intel SDM / ARM ARM; DDR/PCIe/NVMe datasheets; OSDev | Read a memory/storage controller datasheet and explain its operation; explain speculative execution and Spectre-class bugs | Ongoing |
| 8 | Breadth into domains | Domain-specific (networking, databases, graphics, concurrency) | Build one real artifact per domain (e.g., TCP stack, storage engine) | Lifelong |

---

## Stage Detail

### Stage 0 — Transistors & physical switching (~1–2 weeks)
Understand a MOSFET as a voltage-controlled switch and how two of them make a gate. **Do not** go deeper into semiconductor physics — it doesn't serve this goal. Know what a transistor does, then move up.

### Stage 1 — Digital logic to a working CPU (~6–10 weeks)
Where "hardware is magic" dies. Build gates → flip-flops → registers → ALU → memory → CPU, and watch the clock and reset behave as real signals.

### Stage 2 — Software side of the same machine (~4–6 weeks)
Cross the hardware/software seam with your own hands: assembler, virtual machine, compiler, basic OS, all targeting the CPU you built.

### Stage 3 — C & how real code meets hardware (~3–4 months)
**The single most important stage for the software goal.** CS:APP sits exactly on the hardware/software seam: how C becomes assembly, how the stack and memory work, how linking/loading happen, how cache affects speed, how the OS presents processes and virtual memory. Do the labs (Data, Bomb, Attack, Cache, Malloc, Shell).

### Stage 4 — Real assembly & principled architecture (~2–3 months)
Pick one real ISA and learn it properly. Patterson & Hennessy gives the principled foundation (uses MIPS — clean teaching architecture, principles transfer everywhere).

### Stage 5 — Operating systems (~3–4 months)
Where "how software actually runs" fully resolves: processes, threads, scheduling, virtual memory, system calls, file systems, drivers. This ties back to the power-on/reset/Power-Good sequence.

### Stage 6 — Compilers & linkers (~2–3 months)
Translation from source to machine code stops being a black box. Start gentle (Crafting Interpreters), go deep if you want (Dragon Book).

### Stage 7 — Advanced microarchitecture & real component integration (ongoing)
Modern out-of-order, speculative, multicore CPUs; then the concrete controllers, buses, and boot sequencing that live in datasheets and vendor manuals — too vendor-specific and fast-changing for textbooks. By now you'll have the vocabulary to read them directly.

### Stage 8 — Breadth into domains (lifelong)
With the spine solid, domains attach cleanly because you can finally see through their abstractions. Pick what's relevant to the software you want to write.

## Resource Quick Reference

- **Nand2Tetris** — nand2tetris.org (free course + Coursera videos; book: *The Elements of Computing Systems*)
- **Ben Eater** — youtube.com/@BenEater (8-bit breadboard computer; 6502 series)
- **CS:APP** — *Computer Systems: A Programmer's Perspective*, Bryant & O'Hallaron
- **K&R** — *The C Programming Language*, Kernighan & Ritchie
- **P&H** — *Computer Organization and Design*, Patterson & Hennessy
- **OSTEP** — *Operating Systems: Three Easy Pieces* (free at ostep.org)
- **OSDev** — wiki.osdev.org
- **Crafting Interpreters** — craftinginterpreters.com (free)
- **Dragon Book** — *Compilers: Principles, Techniques, and Tools*
- **Quantitative Approach** — *Computer Architecture: A Quantitative Approach*, Hennessy & Patterson
- **Vendor manuals** — Intel SDM, ARM Architecture Reference Manual; DDR/PCIe/NVMe datasheets