#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <cstring>

using namespace std;

const char *text1 = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
const char *text2 = "We look for the truth, but we only want to find it where we like it.\n";

int main()
{
    // shared memory of size 1000
    char *shared_memory = (char*)mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pid_t pid = fork();
    if (pid < 0)
    {
        cerr << "Fork failed." << endl;
        return 1;
    } else if (pid == 0) {
        // child process
        for (int i = 0; i < 1000; i++)
        {
            if (i % 2 == 0) {
                strcpy(shared_memory, text1);
                if (i == 0)
                {
                    printf("Child thread: copied string t1 into shared_memory array: %s\n", text1);
                }
            } else
            {
                strcpy(shared_memory, text2);
                if (i == 1)
                {
                    printf("Child thread: copied string t2 into shared_memory array: %s\n", text2);
                }
            }
            if (i == 0)
            {
                printf("Current contents of shared_memory array: %s\n", shared_memory);
            }
            usleep(100); // delay
        }
    } else
    {
        // parent process
        usleep(1000 * 1000); // quick delay
        char outtext[1000];
        for (int i = 0; i < 1000; i++) {
            strcpy(outtext, shared_memory);
            if (i == 0)
            {
                printf("Parent thread: copied string from shared_memory array and printing it\n");
            }
            if (i == 0)
            {
                printf("%s\n", outtext);
            }
            usleep(100);
        }
    }

    return 0;
}