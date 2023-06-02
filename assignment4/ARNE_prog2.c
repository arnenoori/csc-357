#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s program_path instance_count\n", argv[0]);
        return 1;
    }

    char *program_path = argv[1];
    int instance_count = atoi(argv[2]);

    for (int i = 0; i < instance_count; i++) {
        pid_t pid = fork();
        printf("Parent: Forked process with ID %d\n", pid);
        
        if (pid < 0) {
            perror("Failed to fork");
            return 1;
        }

        if (pid == 0) {
            // This is the child process.
            printf("Child: This is process with ID %d\n", i);
            char par_id_str[10];
            char par_count_str[10];
            sprintf(par_id_str, "%d", i);
            sprintf(par_count_str, "%d", instance_count);

            char *args[] = {program_path, par_id_str, par_count_str, NULL};

            if (execv(program_path, args) == -1) {
                perror("Failed to execv");
                return 1;
            }
        }
    }

    // Wait for all child processes to complete.
    for (int i = 0; i < instance_count; i++) {
        wait(NULL);
    }

    return 0;
}
