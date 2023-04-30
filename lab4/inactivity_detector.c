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
    int main_pipe[2];
    pipe(main_pipe); // create pipe before the fork
    fcntl(main_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(main_pipe[1], F_SETFL, O_NONBLOCK);

    pid_t pid = fork();
    if (pid == 0) { // child process
        close(main_pipe[1]); // close write end of the pipe

        while (1) {
            sleep(10); // child becomes inactive for 10 seconds

            char buffer[2];
            int bytesRead = read(main_pipe[0], buffer, 1);
            if (bytesRead <= 0) {
                printf("Inactivity detected!\n");
            }
        }
    } else if (pid > 0) { // parent process
        close(main_pipe[0]); // close read end of the pipe
        signal(SIGUSR1, activity_detected);
        char text[256];

        while (1) {
            fgets(text, sizeof(text), stdin);
            text[strcspn(text, "\n")] = 0; // remove the newline character

            if (strcmp(text, "quit") == 0) { // quit functionality
                kill(pid, SIGTERM);
                wait(NULL);
                break;
            }
            update_text(text); // updates the output
            write(main_pipe[1], "1", 1);
        }
    } else {
        perror("fork");
        exit(1);
    }

    return 0;
}