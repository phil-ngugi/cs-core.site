# Undefined Behavior in C

Sometimes code violates the **C standard**, but the compiler does not have to do anything about it (e.g., return an error). Since the C standard does not define what happens, the program may behave weirdly:

* **Work perfectly:** But hide a latent bug.
* **Program crash:** A segmentation fault caused by attempting to access restricted memory, unallocated memory, or null pointers.
* **Code Deletion:** During optimization, the compiler might delete segments of code it deems unreachable or redundant based on UB assumptions.
* **Erratic Hardware:** The hardware might behave unpredictably.

---

## Example: Signed Integer Overflow

```c
#include <stdio.h>
#include <limits.h>

int main() {
    int x = INT_MAX;

    // Potential Undefined Behavior
    if (x + 1 > x) {
        printf("The world makes sense.\n");
    } else {
        printf("Overflow occurred!\n");
    }

    return 0;
}
```

A modern smart compiler might reason that since algebraically $x + 1$ will always be greater than $x$, it can remove the `else` block entirely. However, during **runtime**, if the CPU actually attempts to add $1$ to $x$ and causes an overflow, the `else` block no longer exists to handle it, making the behavior unpredictable.

---

## Why This Exists: Performance

C prioritizes **performance**. Instead of the standard requiring extra CPU instructions to handle every edge case, it assumes the programmer knows the hardware limits.

---

## Types of UB

1.  **Dereferencing NULL:** The compiler may optimize away surrounding code assuming the pointer is always valid.
2.  **Out of Bounds Array:** Accessing indices outside the allocated memory of an array.
3.  **Use-After-Free:** Attempting to use memory that has already been released back to the system.