#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <program> <instance_count>\n", argv[0]);
        return 1;
    }
    const char *program = argv[1];
    int instance_count = atoi(argv[2]);

    pid_t *pids = malloc(sizeof(pid_t) * instance_count);
    if (pids == NULL)
    {
        printf("Error: Unable to allocate memory for process IDs\n");
        return 1;
    }

    for (int i = 0; i < instance_count; i++)
    {
        // fork a new process
        pids[i] = fork();

        if (pids[i] < 0)
        {
            // error handling
            printf("Error: Unable to fork process\n");
            return 1;
        } else if (pids[i] == 0)
        {
            // child process
            char par_id_str[10];
            sprintf(par_id_str, "%d", i);
            char par_count_str[10];
            sprintf(par_count_str, "%d", instance_count);

            char *args[] = {program, par_id_str, par_count_str, NULL};
            execv(program, args);

            printf("Error: Unable to execute program\n");
            return 1;
        }
    }

    // parent process
    for (int i = 0; i < instance_count; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    free(pids);

    printf("All instances completed\n");

    return 0;
}
