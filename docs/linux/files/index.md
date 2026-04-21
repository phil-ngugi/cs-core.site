# Unix File / I/O Model

- Everything is treated as a **byte stream**
- Same syscalls for I/O:
  - `read()`, `write()`, `open()`, `close()`
- Works for:
  - files, devices, pipes, sockets
- Default is **sequential access**
- `lseek()` enables random access (for seekable files only)
- EOF = `read()` returns `0` (no special character)

---

# File Descriptors (FDs)

- FD = **integer handle to an open I/O resource**
- Each process has its own **FD table**
- FD → file description → actual resource

## File description stores:
- file offset
- flags (e.g. `O_RDONLY`)
- reference to resource

- Multiple FDs can share same file description (`dup`, `fork`)

## Fd Example
```c
{% include-markdown "./code/file-descriptors.c" %}
```

---

## Standard FDs

- `0` → stdin  
- `1` → stdout  
- `2` → stderr  

---

## Key points

- Kernel returns **lowest available FD**
- Max FDs per process: `ulimit -n`
- Not closing FDs ⇒ **FD leaks**

---

## Summary

- FDs = **handles for all I/O**
- Enable uniform access to files, sockets, pipes, devices