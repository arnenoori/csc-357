#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <program> <count>\n", argv[0]);
        return -1;
    }

    char *program = argv[1];
    int count = atoi(argv[2]);

    for (int i = 0; i < count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            char par_id_str[10];
            char par_count_str[10];

            sprintf(par_id_str, "%d", i);
            sprintf(par_count_str, "%d", count);

            char *args[4];
            args[0] = program;
            args[1] = par_id_str;
            args[2] = par_count_str;
            args[3] = NULL;

            setbuf(stdout, NULL);  // Disable stdout buffering

            execv(program, args);

            // If execv returns, there was an error
            perror("execv");
            return -1;
        } else if (pid < 0) {
            // Error forking
            perror("fork");
            return -1;
        }
    }

    // Parent process waits for all child processes to finish
    while (wait(NULL) > 0);

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