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

    int shm_fd = shm_open(shared_memory_name, O_RDWR, 0666); // opens the shared memory

    if (shm_fd == -1) {
        perror("Failed to open shared memory");
        exit(1);
    }

    char *shm_ptr = mmap(0, shared_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    while (1) {
        if (strlen(shm_ptr) > 0) { // check if shared memory contains
            printf("Received message: %s\n", shm_ptr);
            break;
        }
        sleep(1);
    }

    munmap(shm_ptr, shared_memory_size);
    close(shm_fd);

    return 0;
}
