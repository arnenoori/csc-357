#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MATRIX_DIMENSION_XY 10

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s program_path instance_count\n", argv[0]);
        return 1;
    }

    char *program_path = argv[1];
    int instance_count = atoi(argv[2]);
    pid_t *pids = malloc(instance_count * sizeof(pid_t));

    for (int i = 0; i < instance_count; i++)
    {
        pids[i] = fork();
        
        if (pids[i] < 0)
        {
            perror("Failed to fork");
            return 1;
        }

        if (pids[i] == 0)
        {
            // child process

            // printf("Child: This is process with ID %d\n", i);
            char par_id_str[10];
            char par_count_str[10];
            
            sprintf(par_id_str, "%d", i);
            sprintf(par_count_str, "%d", instance_count);

            char *args[] = {program_path, par_id_str, par_count_str, NULL};
            execv(program_path, args);
            perror("Failed to execv");
            exit(1);
        }
    }

    // wait for all children to complete
    for (int i = 0; i < instance_count; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    free(pids);

    return 0;
}
