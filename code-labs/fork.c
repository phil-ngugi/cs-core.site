pid_t pid = fork();
if (pid == 0) {
    execve("/bin/ls", NULL, NULL);
}