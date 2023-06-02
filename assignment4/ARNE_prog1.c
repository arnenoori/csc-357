#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MATRIX_DIMENSION_XY 3

void set_matrix_elem(float *matrix, int row, int col, float value) {
    matrix[row * MATRIX_DIMENSION_XY + col] = value;
}

void quadratic_matrix_print(float *matrix) {
    for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
            printf("%.2f ", matrix[i * MATRIX_DIMENSION_XY + j]);
        }
        printf("\n");
    }
}

// multiply two matrices
void quadratic_matrix_multiplication(float *A,float *B,float *C)
{
	//nullify the result matrix first
for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
    for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
        C[a + b*MATRIX_DIMENSION_XY] = 0.0;
//multiply
for(int a = 0;a<MATRIX_DIMENSION_XY;a++) // over all cols a
    for(int b = 0;b<MATRIX_DIMENSION_XY;b++) // over all rows b
        for(int c = 0;c<MATRIX_DIMENSION_XY;c++) // over all rows/cols left
            {
                C[a + b*MATRIX_DIMENSION_XY] += A[c + b*MATRIX_DIMENSION_XY] * B[a + c*MATRIX_DIMENSION_XY]; 
            }
}

int quadratic_matrix_compare(float *matrix1, float *matrix2) {
    for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
            if (matrix1[i * MATRIX_DIMENSION_XY + j] != matrix2[i * MATRIX_DIMENSION_XY + j]) {
                return 0; // Not equal
            }
        }
    }
    return 1; // Equal
}


void synch(int par_id, int par_count, int *ready, int sync)
{
    ready[par_id] = sync;

    if (par_id != 0) {
        // Wait until the previous process has reached the synchronization point
        while (ready[par_id - 1] < sync) {
            usleep(1000);
        }
    }

    if (par_id != par_count - 1) {
        // Wait until the next process has reached the synchronization point
        while (ready[par_id + 1] < sync) {
            usleep(1000);
        }
    }
}


void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float* A, float* B, float* C, int* ready)
{
    int splitWork = MATRIX_DIMENSION_XY / par_count;
    int startCol = par_id * splitWork;
    int endCol;
    if (par_id == par_count - 1)
    {
        endCol = MATRIX_DIMENSION_XY;
    }
    else
    {
        endCol = startCol + splitWork ;
    }

    // Wait for all processes to complete the initialization of matrix C
    synch(par_id, par_count, ready, 2);

    // Multiply matrices
    for (int a = startCol; a < endCol; a++) // over all cols a
    {
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++) // over all rows b
        {
            for (int c = 0; c < MATRIX_DIMENSION_XY; c++) // over all rows/cols left
            {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
        }
    }
}


int main(int argc, char *argv[]) {
    int par_id = 0; // the parallel ID of this process
    int par_count = 1; // the amount of processes
    float *A, *B, *C; // matrices A, B, and C
    int *ready; // needed for synch

    if (argc != 3) {
        printf("Usage: %s par_id par_count\n", argv[0]);
        return 1;
    }

    par_id = atoi(argv[1]);
    par_count = atoi(argv[2]);

    if (par_count == 1) {
        printf("Only one process\n");
        return 0;
    }

    int fd[4];

    synch(par_id, par_count, ready, 0);

    if (par_id == 0) {
        // Create the shared memory for A, B, C, ready.
        fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, 0666);
        fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, 0666);
        fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, 0666);
        fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, 0666);

        // Set the size of the shared memory.
        ftruncate(fd[0], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[1], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[2], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[3], par_count * sizeof(int));

        // Map the shared memory into this process's address space.
        A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);

        // Initialize the synchronization array.
        for (int i = 0; i < par_count; i++) {
            ready[i] = 0;
        }

        // Initialize matrices A and B.
        for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
            for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
                set_matrix_elem(A, i, j, (float)rand() / RAND_MAX * 10.0);
                set_matrix_elem(B, i, j, (float)rand() / RAND_MAX * 10.0);
            }
        }
    } else {
        // Open the shared memory for A, B, C, ready.
        fd[0] = shm_open("matrixA", O_RDWR, 0666);
        fd[1] = shm_open("matrixB", O_RDWR, 0666);
        fd[2] = shm_open("matrixC", O_RDWR, 0666);
        fd[3] = shm_open("synchobject", O_RDWR, 0666);

        // Map the shared memory into this process's address space.
        A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    }

    synch(par_id, par_count, ready, 1);

    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C, ready);

    synch(par_id, par_count, ready, 2);

    if (par_id == 0) {
        quadratic_matrix_print(C);
    }

    synch(par_id, par_count, ready, 3);

    // Test the result
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    quadratic_matrix_multiplication(A, B, M);
    if (quadratic_matrix_compare(C, M)) {
        printf("Full points!\n");
    } else {
        printf("Bug!\n");
    }

    synch(par_id, par_count, ready, 4);

    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    shm_unlink("matrixA");
    shm_unlink("matrixB");
    shm_unlink("matrixC");
    shm_unlink("synchobject");

    return 0;
}
