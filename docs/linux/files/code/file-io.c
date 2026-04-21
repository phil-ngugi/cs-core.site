#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char buffer[20];
    ssize_t bytes_read;

    // Open file (create if it doesn't exist)
    fd = open("example.txt", O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    // Write some data into the file
    write(fd, "Hello Unix IO!\n", 15);

    // Move file offset back to beginning
    lseek(fd, 0, SEEK_SET);

    printf("First read (sequential):\n");

    // Read until EOF
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    if (bytes_read == 0) {
        printf("\nReached EOF (read returned 0)\n");
    }

    // Demonstrate random access using lseek
    printf("\nJumping back to beginning using lseek...\n");
    lseek(fd, 0, SEEK_SET);

    bytes_read = read(fd, buffer, 5); // read first 5 bytes only
    if (bytes_read > 0) {
        printf("First 5 bytes: ");
        write(STDOUT_FILENO, buffer, bytes_read);
        printf("\n");
    }

    close(fd);
    return 0;
}