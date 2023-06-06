#include <iostream>
#include <atomic>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

const char *t1 = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
const char *t2 = "We look for the truth, but we only want to find it where we like it.\n";

int main()
{
    // shared memory of size 1000
    char *shared_memory = (char*)mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pid_t pid = fork();

    if(pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return 1;
    }

    if(pid == 0) { // child process
        strcpy(shared_memory, t1);
        printf("Child thread: copied string t1 into shared_memory array: %s\n", t1);
        usleep(500000); 
        printf("Current contents of shared_memory array: %s\n", shared_memory);
        sleep(1);

        strcpy(shared_memory, t2);
        printf("Child thread: copied string t2 into shared_memory array: %s\n", t2);
    }
    else { // parent process
        sleep(2);

        char outtext[1000];
        strcpy(outtext, shared_memory);
        printf("Parent thread: copied string from shared_memory array and printing it\n");
        printf("Current contents of outtext array: %s\n", outtext);
    }

    return 0;
}
