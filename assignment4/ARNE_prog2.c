#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

const int SHM_SIZE = 1024;
const char* SHM_NAME = "ARNE_SharedMemory";

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Incorrect number of arguments. Expected 2 arguments: program name and number of instances.\n");
        return 1;
    }

    char *program_name = argv[1];
    int num_instances = atoi(argv[2]);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) != 0) {
        perror("ftruncate");
        return 1;
    }

    int *shared_counter = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_counter == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    *shared_counter = 0;

    pid_t pid;

    char *args[5]; // Increase the size to 5 to accommodate the new argument
    args[0] = program_name;
    args[4] = NULL;

    char par_id_str[10];
    char par_count_str[10]; // New variable to hold the number of instances as a string
    char shm_fd_str[10];
    char shm_name_str[SHM_SIZE];

    sprintf(par_count_str, "%d", num_instances); // Convert the number of instances to a string
    sprintf(shm_fd_str, "%d", shm_fd);

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
            sprintf(shm_name_str, "%s", SHM_NAME);

            args[1] = par_id_str;
            args[2] = par_count_str; // Replace shm_name_str with par_count_str
            args[3] = shm_fd_str;

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

    printf("Final counter value: %d\n", *shared_counter);

    if (munmap(shared_counter, SHM_SIZE) == -1) {
        perror("munmap");
        return 1;
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
        return 1;
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