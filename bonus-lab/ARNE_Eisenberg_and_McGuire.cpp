#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>

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
        std::cout << "Mutex activated by thread: " << id << std::endl;
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
        std::cout << "Mutex deactivated by thread: " << id << std::endl;
    }

private:
    std::atomic<int> last;
    std::vector<std::atomic<int>> states;
};

char text[1000];
char t1[] = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
char t2[] = "We look for the truth, but we only want to find it where we like it.\n";
char outtext[1000];

void child_thread(EisenbergMcGuireMutex* mutex)
{
    mutex->lock(0);
    strcpy(text, t1);
    std::cout << "Child thread: copied string t1 into text array: " << t1 << std::endl;
    std::cout << "Current contents of text array: " << text << std::endl;

    mutex->unlock(0);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    mutex->lock(0);
    strcpy(text, t2);
    std::cout << "Child thread: copied string t2 into text array: " << t2 << std::endl;
    mutex->unlock(0);
}


void parent_thread(EisenbergMcGuireMutex* mutex)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));

    mutex->lock(1);
    strcpy(outtext, text);
    std::cout << "Parent thread: copied string from text array and printing it" << std::endl;
    std::cout << "Current contents of outtext array: " << outtext << std::endl;
    mutex->unlock(1);
}


int main()
{
    EisenbergMcGuireMutex mutex(2); // parent & child threads

    std::thread child(child_thread, &mutex);
    std::thread parent(parent_thread, &mutex);

    child.join();
    parent.join();

    return 0;
}


/*
g++ -o eisenberg ARNE_Eisenberg_&_McGuire.cpp
./eisenberg
*/