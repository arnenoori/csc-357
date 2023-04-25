#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

void update_text(char *text) {
    printf("!%s!\n", text);
}

void activity_detected(int sig) {
}

int main() {
    int comm_pipe[2];
    pipe(comm_pipe);
    fcntl(comm_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(comm_pipe[1], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid == 0) { // Child process
        close(comm_pipe[1]); // Close write end of the pipe

        while (1) {
            sleep(10);

            char buffer[2];
            int bytesRead = read(comm_pipe[0], buffer, 1);
            if (bytesRead <= 0) {
                printf("Inactivity detected!\n");
            }
        }
    } else if (pid > 0) { // Parent process
        close(comm_pipe[0]); // Close read end of the pipe

        signal(SIGUSR1, activity_detected);

        char text[256];

        while (1) {
            fgets(text, sizeof(text), stdin);
            text[strcspn(text, "\n")] = 0; // Remove the newline character

            if (strcmp(text, "quit") == 0) {
                kill(pid, SIGTERM);
                wait(NULL);
                break;
            }

            update_text(text);
            write(comm_pipe[1], "1", 1);
        }
    } else {
        perror("fork");
        exit(1);
    }

    return 0;
}
