# IO Devices
- Consider a hypthetical(typical device). We shall explore what it takes to make such a device efficient.
- Such a device may within itself contain hardware for use within the device itself and software that it presents for system software for it to be usable outside of itself.
- Some devices may even contain a simple CPU, RAM and registers within itself
Example: A device may contain  a status (to store the current status of the device), data(to pass or get data from the device) and command register(to tell the device to perform a specific action). An os can read or write from these registers thereby controlling the device's behavior.
- A device internal structure is transparent to the system
- Hypothetically, the CPU can poll the status and perform actions, but this may be inefficient (CPU waiting for the device to finish). This can be solved using hardware interrupts, freeing the OS to context switch to another process, and jumping to the predetermined OS *interrupt handler* or *interrupt service routing (ISR)*
- 
## Files
A file is basically a linear array of bytes, each containing a low-level name(called inode number) not visible to users while a directory (also has an inode number and a human readable name) contains a list of entries that either refer to files or other directories.
Using open() on a file returns a descriptor which then lets you use the normal methods to interact with whatever was returned,
You can use strace command to track what syscalls a process makes, arguments and return codes. e.g ```strace cat foo.txt``` 