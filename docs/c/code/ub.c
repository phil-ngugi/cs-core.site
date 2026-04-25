#include <stdio.h>
#include <limits.h>

int main() {
    int i = INT_MAX; // The largest possible value for an int

    // This is Undefined Behavior!
    if (i + 1 > i) {
        printf("The world makes sense.\n");
    } else {
        printf("Overflow occurred!\n");
    }

    return 0;
}