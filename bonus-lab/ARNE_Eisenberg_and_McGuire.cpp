#include <iostream>
#include <atomic>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

enum State {idle, want_in, in_cs};

class EisenbergMcGuireMutex
{
public:
    EisenbergMcGuireMutex(int num_threads) : last(0), states(num_threads) {
        for(auto &state : states) {
            state.store(static_cast<int>(idle));
        }
    }

    void lock(int id) {
        printf("Mutex activated by thread: %d\n", id);
        states[id].store(static_cast<int>(want_in));
        int index = last.load();
        while (index != id) {
            if (states[index].load() != idle) {
                index = last.load();
            } else {
                index = (index + 1) % states.size();
            }
        }
        states[id].store(static_cast<int>(in_cs));
        for (index = 0; index < states.size(); ++index) {
            if ((index != id) && (states[index].load() == in_cs)) {
                break;
            }
        }
        if (index >= states.size()) {
            last.store(id);
        } else {
            lock(id);
        }
    }

    void unlock(int id) {
        states[id].store(static_cast<int>(idle));
        printf("Mutex deactivated by thread: %d\n", id);
    }

private:
    std::atomic<int> last;
    std::vector<std::atomic<int>> states;
};

const char *t1 = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
const char *t2 = "We look for the truth, but we only want to find it where we like it.\n";

int main()
{
    // shared memory of size 1000
    char *shared_memory = (char*)mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    EisenbergMcGuireMutex mutex(2); // parent & child threads

    pid_t pid = fork();

    if(pid < 0) {
        fprintf(stderr, "Fork failed\n");
        return 1;
    }

    if(pid == 0) { // child process
        mutex.lock(0);
        strcpy(shared_memory, t1);
        printf("Child thread: copied string t1 into shared_memory array: %s\n", t1);
        printf("Current contents of shared_memory array: %s\n", shared_memory);
        mutex.unlock(0);
        sleep(1);

        mutex.lock(0);
        strcpy(shared_memory, t2);
        printf("Child thread: copied string t2 into shared_memory array: %s\n", t2);
        mutex.unlock(0);
    }
    else { // parent process
        sleep(2);

        mutex.lock(1);
        char outtext[1000];
        strcpy(outtext, shared_memory);
        printf("Parent thread: copied string from shared_memory array and printing it\n");
        printf("Current contents of outtext array: %s\n", outtext);
        mutex.unlock(1);
    }

    return 0;
}



/*
g++ -o eisenberg ARNE_Eisenberg_&_McGuire.cpp
./eisenberg
*/