*volatile* - In the eyes of a C compiler, if the current thread of execution doesn't modify a variable, it’s considered "dead weight" to keep reloading it from slow RAM. The compiler wants to be efficient, so it might cache that value in a CPU register indefinitely.

volatile is your way of telling the compiler: "Your local view of the world is incomplete. Trust me, stay away from the registers and go to the memory bus every single time."

Example:
```c
volatile int *status_reg = (int *) 0x40001000; 

while (*status_reg == 0) {
    // The compiler is now FORCED to check the RAM every time
}

printf("Hardware is ready!\n");
```
