#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <atomic>
#include <array>

using namespace std;


class LamportsBakeryMutex
{
private:
    static constexpr int num_processes = 2;
    std::array<std::atomic<bool>, num_processes> entering;
    std::array<std::atomic<int>, num_processes> number;

public:
    LamportsBakeryMutex()
    {
        for (int i = 0; i < num_processes; i++)
        {
            entering[i] = false;
            number[i] = 0;
        }
    }

    void lock(int id)
    {
        entering[id] = true;
        int max = 0;
        for (int i = 0; i < num_processes; i++)
        {
            max = std::max(max, number[i].load());
        }
        number[id] = max + 1;
        entering[id] = false;

        for (int other_id = 0; other_id < num_processes; ++other_id)
        {
            while (entering[other_id]) { }
            while (number[other_id] != 0 &&
                (number[other_id] < number[id] || (number[other_id] == number[id] && other_id < id))) { }
        }
    }

    void unlock(int id)
    {
        number[id] = 0;
    }
};

const char *text1 = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
const char *text2 = "We look for the truth, but we only want to find it where we like it.\n";

int main()
{
    // shared memory of size 1000
    char *shared_memory = (char*)mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    LamportsBakeryMutex mutex;

    pid_t pid = fork();
    if (pid < 0)
    {
        cerr << "Fork failed." << endl;
        return 1;
    } else if (pid == 0) {
        // child process
        for (int i = 0; i < 1000; i++)
        {
            mutex.lock(0);
            if (i % 2 == 0) {
                strcpy(shared_memory, text1);
                if (i == 0)
                {
                    cout << "Child thread: copied string t1 into shared_memory array: " << text1 << endl;
                }
            } else
            {
                strcpy(shared_memory, text2);
                if (i == 1)
                {
                    cout << "Child thread: copied string t2 into shared_memory array: " << text2 << endl;
                }
            }
            if (i == 0)
            {
                cout << "Current contents of shared_memory array: " << shared_memory << endl;
            }
            mutex.unlock(0);
            usleep(100); // delay
        }
    } else
    {
        // parent process
        usleep(1000 * 1000); // quick delay
        char outtext[1000];
        for (int i = 0; i < 1000; i++) {
            mutex.lock(1);
            strcpy(outtext, shared_memory);
            if (i == 0)
            {
                cout << "Parent thread: copied string from shared_memory array and printing it" << endl;
            }
            if (i == 0)
            {
                cout << outtext << endl;
            }
            mutex.unlock(1);
            usleep(100);
        }
    }

    return 0;
}


/*

g++ -std=c++17 -o lamports ARNE_Lamports_bakery.cpp
./lamports


*/