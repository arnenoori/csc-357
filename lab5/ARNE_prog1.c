#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    const char *shared_memory_name = "ARNE_SharedMemory";
    const int shared_memory_size = 1024;

    int shm_fd = shm_open(shared_memory_name, O_CREAT | O_RDWR, 0666); //0666 permission code for read and write
    ftruncate(shm_fd, shared_memory_size); // size of shared memory

    char *shm_ptr = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    memset(shm_ptr, 0, shared_memory_size); // intiialize shared memory with 0

    while (1) { // checks if theres user input 
        printf("Enter a message (type 'quit' to exit): ");
        char message[shared_memory_size];
        fgets(message, shared_memory_size, stdin);
        message[strcspn(message, "\n")] = 0;

        if (strcmp(message, "quit") == 0) break;
// if user calls for break
        strncpy(shm_ptr, message, shared_memory_size - 1);
    }

    shm_unlink(shared_memory_name); // closes the shared memory
    munmap(shm_ptr, shared_memory_size);
    close(shm_fd);

    return 0;
}
