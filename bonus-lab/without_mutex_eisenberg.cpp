#include <iostream>
#include <thread>
#include <cstring>

char text[1000];
char t1[] = "The wiser gives in! A sad truth, it established the world domination of stupidity.\n";
char t2[] = "We look for the truth, but we only want to find it where we like it.\n";
char outtext[1000];

void child_thread()
{
    strcpy(text, t1);
    std::cout << "Child thread: copied string t1 into text array: " << t1 << std::endl;
    std::cout << "Current contents of text array: " << text << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));

    strcpy(text, t2);
    std::cout << "Child thread: copied string t2 into text array: " << t2 << std::endl;
}


void parent_thread()
{
    std::this_thread::sleep_for(std::chrono::seconds(2));

    strcpy(outtext, text);
    std::cout << "Parent thread: copied string from text array and printing it" << std::endl;
    std::cout << "Current contents of outtext array: " << outtext << std::endl;
}


int main()
{
    std::thread child(child_thread);
    std::thread parent(parent_thread);

    child.join();
    parent.join();

    return 0;
}
