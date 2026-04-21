// print and inspect file descriptors
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd1 = open("file1.txt", O_CREAT | O_WRONLY, 0644);
    int fd2 = open("file2.txt", O_CREAT | O_WRONLY, 0644);

    printf("fd1 = %d\n", fd1);
    printf("fd2 = %d\n", fd2);

    pause(); // keep process alive so we can inspect

    close(fd1);
    close(fd2);
    return 0;
}