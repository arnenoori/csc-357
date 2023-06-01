#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incorrect number of arguments. Expected 2 arguments: program name and number of instances.\n");
        return 1;
    }

    char *program_name = argv[1];
    int num_instances = atoi(argv[2]);
    pid_t pid;

    char *args[4];
    args[0] = program_name;
    args[3] = NULL;

    char par_id_str[10];
    char par_count_str[10];

    for (int i = 0; i < num_instances; i++)
    {
        pid = fork();

        if (pid < 0)
        {
            perror("Error during fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // child process

            sprintf(par_id_str, "%d", i);
            sprintf(par_count_str, "%d", num_instances);

            args[1] = par_id_str;
            args[2] = par_count_str;

            if (execv(program_name, args) == -1) 
            {
                fprintf(stderr, "Error during execv: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    // parent process
    for (int i = 0; i < num_instances; i++)
    {
        int status;
        waitpid(-1, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            fprintf(stderr, "Child process exited with error: %d\n", WEXITSTATUS(status));
        }
    }

    return 0;
}


/*
Testing:
    gcc -o p1 ARNE_prog1.c
    gcc -o p2 ARNE_prog2.c

    ./p2 ./p1 4

    Make sure that program works normally
    ./p1 0 1


*/