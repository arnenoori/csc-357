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
#define MATRIX_DIMENSION_XY 10

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

void synch(int par_id, int par_count, int *ready, pthread_mutex_t *mutex)
{
    pthread_mutex_lock(mutex);
    ready[par_id] = 1;
    pthread_mutex_unlock(mutex);

    int all_ready = 0;
    while (!all_ready) {
        all_ready = 1;
        for (int i = 0; i < par_count; i++)
        {
            if (!ready[i])
            {
                all_ready = 0;
                break;
            }
        }
        if (!all_ready) {
            // Not all processes are ready, so wait a bit before checking again
            sleep(1);
        }
    }
}




//************************************************************************************************************************


int main(int argc, char *argv[])
{
int par_id = 0; // the parallel ID of this process
int par_count = 1; // the amount of processes
float *A,*B,*C; //matrices A,B and C
int *ready; //needed for synch
if(argc!=3){printf("no shared\n");}
else
    {
    par_id= atoi(argv[1]);
    par_count= atoi(argv[2]);
    }
if(par_count==1){printf("only one process\n");}


pthread_mutex_t *mutex;
int fd[5];
if(par_id==0)
    {
        fd[0] = shm_open("matrixA", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fd[1] = shm_open("matrixB", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fd[2] = shm_open("matrixC", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fd[3] = shm_open("synchobject", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fd[4] = shm_open("mutex", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

        ftruncate(fd[0], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[1], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[2], MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float));
        ftruncate(fd[3], par_count * sizeof(int));
        ftruncate(fd[4], sizeof(pthread_mutex_t));
        

        A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
        mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd[4], 0);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(mutex, &attr);
    }
else
    {
	//TODO: init the shared memory for A,B,C, ready. shm_open withOUT C_CREAT here! NO ftruncate! but yes to mmap
    fd[0] = shm_open("matrixA", O_RDWR, S_IRUSR | S_IWUSR);
    fd[1] = shm_open("matrixB", O_RDWR, S_IRUSR | S_IWUSR);
    fd[2] = shm_open("matrixC", O_RDWR, S_IRUSR | S_IWUSR);
    fd[3] = shm_open("synchobject", O_RDWR, S_IRUSR | S_IWUSR);
    fd[4] = shm_open("mutex", O_RDWR, S_IRUSR | S_IWUSR);

    A = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
    B = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
    C = mmap(NULL, MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
    ready = mmap(NULL, par_count * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
    mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd[4], 0);
    sleep(2); //needed for initalizing synch
    }


synch(par_id,par_count,ready, mutex);

if(par_id==0)
{
    //TODO: initialize the matrices A and B
    for (int i = 0; i < MATRIX_DIMENSION_XY; i++)
    {
        for (int j = 0; j < MATRIX_DIMENSION_XY; j++)
        {
        A[i * MATRIX_DIMENSION_XY + j] = (i + 1) * (j + 1);
        B[i * MATRIX_DIMENSION_XY + j] = (i + 1) + (j + 1);
        }
    }
}

synch(par_id,par_count,ready, mutex);

//TODO: quadratic_matrix_multiplication_parallel(par_id, par_count,A,B,C, ...);

struct timespec start_time_with_sync, end_time_with_sync, start_time_without_sync, end_time_without_sync;

clock_gettime(CLOCK_REALTIME, &start_time_with_sync);

for (int a = par_id; a < MATRIX_DIMENSION_XY; a += par_count) // Changed here
{
    clock_gettime(CLOCK_REALTIME, &start_time_without_sync);
    printf("Process %d: Beginning multiplication...\n", par_id);
    for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
    {
        C[a * MATRIX_DIMENSION_XY + b] = 0.0;
        for (int c = 0; c < MATRIX_DIMENSION_XY; c++)
        {
            C[a * MATRIX_DIMENSION_XY + b] += A[a * MATRIX_DIMENSION_XY + c] * B[c * MATRIX_DIMENSION_XY + b];
        }
    }
    printf("Process %d: Finished multiplication...\n", par_id);
    clock_gettime(CLOCK_REALTIME, &end_time_without_sync);
    printf("Time taken for multiplication (without sync): %.6f seconds\n", (end_time_without_sync.tv_sec - start_time_without_sync.tv_sec) + (end_time_without_sync.tv_nsec - start_time_without_sync.tv_nsec) / 1000000000.0);
}


clock_gettime(CLOCK_REALTIME, &end_time_with_sync);
printf("Time taken for multiplication (with sync): %.6f seconds\n", (end_time_with_sync.tv_sec - start_time_with_sync.tv_sec) + (end_time_with_sync.tv_nsec - start_time_with_sync.tv_nsec) / 1000000000.0);

synch(par_id,par_count,ready, mutex);

if(par_id==0)
{
    quadratic_matrix_print(C);
    pthread_mutex_destroy(mutex);
    shm_unlink("mutex");
}
    
synch(par_id, par_count, ready, mutex);

//lets test the result:



float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
quadratic_matrix_multiplication(A, B, M);

/*
printf("\nValues in M:\n");
for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
    for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
        printf("%.2f ", M[i * MATRIX_DIMENSION_XY + j]);
    }
    printf("\n");
}


printf("\nValues in C:\n");
for (int i = 0; i < MATRIX_DIMENSION_XY; i++) {
    for (int j = 0; j < MATRIX_DIMENSION_XY; j++) {
        printf("%.2f ", C[i * MATRIX_DIMENSION_XY + j]);
    }
    printf("\n");
}
*/


if (quadratic_matrix_compare(C, M))
	printf("full points!\n");
else
	printf("buuug!\n");

printf("par_id: %d, par_count: %d\n", par_id, par_count);

close(fd[0]);
close(fd[1]);
close(fd[2]);
close(fd[3]);

if (par_id == 0)
{
    // Only the process that created the shared memory objects should unlink them
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