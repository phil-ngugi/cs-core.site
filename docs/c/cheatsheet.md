# C Programming Cheat Sheet

## 1. Basic Program Structure

```c
#include <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

---

# 2. Basic Data Types

| Type         | Typical Size | Format Specifier |
| ------------ | ------------ | ---------------- |
| char         | 1 byte       | `%c`             |
| int          | 4 bytes      | `%d`             |
| float        | 4 bytes      | `%f`             |
| double       | 8 bytes      | `%lf`            |
| long         | 8 bytes      | `%ld`            |
| short        | 2 bytes      | `%hd`            |
| unsigned int | 4 bytes      | `%u`             |
| pointer      | 4/8 bytes    | `%p`             |

```c
int x = 10;
float y = 3.14f;
char c = 'A';
```

---

# 3. Variables & Constants

```c
const int MAX = 100;
int age = 25;
```

---

# 4. Operators

## Arithmetic

```c
+  -  *  /  %
```

## Comparison

```c
== != > < >= <=
```

## Logical

```c
&& || !
```

## Bitwise

```c
& | ^ ~ << >>
```

## Assignment

```c
= += -= *= /= %=
```

---

# 5. Control Flow

## if / else

```c
if (x > 0) {
    printf("positive");
} else {
    printf("negative");
}
```

## switch

```c
switch (x) {
    case 1:
        break;
    default:
        break;
}
```

## Loops

### for

```c
for (int i = 0; i < 10; i++) {
    printf("%d\n", i);
}
```

### while

```c
while (x > 0) {
    x--;
}
```

### do while

```c
do {
    x--;
} while (x > 0);
```

---

# 6. Functions

```c
int add(int a, int b) {
    return a + b;
}
```

Function prototype:

```c
int add(int a, int b);
```

---

# 7. Arrays

```c
int arr[5] = {1,2,3,4,5};
```

Access:

```c
arr[0]
```

2D array:

```c
int matrix[3][3];
```

---

# 8. Strings

```c
char str[] = "hello";
```

String functions (`string.h`):

| Function           | Purpose        |
| ------------------ | -------------- |
| strlen(s)          | length         |
| strcpy(a,b)        | copy           |
| strcat(a,b)        | concatenate    |
| strcmp(a,b)        | compare        |
| strchr(s,c)        | find char      |
| strstr(a,b)        | find substring |
| memset(ptr,val,n)  | fill memory    |
| memcpy(dest,src,n) | copy memory    |

Example:

```c
#include <string.h>

char a[20] = "Hello";
char b[] = " World";
strcat(a, b);
```

---

# 9. Pointers

## Basics

```c
int x = 10;
int *p = &x;
```

| Expression | Meaning          |
| ---------- | ---------------- |
| x          | value            |
| &x         | address of x     |
| p          | stored address   |
| *p         | value at address |

## Pointer arithmetic

```c
p++;
p--;
```

## Null pointer

```c
int *p = NULL;
```

---

# 10. Dynamic Memory (`stdlib.h`)

## malloc

```c
int *p = malloc(sizeof(int));
```

## calloc

```c
int *arr = calloc(10, sizeof(int));
```

## realloc

```c
arr = realloc(arr, 20 * sizeof(int));
```

## free

```c
free(arr);
```

Always:

```c
if (p == NULL) {
    // allocation failed
}
```

---

# 11. Structures

```c
struct Person {
    char name[50];
    int age;
};
```

Usage:

```c
struct Person p1;
p1.age = 20;
```

Pointer access:

```c
ptr->age
```

---

# 12. typedef

```c
typedef struct Person Person;
```

---

# 13. Enums

```c
enum Color {
    RED,
    GREEN,
    BLUE
};
```

---

# 14. File I/O (`stdio.h`)

## Open file

```c
FILE *fp = fopen("test.txt", "r");
```

Modes:

| Mode | Meaning      |
| ---- | ------------ |
| r    | read         |
| w    | write        |
| a    | append       |
| rb   | binary read  |
| wb   | binary write |

## Read/write

```c
fprintf(fp, "Hello");
fscanf(fp, "%d", &x);
```

## fgets

```c
char buf[100];
fgets(buf, sizeof(buf), fp);
```

## fputs

```c
fputs("Hello", fp);
```

## Binary

```c
fread(ptr, size, count, fp);
fwrite(ptr, size, count, fp);
```

## Close

```c
fclose(fp);
```

---

# 15. Important Headers

| Header    | Purpose             |
| --------- | ------------------- |
| stdio.h   | input/output        |
| stdlib.h  | memory, conversions |
| string.h  | strings/memory      |
| math.h    | math                |
| ctype.h   | character checks    |
| time.h    | time/date           |
| stdbool.h | bool                |
| stdint.h  | fixed-size ints     |
| assert.h  | debugging           |
| errno.h   | errors              |

---

# 16. Common `stdio.h` Functions

| Function | Purpose         |
| -------- | --------------- |
| printf   | print formatted |
| scanf    | input           |
| getchar  | read char       |
| putchar  | print char      |
| fgets    | read line       |
| puts     | print line      |
| sprintf  | write to string |
| sscanf   | parse string    |

---

# 17. Common `stdlib.h` Functions

| Function | Purpose                |
| -------- | ---------------------- |
| malloc   | allocate memory        |
| calloc   | allocate zeroed memory |
| realloc  | resize memory          |
| free     | free memory            |
| atoi     | string to int          |
| atof     | string to float        |
| exit     | terminate program      |
| rand     | random number          |
| srand    | seed RNG               |
| qsort    | sorting                |
| abs      | absolute value         |

Example:

```c
srand(time(NULL));
int n = rand() % 100;
```

---

# 18. Common `math.h` Functions

| Function    | Purpose        |
| ----------- | -------------- |
| sqrt        | square root    |
| pow         | exponent       |
| sin cos tan | trig           |
| log         | natural log    |
| fabs        | absolute float |
| floor       | round down     |
| ceil        | round up       |

Compile with:

```bash
gcc main.c -lm
```

---

# 19. Common `ctype.h` Functions

| Function | Purpose          |
| -------- | ---------------- |
| isdigit  | digit check      |
| isalpha  | alphabetic check |
| isalnum  | alphanumeric     |
| toupper  | uppercase        |
| tolower  | lowercase        |
| isspace  | whitespace check |

---

# 20. Boolean (`stdbool.h`)

```c
#include <stdbool.h>

bool ok = true;
```

---

# 21. Fixed Width Integers (`stdint.h`)

| Type     | Meaning         |
| -------- | --------------- |
| int8_t   | 8-bit signed    |
| uint8_t  | 8-bit unsigned  |
| int32_t  | 32-bit signed   |
| uint64_t | 64-bit unsigned |

---

# 22. Macros

```c
#define PI 3.14159
#define SQUARE(x) ((x)*(x))
```

---

# 23. Command Line Arguments

```c
int main(int argc, char *argv[]) {
    printf("%s", argv[1]);
}
```

---

# 24. Useful `printf` Specifiers

| Specifier | Meaning      |
| --------- | ------------ |
| %d        | int          |
| %u        | unsigned int |
| %f        | float        |
| %lf       | double       |
| %c        | char         |
| %s        | string       |
| %p        | pointer      |
| %x        | hexadecimal  |
| %%        | literal %    |

---

# 25. Useful Escape Sequences

| Escape | Meaning      |
| ------ | ------------ |
| \n     | newline      |
| \t     | tab          |
| \      | backslash    |
| "      | double quote |
| '      | single quote |

---

# 26. Common GCC Commands

## Compile

```bash
gcc main.c -o app
```

## Run

```bash
./app
```

## Warnings

```bash
gcc -Wall -Wextra main.c
```

## Debugging symbols

```bash
gcc -g main.c
```

---

# 27. Common Bugs

## Uninitialized pointer

```c
int *p;
*p = 10; // BAD
```

## Memory leak

```c
malloc(...);
// forgot free
```

## Buffer overflow

```c
char s[5];
strcpy(s, "toolong");
```

## Dangling pointer

```c
free(p);
*p = 10; // BAD
```

---

# 28. Memory Layout of a Process

```text
+----------------+
|     Stack      |
+----------------+
|      Heap      |
+----------------+
| Global/Data    |
+----------------+
| Program Code   |
+----------------+
```

---

# 29. Bitwise Cheatsheet

```c
x & y   // AND
x | y   // OR
x ^ y   // XOR
~x      // NOT
x << 1  // left shift
x >> 1  // right shift
```

---

# 30. Quick Pointer Examples

## Pointer to pointer

```c
int x = 5;
int *p = &x;
int **pp = &p;
```

## Array + pointer

```c
int arr[3] = {1,2,3};
int *p = arr;
```

## Function pointer

```c
int add(int a, int b) {
    return a+b;
}

int (*fp)(int,int) = add;
```

---

# 31. Frequently Used Patterns

## Swap using pointers

```c
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
```

## Dynamic array

```c
int *arr = malloc(n * sizeof(int));
```

## Iterate string

```c
for (int i = 0; str[i] != '\0'; i++) {
}
```

---

# 32. Compilation Model

```text
Source Code (.c)
    ↓
Preprocessor
    ↓
Compiler
    ↓
Assembler
    ↓
Linker
    ↓
Executable
```

---

# 33. Useful Debugging

## gdb

```bash
gdb ./app
```

## valgrind

```bash
valgrind ./app
```

---

# 34. ASCII Basics

```c
'A' = 65
'a' = 97
'0' = 48
```

---

# 35. Common Interview Topics

* pointers
* memory management
* linked lists
* stacks/queues
* recursion
* bitwise operations
* structs
* file I/O
* undefined behavior
* arrays vs pointers

---

# 36. Core Rules To Remember

1. Every variable has an address.
2. Pointers store addresses.
3. `*p` accesses value at address.
4. Arrays decay to pointers.
5. Always free allocated memory.
6. Never dereference NULL.
7. Buffer overflows are dangerous.
8. Undefined behavior means anything can happen.
9. `%s` expects `char *`.
10. `%p` prints addresses.

---

# 37. Mini Standard Library Reference

## String conversion

```c
atoi("123");
atof("3.14");
strtol(str, NULL, 10);
```

## Search & sort

```c
qsort(arr, n, sizeof(int), cmp);
bsearch(...);
```

## Memory

```c
memcpy(dest, src, n);
memmove(dest, src, n);
memcmp(a, b, n);
```

## Error handling

```c
perror("Error");
strerror(errno);
```

---

# 38. Learning Roadmap

1. Syntax
2. Functions
3. Arrays & strings
4. Pointers
5. Memory allocation
6. Structs
7. File I/O
8. Data structures
9. Systems programming
10. Multithreading

---

# 39. Recommended Build Flags

```bash
gcc -Wall -Wextra -Werror -pedantic -g main.c -o app
```

---

# 40. Golden Rule

> C gives you direct memory control.
> With power comes responsibility.
