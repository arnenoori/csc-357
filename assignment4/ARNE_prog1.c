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

const int MATRIX_DIMENSION_XY = 10;
const size_t SHM_SIZE = 1024;

//SEARCH FOR TODO

//************************************************************************************************************************
// sets one element of the matrix
void set_matrix_elem(float *M,int x,int y,float f)
{
M[x + y*MATRIX_DIMENSION_XY] = f;
}

//************************************************************************************************************************

// lets see it both are the same
int quadratic_matrix_compare(float *A,float *B)
{
for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
    for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
       if(A[a +b * MATRIX_DIMENSION_XY]!=B[a +b * MATRIX_DIMENSION_XY]) 
        return 0;
   
return 1;
}

//************************************************************************************************************************

//print a matrix
void quadratic_matrix_print(float *C)
{
    printf("\n");
for(int a = 0;a<MATRIX_DIMENSION_XY;a++)
    {
    printf("\n");
    for(int b = 0;b<MATRIX_DIMENSION_XY;b++)
        printf("%.2f,",C[a + b* MATRIX_DIMENSION_XY]);
    }
printf("\n");
}

//************************************************************************************************************************

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

//************************************************************************************************************************

void quadratic_matrix_multiplication_parallel(int par_id, int par_count, float *A, float *B, float *C) {
    for (int i = par_id; i < MATRIX_DIMENSION_XY; i += par_count) {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
            C[i * MATRIX_DIMENSION_XY + j] = 0;
            for (int k = 0; k < MATRIX_DIMENSION_XY; k++) {
                C[i * MATRIX_DIMENSION_XY + j] += A[i * MATRIX_DIMENSION_XY + k] * B[k * MATRIX_DIMENSION_XY + j];
            }
        }
    }
}

void synch(int par_id, int par_count, int *ready, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    ready[par_id] = 1;
    pthread_mutex_unlock(mutex);

    for (int i = 0; i < par_count; i++) {
        while (!ready[i]) {
            // Not all processes are ready, so wait a bit before checking again
            usleep(100);
        }
    }
}


//************************************************************************************************************************

int main(int argc, char *argv[]) {
    int par_id = 0;
    int par_count = 1;
    float *A, *B, *C;
    int *ready;
    int *shared_counter;
    

if(argc!=3) { 
    printf("Usage: %s <parent id> <parent count>\n", argv[0]); 
    return -1;
} else {
    par_id = atoi(argv[1]);
    par_count = atoi(argv[2]);
    
    // Prepare shared memory segment name
    char shm_name[64];
    sprintf(shm_name, "ARNE_SharedMemory%d", par_id);

    // Create the shared memory segment
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0600);
    if (shm_fd < 0) {
        perror("shm_open");
        return -1;
    }
    ftruncate(shm_fd, sizeof(int));

    // Map the shared memory segment in the address space of the process
    shared_counter = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_counter == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
}

pthread_mutex_t *mutex;
int fd[5];

if(par_id==0) {
    // Create shared memory segments in process with id 0
    fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    fd[4] = shm_open("mutex", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    // Resize shared memory segments
    ftruncate(fd[0], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    ftruncate(fd[1], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    ftruncate(fd[2], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
    ftruncate(fd[3], par_count * sizeof(int));
    ftruncate(fd[4], sizeof(pthread_mutex_t));

    // Map shared memory segments into address space
    A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd[4], 0);

    // Initialize mutex for process-shared synchronization
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mutex, &attr);

    // Initialize ready array
    for (int i = 0; i < par_count; i++) {
        ready[i] = 0;
    }
} else {
    // All other processes only open already created shared memory segments
    fd[0] = shm_open("matrixA", O_RDWR, S_IRUSR | S_IWUSR);
    fd[1] = shm_open("matrixB", O_RDWR, S_IRUSR | S_IWUSR);
    fd[2] = shm_open("matrixC", O_RDWR, S_IRUSR | S_IWUSR);
    fd[3] = shm_open("synchobject", O_RDWR, S_IRUSR | S_IWUSR);
    fd[4] = shm_open("mutex", O_RDWR, S_IRUSR | S_IWUSR);

    // Map shared memory segments into address space
    A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd[4], 0);

    // Sleep to ensure the shared memory is initialized
    sleep(2);
}

    for (int i = 0; i < par_count; i++) {
        ready[i] = 0;
    }

    synch(par_id, par_count, ready, mutex);

    if (par_id == 0) {
        for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
            for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
                A[i * MATRIX_DIMENSION_XY + j] = (i + 1) * (j + 1);
                B[i * MATRIX_DIMENSION_XY + j] = (i + 1) + (j + 1);
            }
        }
    }

    synch(par_id, par_count, ready, mutex);

    quadratic_matrix_multiplication_parallel(par_id, par_count, A, B, C);

    synch(par_id, par_count, ready, mutex);

    if (par_id == 0) {
        quadratic_matrix_print(C);
        pthread_mutex_destroy(mutex);
        shm_unlink("mutex");
    }
    
    synch(par_id, par_count, ready, mutex);
    
        pthread_mutex_lock(mutex);
    ++(*shared_counter);
    pthread_mutex_unlock(mutex);

    printf("Counter value: %d\n", *shared_counter);
    // Test the result
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    quadratic_matrix_multiplication(A, B, M);

    if (quadratic_matrix_compare(C, M)) {
        printf("full points!\n");
    } else {
        printf("buuug!\n");
    }

    printf("par_id: %d, par_count: %d\n", par_id, par_count);

    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    close(fd[4]);

    if (par_id == 0) {
        shm_unlink("matrixA");
        shm_unlink("matrixB");
        shm_unlink("matrixC");
        shm_unlink("synchobject");
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