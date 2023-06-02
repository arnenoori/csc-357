#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include <errno.h>

const int MATRIX_DIMENSION_XY = 10;

//SEARCH FOR TODO

//************************************************************************************************************************
// sets one element of the matrix
void set_matrix_elem(float *M, int x, int y, float f)
{
    M[x + y * MATRIX_DIMENSION_XY] = f;
}

//************************************************************************************************************************

// lets see if both matrices are the same
int quadratic_matrix_compare(float *A, float *B)
{
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++) {
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++) {
            if (A[a + b * MATRIX_DIMENSION_XY] != B[a + b * MATRIX_DIMENSION_XY])
                return 0;
        }
    }
    return 1;
}

//************************************************************************************************************************

// print a matrix
void quadratic_matrix_print(float *C)
{
    printf("\n");
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++) {
        printf("\n");
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            printf("%.2f,", C[a + b * MATRIX_DIMENSION_XY]);
    }
    printf("\n");
}

//************************************************************************************************************************

// multiply two matrices
void quadratic_matrix_multiplication(float *A, float *B, float *C)
{
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++) {
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++) {
            C[a + b * MATRIX_DIMENSION_XY] = 0.0;
            for (int c = 0; c < MATRIX_DIMENSION_XY; c++) {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
        }
    }
}

//************************************************************************************************************************

void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float *A, float *B, float *C)
{
    for (int i = par_id; i < MATRIX_DIMENSION_XY; i += par_count) {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
            C[i * MATRIX_DIMENSION_XY + j] = 0;
            for (int k = 0; k < MATRIX_DIMENSION_XY; k++) {
                C[i * MATRIX_DIMENSION_XY + j] += A[i * MATRIX_DIMENSION_XY + k] * B[k * MATRIX_DIMENSION_XY + j];
            }
        }
    }
}

// Function to synchronize the processes
void synch(int par_id, int par_count)
{
    char filename[] = "synchobject";
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // Seek to the position based on par_id
    off_t offset = par_id * sizeof(int);
    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        exit(1);
    }

    // Read the current value
    int value;
    if (read(fd, &value, sizeof(int)) < 0) {
        perror("read");
        close(fd);
        exit(1);
    }

    // Increment the value and write it back
    value++;
    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        exit(1);
    }
    if (write(fd, &value, sizeof(int)) < 0) {
        perror("write");
        close(fd);
        exit(1);
    }

    close(fd);

    // Synchronization point: Wait until all processes have reached this point
    while (1) {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(1);
        }

        // Check if all values are equal to par_count
        int count = 0;
        for (int i = 0; i < par_count; i++) {
            if (read(fd, &value, sizeof(int)) < 0) {
                perror("read");
                close(fd);
                exit(1);
            }
            if (value == par_count) {
                count++;
            }
        }
        close(fd);

        // If all values are equal to par_count, break the loop
        if (count == par_count) {
            break;
        }

        // Sleep for a short duration before checking again
        usleep(100);
    }
}

//************************************************************************************************************************


int create_shared_memory(const char *name, size_t size) {
    int fd = shm_open(name, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("shm_open");
        exit(1);
    }
    
    if (ftruncate(fd, size) < 0) {
        perror("ftruncate");
        exit(1);
    }
    
    return fd;
}

int main(int argc, char *argv[]) {
    int par_id = 0;
    int par_count = 1;
    float *A, *B, *C;
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];

    if (argc != 3) {
        printf("Usage: %s <parent id> <parent count>\n", argv[0]);
        return -1;
    } else {
        par_id = atoi(argv[1]);
        par_count = atoi(argv[2]);
    }

    // Open or create shared memory objects
    int fdA = create_shared_memory("matrixA", MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    int fdB = create_shared_memory("matrixB", MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    int fdC = create_shared_memory("matrixC", MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));

    // Map the shared memory objects into the process's address space
    A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fdA, 0);
    B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fdB, 0);
    C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fdC, 0);

    // Initialize the matrices if par_id is 0
    if (par_id == 0) {
        for (int a = 0; a < MATRIX_DIMENSION_XY; a++) {
            for (int b = 0; b < MATRIX_DIMENSION_XY; b++) {
                set_matrix_elem(A, a, b, a * b * 0.1 + 0.1);
                set_matrix_elem(B, a, b, a * b * 0.1 + 0.2);
            }
        }
    }

    // Synchronization point: wait until all processes have initialized their matrices
    synch(par_id, par_count);

    // Perform matrix multiplication in parallel
    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C);

    // Synchronization point: wait until all processes have completed their calculations
    synch(par_id, par_count);

    // Only the process with par_id == 0 should print the matrix and verify the result
    if (par_id == 0) {
        quadratic_matrix_print(C);

        quadratic_matrix_multiplication(A, B, M);
        if (quadratic_matrix_compare(C, M)) {
            printf("full points!\n");
        } else {
            printf("bug!\n");
        }
    }

    // Clean up
    munmap(A, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    munmap(B, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    munmap(C, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    close(fdA);
    close(fdB);
    close(fdC);

    // Unlink shared memory objects if par_id is 0
    if (par_id == 0) {
        shm_unlink("matrixA");
        shm_unlink("matrixB");
        shm_unlink("matrixC");
    }

    return 0;
}



/*
Ideally. I want my code to output:
par_id: 0, par_count: 4
correct matrix
time for that matrix calculation
"full points!"

par_id: 1, par_count: 4
correct matrix
time for that matrix calculation
"full points!"

par_id: 2, par_count: 4
correct matrix
time for that matrix calculation
"full points!"

par_id: 3, par_count: 4
correct matrix
time for that matrix calculation
"full points!"

gcc -o p1 ARNE_prog1.c -lpthread
gcc -o p2 ARNE_prog2.c -lpthread
./p2 ./p1 4


*/