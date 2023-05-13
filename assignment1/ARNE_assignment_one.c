#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef uint8_t BYTE;

typedef struct chunkinfo
    {
    int size;
    int info;
    BYTE *next, *prev;
    } chunkinfo;
void *startofheap = NULL;


chunkinfo *get_last_chunk()
    {
    if(!startofheap) return NULL;
    chunkinfo *ch = (chunkinfo *)startofheap;
    for (; ch->next; ch = (chunkinfo *)ch->next);
    return ch;
    }


BYTE *mymalloc(int size)
{
    int rsize = size + sizeof(chunkinfo); // total size including the chunkinfo size
    if(rsize % 4096 != 0)
    {
        rsize += 4096 - (rsize % 4096); // align to nearest multiple of 4096
    }

    if(!startofheap)
    {
        startofheap = sbrk(rsize);
        chunkinfo *ci = (chunkinfo *)startofheap;
        ci->size = rsize;
        ci->info = 1;
        ci->next = ci->prev = NULL;
        return (BYTE *)startofheap + sizeof(chunkinfo);
    }

    chunkinfo *current = (chunkinfo *)startofheap;
    chunkinfo *smallest_free_chunk = NULL;
    while (current)
    {
        if(current->info == 0 && current->size >= rsize)
        {
            if(!smallest_free_chunk || (current->size < smallest_free_chunk->size && current->size >= rsize))
            {
                smallest_free_chunk = current;
            }
        }
        current = (chunkinfo *)current->next;
    }

    if(smallest_free_chunk)
    {
        if(smallest_free_chunk->size - rsize >= 4096 + (int)sizeof(chunkinfo)) // if the chunk is large enough to split
        {
            chunkinfo *new_chunk = (chunkinfo *)((BYTE *)smallest_free_chunk + rsize);
            new_chunk->size = smallest_free_chunk->size - rsize;
            new_chunk->info = 0;
            new_chunk->prev = smallest_free_chunk;
            new_chunk->next = smallest_free_chunk->next;
            if(smallest_free_chunk->next) // if the current chunk is not the last chunk
            {
                ((chunkinfo *)smallest_free_chunk->next)->prev = (BYTE *)new_chunk;
            }
            smallest_free_chunk->next = (BYTE *)new_chunk;
            smallest_free_chunk->size = rsize;
        }
        // printf("Reusing a chunk at address %p with size %d\n", smallest_free_chunk, smallest_free_chunk->size);
        smallest_free_chunk->info = 1;
        return (BYTE *)smallest_free_chunk + sizeof(chunkinfo);
    }

    // if no suitable chunk found, then extend the heap
    chunkinfo *last_chunk = get_last_chunk();
    BYTE *new_addr = sbrk(rsize);
    chunkinfo *new_chunk = (chunkinfo *)new_addr;
    new_chunk->size = rsize;
    new_chunk->info = 1;
    new_chunk->prev = (BYTE *)last_chunk;
    new_chunk->next = NULL;
    // printf("No suitable chunk found, extending the heap by %d bytes\n", rsize);
    last_chunk->next = (BYTE *)new_chunk;

    return (BYTE *)new_addr + sizeof(chunkinfo);

}


void myfree(BYTE *addr)
{
    if(!addr) return;

    chunkinfo *current = (chunkinfo *)(addr - sizeof(chunkinfo));
    current->info = 0;

    if(current->next)
    {
        chunkinfo *next = (chunkinfo *)current->next;
        if(next->info == 0 && ((BYTE*)current + current->size == (BYTE*)next))  // check if the next chunk is free and adjacent
        {
            current->size += next->size;
            current->next = next->next;
            if(next->next)
            {
                chunkinfo *next_next = (chunkinfo *)next->next;
                next_next->prev = (BYTE *)current;
            }
        }
    }

    if(current->prev)
    {
        chunkinfo *prev = (chunkinfo *)current->prev;
        if(prev->info == 0 && ((BYTE*)prev + prev->size == (BYTE*)current))  // check if the previous chunk is free and adjacent
        {
            prev->size += current->size;
            prev->next = current->next;
            if(current->next)
            {
                chunkinfo *next = (chunkinfo *)current->next;
                next->prev = (BYTE *)prev;
            }
            current = prev;
        }
    }

    chunkinfo *last_chunk = get_last_chunk();
    if(last_chunk->info == 0)
    {
        if(last_chunk->prev)
        {
            chunkinfo *prev = (chunkinfo *)last_chunk->prev;
            prev->next = NULL;  // nullify the next pointer of the new last chunk
        }
        else
        {
            startofheap = NULL;
        }
        int res = brk((BYTE*)last_chunk);  // decrease the heap size just before the last_chunk
    }

    // printf("Freeing the chunk at address %p\n", addr);
}


// output (addresses are 4096 decimal apart, 1000)
void analyze()
    {
    printf("\n--------------------------------------------------------------\n");
    if(!startofheap)
        {
        printf("no heap");
        return;
        }
    chunkinfo *ch = (chunkinfo *)startofheap;
    for (int no = 0; ch; ch = (chunkinfo *)ch->next,no++)
        {
        printf("%d | current addr: %p |", no, (void *)ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        printf("next: %p | ", (void *)ch->next);
        printf("prev: %p", (void *)ch->prev);
        printf("\n");
        }
    printf("program break on address: %p\n", sbrk(0));
    }


int main()
    {
    BYTE *a[100];

    analyze(); // 50% points
    for (int i = 0; i < 100; i++)
        {
        a[i] = mymalloc(1000);
        }
    for (int i = 0; i < 90; i++)
        {
        myfree(a[i]);
        }
    analyze(); // 50% of points if this is correct
    myfree(a[95]);
    a[95] = mymalloc(1000);
    analyze(); // 25% points, this new chunk shoud fill the smaller free one
    // (best fit)
    for (int i = 90; i < 100; i++)
        {
        myfree(a[i]);
        }
    analyze(); // 25% should be an empty heap now with start address from the program start

    // tests 95% of the code
    BYTE *x = mymalloc(4000);
    BYTE *y = mymalloc(1000);
    analyze();
    myfree(x);
    analyze();
    x = mymalloc(4000);
    analyze();
    myfree(y);
    analyze();
    myfree(x);
    analyze();

    // if best fit doesn't work, -25%
    int page = 4096;
    a[0] = mymalloc(page * 4);
    a[1] = mymalloc(100);
    a[2] = mymalloc(page * 3);
    a[3] = mymalloc(100);
    a[4] = mymalloc(page * 2 - 10);
    a[5] = mymalloc(100);
    a[6] = mymalloc(100);
    a[7] = mymalloc(100);
    analyze();
    myfree(a[0]);
    myfree(a[2]);
    myfree(a[4]);
    myfree(a[6]);
    analyze();
    a[0] = mymalloc(100);
    analyze();

    // if splitting does not work, -15%
    a[2] = mymalloc(100);
    analyze();

    // tests for speed
    clock_t ca, cb;
    ca = clock(); // Start the timer

    for (int i = 0; i < 100; i++)
        {
        a[i] = mymalloc(1000);
        }
    for (int i = 0; i < 90; i++)
        {
        myfree(a[i]);
        }
    myfree(a[95]);
    a[95] = mymalloc(1000);
    for (int i = 90; i < 100; i++)
        {
        myfree(a[i]);
        }

    cb = clock(); // Stop the timer

    printf("\nduration: %f\n", (double)(cb - ca) / CLOCKS_PER_SEC);

    return 0;
    }