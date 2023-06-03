#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MATRIX_DIMENSION_XY 10


void set_matrix_elem(float *matrix, int row, int col, float value)
{
    matrix[row * MATRIX_DIMENSION_XY + col] = value;
}


// print a matrix
void quadratic_matrix_print(float *matrix)
{
    for (int i = 0; i < MATRIX_DIMENSION_XY; i++)
    {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++)
        {
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


// TODO:
int quadratic_matrix_compare(float *matrix1, float *matrix2)
{
    for (int i = 0; i < MATRIX_DIMENSION_XY; i++)
    {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++)
        {
            if (matrix1[i * MATRIX_DIMENSION_XY + j] != matrix2[i * MATRIX_DIMENSION_XY + j])
            {
                return 0; // Not equal
            }
        }
    }
    return 1; // Equal
}


void synch(int par_id, int par_count, int *ready, int sync)
{
    // printf("Process %d: Entering synch function, sync = %d\n", par_id, sync);
    ready[par_id] = sync + 1;
    // printf("Process %d: ready array after update:\n", par_id);
    for (int i = 0; i < par_count; i++)
    {
        printf("%d ", ready[i]);
    }
    printf("\n");
    // First synchronization step: wait for all processes to reach this point.
    for (int i = 0; i < par_count; i++)
    {
        while (ready[i] < sync + 1)
        {
            // printf("Process %d: waiting for process %d, ready[%d] = %d, sync = %d\n", par_id, i, i, ready[i], sync);
        }
    }
    // Update own status to indicate completion of waiting.
    ready[par_id] = sync + 2;
    // Second synchronization step: wait for all processes to complete their waiting.
    for (int i = 0; i < par_count; i++)
    {
        while (ready[i] < sync + 2)
        {
            // printf("Process %d: waiting for process %d to finish waiting, ready[%d] = %d, sync = %d\n", par_id, i, i, ready[i], sync);
        }
    }
    // printf("Process %d: Leaving synch function, sync = %d\n", par_id, sync);
}


void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float* A, float* B, float* C, int* ready)
{
    // printf("Process %d: Entering quadratic_matrix_multiplication_parallel function\n", par_id);

    int baseWork = MATRIX_DIMENSION_XY / par_count;
    int remainingWork = MATRIX_DIMENSION_XY % par_count;
    int startColumn = par_id * baseWork + (par_id < remainingWork ? par_id : remainingWork);
    int endColumn = startColumn + baseWork + (par_id < remainingWork ? 1 : 0);

    synch(par_id, par_count, ready, 2);

    for (int a = startColumn; a < endColumn; a++)
    {
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
        {
            for (int c = 0; c < MATRIX_DIMENSION_XY; c++)
            {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
        }
    }

    // printf("Process %d: Leaving quadratic_matrix_multiplication_parallel function\n", par_id);
}


int main(int argc, char *argv[])
{
    int par_id = 0; // the parallel ID of this process
    int par_count = 1; // the amount of processes
    float *A, *B, *C; // matrices A, B, and C
    int *ready; // needed for synch

    if (argc != 3)
    {
        printf("Usage: %s par_id par_count\n", argv[0]);
        return 1;
    }

    par_id = atoi(argv[1]);
    par_count = atoi(argv[2]);

    if (par_count == 1)
    {
        // Initialize matrices A and B.
        A = malloc(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        B = malloc(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        C = malloc(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        for (int i = 0; i < MATRIX_DIMENSION_XY; i++)
        {
            for (int j = 0; j < MATRIX_DIMENSION_XY; j++)
            {
                set_matrix_elem(A, i, j, (float)rand() / RAND_MAX * 10.0);
                set_matrix_elem(B, i, j, (float)rand() / RAND_MAX * 10.0);
            }
        }
        // Perform the matrix multiplication.
        quadratic_matrix_multiplication(A, B, C);
        // Print the result.
        quadratic_matrix_print(C);
        // Test the result.
        float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
        quadratic_matrix_multiplication(A, B, M);
        if (quadratic_matrix_compare(C, M))
        {
            printf("Full points!\n");
        } else
        {
            printf("Bug!\n");
        }
        // Free the matrices.
        free(A);
        free(B);
        free(C);
        return 0;
    }

    int fd[4];


if (par_id == 0)
{
        // shared memory for A, B, C and ready
        fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, 0666);
        fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, 0666);
        fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, 0666);
        fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, 0666);
        if (fd[3] == -1)
        {
            perror("shm_open failed for ready");
            return 1;
        }

        // size of the shared memory
        ftruncate(fd[0], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[1], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[2], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        if (ftruncate(fd[3], par_count * sizeof(int)) == -1)
        {
            perror("ftruncate failed for ready");
            close(fd[3]);
            return 1;
        }

        // mapping shared memory into this process's address space.
        A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        if (A == MAP_FAILED)
        {
            perror("mmap failed for A");
            return 1;
        }

        B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        if (B == MAP_FAILED)
        {
            perror("mmap failed for B");
            return 1;
        }

        C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        if (C == MAP_FAILED)
        {
            perror("mmap failed for C");
            return 1;
        }

        ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
        if (ready == MAP_FAILED)
        {
            perror("mmap failed for ready");
            return 1;
        }

        for (int i = 0; i < par_count; i++)
        {
            ready[i] = 0;
        }

        // initialize  & B matrices
        for (int i = 0; i < MATRIX_DIMENSION_XY; i++)
        {
            for (int j = 0; j < MATRIX_DIMENSION_XY; j++)
            {
                set_matrix_elem(A, i, j, (float)rand() / RAND_MAX * 10.0);
                set_matrix_elem(B, i, j, (float)rand() / RAND_MAX * 10.0);
            }
        }
    } else
    {
        fd[0] = shm_open("matrixA", O_RDWR, 0666);
        fd[1] = shm_open("matrixB", O_RDWR, 0666);
        fd[2] = shm_open("matrixC", O_RDWR, 0666);
        fd[3] = shm_open("synchobject", O_RDWR, 0666);
        if (fd[3] == -1)
        {
        perror("shm_open failed for ready");
        return 1;
        }

    A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    if (A == MAP_FAILED)
    {
        perror("mmap failed for A");
        return 1;
    }

    B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    if (B == MAP_FAILED)
    {
        perror("mmap failed for B");
        return 1;
    }

    C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    if (C == MAP_FAILED)
    {
        perror("mmap failed for C");
        return 1;
    }

    ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    if (ready == MAP_FAILED)
    {
        perror("mmap failed for ready");
        return 1;
    }
}

    // printf("Process %d: Before synch 1\n", par_id);
    synch(par_id, par_count, ready, 1);
    // printf("Process %d: After synch 1\n", par_id);

    // printf("Process %d: Before multiplication\n", par_id);
    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C, ready);
    // printf("Process %d: After multiplication\n", par_id);

    // printf("Process %d: Before synch 3\n", par_id);
    synch(par_id, par_count, ready, 3);
    // printf("Process %d: After synch 3\n", par_id);

    if (par_id == 0)
    {
        quadratic_matrix_print(C);
    }

    // printf("Process %d: Before synch 5\n", par_id);
    synch(par_id, par_count, ready, 5);
    //printf("Process %d: After synch 5\n", par_id);

    // lets test the result:
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    quadratic_matrix_multiplication(A, B, M);
    if (quadratic_matrix_compare(C, M))
    {
        if (par_id == 0)
        {
            printf("Full points!\n");
        }
    } else
    {
        printf("Bug!\n");
    }

    // printf("Process %d: Before synch 7\n", par_id);
    synch(par_id, par_count, ready, 7);
    // printf("Process %d: After synch 7\n", par_id);

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