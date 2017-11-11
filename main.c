/*
 * Virtual Memory Simulator Project
 *
 * Eric Kuha
 * Carlos Ferry
 * Jared Willard
 *
 * Three page fault handlders with three different
 * eviction policies.
 *
 *  FIFO: evicts on a first-in-first-out policy.
 *
 *  Random: evicts pages completely randomly.
 *
 *  Custom: evicts pages on a random, second-chance basis, favoring
 *          pages that have the dirty bit set. We have named this
 *          algorithm: "Second-try-random", or "2ndrand".
 */

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* Handy Bool Typedef */

typedef enum { false, true } bool;  


static int TotalPages = 0;
static int TotalFrames = 0; 

/* Pointers to data structures that the fault handlers need */

char *physmem = NULL;

struct disk *disk = NULL;

/* Handles appending file */

FILE *Output = NULL;

/* Algo option provided by user in argc. */

static int UserOption = 0;

/* Fault counters */

static int Faults = 0;
static int Writes = 0;
static int Reads = 0;

/* Counts number of frames occupied (random/custom) */ 

static int counter = 0; 

int *FrameArray;

/* FIFO data structures */

typedef struct FifoNode
{
    int data;
    struct FifoNode *next;
} FifoNode;

typedef struct FifoQueue
{
    int size;
    struct FifoNode *front, *rear;
    unsigned capacity;
} FifoQueue;

FifoQueue *fifoq;

void setup_fifo(int cap);
void push_fifo(int page);
int pop_fifo(void);

/* Set up for random algorithm. */

void setup_frame_array(int cap);

int cust_get_frame(struct page_table *pt);

void FrameworkSetup(struct page_table *pt);

/*
 * FIFO agorithm for handling page faults.
 * Very basic fault handler which evicts pages based
 * solely on position in queue.
 */

void fifo_fault_handler( struct page_table *pt, int page)
{
    /* Increment page fault counter. */
    
    Faults++;

    /* Get information about the page. */
    
    int frame, bits;
    
    page_table_get_entry(pt, page, &frame, &bits);

    /* if this page is not in memory */
    
    if (!bits&PROT_READ)
    {
        /* There will be a read. */
        
        Reads++;
        
        /* If not all frames are being used. */
        
        if (fifoq->size < fifoq->capacity)
        {
            /* It will take the next frame in the queue. */
            
            frame = fifoq->size;
            disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
        }
        else
        {
            /* We need to evict the front page in the queue. */
            
            int out_page = pop_fifo();
            int out_frame, out_bits;
            page_table_get_entry(pt, out_page, &out_frame, &out_bits);

            /* If the evicted frame is dirty, write it to the disk */
            
            if (out_bits&PROT_WRITE)
            {
                Writes++;
                disk_write(disk, out_page, &physmem[out_frame*PAGE_SIZE]);
            }
            
            disk_read(disk, page, &physmem[out_frame*PAGE_SIZE]);
            page_table_set_entry(pt, out_page, 0, 0);
            frame = out_frame;
        }
        
        push_fifo(page);
        bits = PROT_READ;
    }
    
    /* else, make it dirty */
    
    else
    {
        bits = PROT_READ|PROT_WRITE;
    }

    page_table_set_entry(pt,page,frame,bits);

}

/*
 * FindPage finds a particular page in an array
 * Used by Custom and Random handlers
 */

int FindPage(int* arr, int size, int key) 
{
      for (unsigned i = 0; i < size; i++) 
      {
            if (arr[i] == key)
            {
                return i;
            }    
      }
    
      return -1;
}

/*
 * Custom Fault Handler
 * Eviction policy is based on random but gives
 * a second chance to any frame with PROT_WRITE
 * bit set.
 */
 
void cust_fault_handler(struct page_table *pt, int page)
{
    Faults++;

    /* Get info about faulting frame. */
    
    int frame, bits;
    
    page_table_get_entry(pt, page, &frame, &bits);

    /* If page is not in memory. */
    
    if (!bits&PROT_READ)
    {
        Reads++;
        
        /* If we still have unallocated frames. */
        
        if (counter < TotalFrames)
        {
             FrameArray[counter] = page;
             counter++;
             disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
        }
        
        /* Execute eviction policy */
        
        else
        {
            /* Find a frame to evict here. */
            
            int out_page = cust_get_frame(pt);
            int out_frame, out_bits;
            page_table_get_entry(pt, out_page, &out_frame, &out_bits);
            
            if (out_bits&PROT_WRITE)
            {
                Writes++;
                disk_write(disk, out_page, &physmem[out_frame*PAGE_SIZE]);
            }
            
            disk_read(disk, page, &physmem[out_frame*PAGE_SIZE]);
            page_table_set_entry(pt, out_page, 0, 0);
            frame = out_frame;
            bits = PROT_READ;
            
            /* put the new page into the FrameArray */
            
            int frame_index = FindPage(FrameArray, TotalFrames, out_page);
            FrameArray[frame_index] = page;
        }
        
        bits = PROT_READ;
    }
    
    /* else make it dirty */
    
    else
    {
        bits = PROT_READ|PROT_WRITE;
    }
   
    /* Finally, set the page table entry. */
    
    page_table_set_entry(pt,page,frame,bits);
}

/*
 * Random Fault Handler
 * Chooses frames to evict purely randomly.
 */
 
void random_fault_handler(struct page_table *pt, int page)
{
    Faults++;

    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);

    /* Checks If page is not in memory. */
    
    if (!bits&PROT_READ)
    {
        Reads++;
        
        /* If we still have unallocated frames */
        
        if (counter < TotalFrames)
        {
            FrameArray[counter] = page;
            counter++;
            disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
        }
        else
        {
            int out_page = rand() % TotalFrames;
            int out_frame, out_bits;
            page_table_get_entry(pt, out_page, &out_frame, &out_bits);
           
            if (out_bits&PROT_WRITE)
            {
                Writes++;
                disk_write(disk, out_page, &physmem[out_frame*PAGE_SIZE]);
            }

            disk_read(disk, page, &physmem[out_frame*PAGE_SIZE]);
            page_table_set_entry(pt, out_page, 0, 0);
            frame = out_frame;
            bits = PROT_READ;

            /* Put new page into FrameArray */
            
            int frameIndex = FindPage(FrameArray, TotalFrames, out_page);
            FrameArray[frameIndex] = page;
        }
        
        bits = PROT_READ;
    }
    else
    {
        bits = PROT_READ|PROT_WRITE;
    }
    
    page_table_set_entry(pt,page,frame,bits);
}

/*
 * Naive page fault handler
 */
 
void page_fault_handler( struct page_table *pt, int page )
{
    printf("page fault on page #%d\n",page);
    page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
    page_table_print(pt);
}

int main(int argc, char *argv[])
{
    if (argc != 5) 
    {
        printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
        return 1;
    }

    int npages = atoi(argv[1]);
    int nframes = atoi(argv[2]);

    char *outputFile;

    /* Checks options provided */

    if (!strcmp(argv[3], "rand"))
    {
        setup_frame_array(nframes);
        UserOption = 1;
        srand(time(NULL));
    }
    else if (!strcmp(argv[3], "fifo"))
    {
        setup_fifo(nframes);
        UserOption = 2;
    }
    else if (!strcmp(argv[3], "custom"))
    {
        setup_frame_array(nframes);
        UserOption = 3;
        srand(time(NULL));
    }

    const char *program = argv[4];

    if (npages < 3) 
    {
        printf("Your page count is too small. The minimum number of page required is 3.\n");
        return 1;
    }

    if (nframes < 3) 
    {
        printf("Your frame count is too small. The minimum number of frames required is 3.\n");
        return 1;
    }


    disk = disk_open("myvirtualdisk",npages);

    if (!disk) 
    {
        fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
        return 1;
    }

    struct page_table *pt = NULL;

    /* Handles FIFO */

    if (UserOption == 2)
    {
        pt = page_table_create( npages, nframes, fifo_fault_handler);
    }
    
    /* Handles Random */
    
    else if (UserOption == 1)
    {
        pt = page_table_create( npages, nframes, random_fault_handler);
        FrameworkSetup(pt);
    }
    
    /* Custom */
    
    else if (UserOption == 3)
    {
        pt = page_table_create( npages, nframes, cust_fault_handler);
        FrameworkSetup(pt);
    }

    else
    {
        printf("Unknown option!: %d\n", UserOption);
        exit(0);
    }

    if(!pt) 
    {
        fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
        return 1;
    }

    char *virtmem = page_table_get_virtmem(pt);

    physmem = page_table_get_physmem(pt);


    if(!strcmp(program,"sort")) 
    {
        sort_program(virtmem,npages*PAGE_SIZE);
        outputFile = "sort_results.csv";

    } 
    else if(!strcmp(program,"scan")) 
    {
        scan_program(virtmem,npages*PAGE_SIZE);
        outputFile = "scan_results.csv";

    } 
    else if(!strcmp(program,"focus")) 
    {
        focus_program(virtmem,npages*PAGE_SIZE);
        outputFile = "focus_results.csv";

    } 
    else 
    {
        fprintf(stderr,"unknown program: %s\n",argv[3]);
    }

    Output = fopen(outputFile, "a");

    if (NULL != Output)
    {
        fseek(Output, 0, SEEK_END);
        int size = ftell(Output);

        if (size == 0)
        {
            fprintf(Output, "Frames,Pages,Faults,Reads,Writes\n");
        }
    }

    printf("\nFrames: %d, Pages: %d, Faults: %d, Reads: %d, Writes: %d\n", nframes, npages, Faults, Reads, Writes);

    fprintf(Output, "%d, %d, %d, %d, %d\n", nframes, npages, Faults, Reads, Writes);

    fclose(Output);

    page_table_delete(pt);
    disk_close(disk);

    return 0;
}

/*
 * Sets up int array of frame numbers
 * based on a based capacity.
 * Initializes all indices to -1
 */
 
void setup_frame_array(int cap)
{
    FrameArray = (int *)malloc(cap * sizeof(int));

    for (unsigned i = 0; i < cap; i++)
    {
        FrameArray[i] = -1;
    }
}

/*
 * Creates FIFO queue.
 */
 
void setup_fifo(int cap)
{
    FifoQueue *q = (FifoQueue*)malloc(sizeof(FifoQueue));
    q->capacity = cap;
    q->size = 0;

    fifoq = q;
}

/* 
 * Pushes an int page number to the back
 * of the FIFO queue
 */

void push_fifo(int page)
{
    FifoNode *n = (FifoNode*)malloc(sizeof(FifoNode));
    n->data = page;

    if (fifoq->size > 0)
    {
        fifoq->rear->next = n;
        fifoq->rear = n;
    }
    else
    {
        fifoq->front = n;
        fifoq->rear = n;
    }
    
    fifoq->size = fifoq->size + 1;
}

/*
 * Pops an int page number from
 * the front of the FIFO queue
 */
 
int pop_fifo(void)
{
    FifoNode *n = fifoq->front;
    int i = n->data;

    fifoq->front = n->next;
    free(n);
    fifoq->size = fifoq->size - 1;
    return i;
}

/*
 * Finds a frame to evict based on a random, second-chance
 * policy:
 * 1. Select a random frame.
 * 2. If the PROT_EXEC bit is set, unset it and look again.
 * 3. If it is not set, return that page.
 */

int cust_get_frame(struct page_table *pt)
{
    while(1)
    {
        int page = FrameArray[rand() % TotalFrames];
        int frame, bits;
        page_table_get_entry(pt, page, &frame, &bits);
        if(bits&PROT_EXEC)
        {
            bits = PROT_READ|PROT_WRITE;
            page_table_set_entry(pt, page, frame, bits);
        }
        else
        {
            return page;
        }
    }
}

/*
 * Establishes some of the important globals for
 * the random and custom algorithms
 */

void FrameworkSetup(struct page_table *pt)
{
    TotalPages = page_table_get_npages(pt);
    TotalFrames = page_table_get_nframes(pt);
    physmem = page_table_get_physmem(pt);
}
