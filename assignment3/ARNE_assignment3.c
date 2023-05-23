#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_CHILDREN 10

// holds the child process information
typedef struct
{
    pid_t pid;
    int pipefd[2];
    bool terminated;
} ChildProcess;

ChildProcess children[MAX_CHILDREN];
int child_count = 0;


void find_file(char *filename, char *startdirectory, int search_in_all_subdirectories, int pipefd)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];
    bool file_found = false;

    if (!(dir = opendir(startdirectory)))
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            if (search_in_all_subdirectories)
            {
                snprintf(path, sizeof(path), "%s/%s", startdirectory, entry->d_name);
                find_file(filename, path, search_in_all_subdirectories, pipefd);
            }
        }
        else
        {
            if (strcmp(entry->d_name, filename) == 0)
            {
                snprintf(path, sizeof(path), "%s/%s", startdirectory, entry->d_name);
                dprintf(pipefd, "Found file: %s\n", path);
                file_found = true;
            }
        }
    }
    closedir(dir);

    if (!file_found && !search_in_all_subdirectories)
        dprintf(pipefd, "File not found.\n");
}


void handle_child(int signal)
{
    pid_t pid;
    int status;

    // wait for any child process
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // find child in the children array and remove it
        for (int i = 0; i < child_count; i++)
        {
            if (children[i].pid == pid)
            {
                // set terminated flag to true
                children[i].terminated = true;

                // remove child from the array by moving all subsequent elements one step to the left
                for (int j = i; j < child_count - 1; j++)
                {
                    children[j] = children[j + 1];
                }

                // decrease the child count
                child_count--;
                break;
            }
        }
    }
}


void spawn_child(char *filename, char *startdirectory, int search_in_all_subdirectories)
{
    if (child_count >= MAX_CHILDREN)
    {
        printf("Maximum number of child processes reached.\n");
        return;
    }

    pid_t pid;
    int pipefd[2] = {-1, -1};

    // create the pipe
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return;
    }

    // create the child process
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        close(pipefd[0]); // closes pipe if fork fails
        close(pipefd[1]);
        return;
    }

    if (pid == 0)
    {
        // child process close read end of the pipe
        close(pipefd[0]);

        // change startdirectory to root directory if -f flag is specified
        if (search_in_all_subdirectories)
            strcpy(startdirectory, "/");

        find_file(filename, startdirectory, search_in_all_subdirectories, pipefd[1]);
        close(pipefd[1]); // close write end of the pipe
        exit(0);
    }
    else
    {
        // parent process
        close(pipefd[1]); // close write end of the pipe
        children[child_count].pid = pid;
        children[child_count].pipefd[0] = pipefd[0];
        children[child_count].pipefd[1] = pipefd[1];
        children[child_count].terminated = false; // initialize terminated flag
        child_count++;
    }
}


int process_command(char *command)
{
    char *filename;
    char *flag;
    char *token;
    int search_in_all_subdirectories = 0;
    char startdirectory[1024];

    // get the current working directory
    if (getcwd(startdirectory, sizeof(startdirectory)) == NULL)
    {
        perror("getcwd");
        return 0;
    }

    // parse the command
    token = strtok(command, " ");
    if (token == NULL)
    {
        printf("Invalid command.\n");
        return 0;
    }

    if (strcmp(token, "quit") == 0 || strcmp(token, "q") == 0)
    {
        // terminate all child processes
        for (int i = 0; i < child_count; i++)
        {
            kill(children[i].pid, SIGTERM);
        }
        return -1;
    }

    if (strcmp(token, "find") != 0)
    {
        printf("Invalid command.\n");
        return 0;
    }

    // get the filename
    token = strtok(NULL, " ");
    if (token == NULL)
    {
        printf("Missing filename.\n");
        return 0;
    }
    filename = token;

    // get the flag
    token = strtok(NULL, " ");
    if (token != NULL)
    {
        if (strcmp(token, "-s") == 0)
        {
            search_in_all_subdirectories = 1;
        }
        else if (strcmp(token, "-f") == 0)
        {
            strcpy(startdirectory, "/");
            search_in_all_subdirectories = 1;
        }
        else
        {
            printf("Invalid flag.\n");
            return 0;
        }
    }

    // spawn child process
    spawn_child(filename, startdirectory, search_in_all_subdirectories);

    return 0;
}


int main()
{
    struct sigaction sa;
    sa.sa_handler = &handle_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, 0) == -1)
    {
        perror(0);
        exit(1);
    }

    usleep(1000);

    // main loop
    char command[256];
    while (1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);

        // add standard input to the set
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = STDIN_FILENO;

        // add pipes to the set
        for (int i = 0; i < child_count; i++)
        {
            if (children[i].pipefd[0] != -1)
            {
                FD_SET(children[i].pipefd[0], &readfds);
                if (children[i].pipefd[0] > maxfd)
                {
                    maxfd = children[i].pipefd[0];
                }
            }
        }

        int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ret == -1)
        {
            perror("select");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            // print the command prompt
            printf("findstuff$ ");
            fflush(stdout);

            // handle user input
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = '\0';
            if (process_command(command) == -1)
            {
                break;
            }
        }
        else
        {
            // handle child process message
            for (int i = 0; i < child_count; i++)
            {
                if (children[i].pipefd[0] != -1 && FD_ISSET(children[i].pipefd[0], &readfds)) // ensure the file descriptor is not -1
                {
                    // read and print the message
                    char buffer[1024];
                    ssize_t len = read(children[i].pipefd[0], buffer, sizeof(buffer) - 1);
                    if (len > 0)
                    {
                        buffer[len] = '\0';
                        printf("%s\n", buffer);
                    }
                }
            }
        }
    }

    return 0;
}

/*

Running it:

gcc ARNE_assignment3.c -o findfile
./findfile

Testing: 

Within folder:
find assignment3.txt

Testing file not found:
find jar.jpg

Testing subfolder:

find jar.jpg -s

All directories:
find test.c -f

q

*/