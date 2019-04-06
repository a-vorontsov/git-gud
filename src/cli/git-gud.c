#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    argv[0] = "git";
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        close(pipefd[0]); // Close read end of child pipe

        dup2(pipefd[1], 2); // Send stderr to pipe

        execvp(argv[0], argv); // Execute git commands
    } else {
        char buffer[1024];

        close(pipefd[1]); // Close read end of parent pipe

        int nbytes = read(pipefd[0], buffer, sizeof(buffer)); // Get size of pipe buffer
        if (nbytes > 0) {
            printf("Err:\n%s\n", buffer);
        }
    }
    return 0;
}
