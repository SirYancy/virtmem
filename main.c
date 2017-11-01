/*
   Main program for the virtual memory project.
   Make all of your modifications to this file.
   You may add or rearrange any code or data as you need.
   The header files page_table.h and disk.h explain
   how to use the page table and disk interfaces.
   */

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int TotalPages = 0;

static int TotalFrames = 0; 

char *physmem = NULL;

struct disk *disk = NULL;

/* Handles appending file */

FILE *Output = NULL;

/* Algo option has provided by user. */

int UserOption = 0;

/* Page faults */

static int Faults = 0;

/* Disk writes */

static int Writes = 0;

/* Disk reads */

static int Reads = 0;

/* Counter */ 

static int counter = 0; 

int *MemArray;

/* Fifo implementation */

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

/* MemArray's memory allocation. Used at starting time. */

void setup_mem_array(int cap);

void push_fifo(int page);
int pop_fifo(void);



void fifo_fault_handler( struct page_table *pt, int page)
{
    // Increment page fault counter;
    Faults++;
    // printf("FIFO page fault on page #%d\n",page);

    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);

    // if this page is not in memory
    if(!bits&PROT_READ)
    {
        Reads++;
        // If not all frames are being used
        if(fifoq->size < fifoq->capacity)
        {
            frame = fifoq->size;
            disk_read(disk, page, &physmem[frame*PAGE_SIZE]);
        }
        else
        {
            int out_page = pop_fifo();
            int out_frame, out_bits;
            page_table_get_entry(pt, out_page, &out_frame, &out_bits);
            // If the current frame is dirty
            if(out_bits&PROT_WRITE)
            {
                disk_write(disk, out_page, &physmem[out_frame*PAGE_SIZE]);
                disk_read(disk, page, &physmem[out_frame*PAGE_SIZE]);
            }
            page_table_set_entry(pt, out_page, 0, 0);
            frame = out_frame;
        }
        push_fifo(page);
        bits = PROT_READ;
    }
    // else, make it dirty
    else
    {
        Writes++;
        bits = PROT_READ|PROT_WRITE;
    }
    page_table_set_entry(pt,page,frame,bits);

}


int FindPage(int begin, int end, int key) 
{
    for (unsigned i = begin; i <= end; i++) 
    {
        if (MemArray[i] == key)
        {
            return i;
        }  
    }

    return -1;
}

void FrameworkSetup(struct page_table *pt)
{
     TotalPages = page_table_get_npages(pt);
     TotalFrames = page_table_get_nframes(pt);
     physmem = page_table_get_physmem(pt);
}

void custom_fault_handler(struct page_table *pt, int page)
{
    FrameworkSetup(pt);
    
    int randoms;
    
    if (TotalFrames >= TotalPages) 
    {
          page_table_set_entry(pt, page, page, PROT_READ|PROT_WRITE);
          Faults++;
          Writes = 0;
          Reads = 0;
    }
    else
    {
          physmem = page_table_get_physmem(pt);

          Faults++;
        
          randoms = lrand48() % TotalFrames;
	  
	  if ((page % 2) == 0)
	  {
		while ((randoms) % 2 !=0)
		{
			randoms = lrand48() % TotalFrames;
		}
			
	  }
	  else
	  {
		while ((randoms) % 2 == 0)
		{
			randoms=lrand48() % TotalFrames;
		}
	  }
		
          int aux = randoms;
        
          if (MemArray[aux] == page) 
          {
                page_table_set_entry(pt, page, aux, PROT_READ|PROT_WRITE);
                Faults--;
          } 
          else if(MemArray[aux] == -1) 
          {
                page_table_set_entry(pt, page, aux, PROT_READ);
                disk_read(disk, page, &physmem[aux * PAGE_SIZE]);
                Reads++;
          } 
          else 
          {
                disk_write(disk, MemArray[aux], &physmem[aux * PAGE_SIZE]);
                disk_read(disk, page, &physmem[aux * PAGE_SIZE]);
                page_table_set_entry(pt, page, aux, PROT_READ);

                Writes++;
                Reads++;
          }
            
                MemArray[aux] = page;
        }
}

/* Handles random faults */

void random_fault_handler(struct page_table *pt, int page)
{

    FrameworkSetup(pt);

    if (TotalFrames >= TotalPages) 
    {
        page_table_set_entry(pt, page, page, PROT_READ|PROT_WRITE);
        Faults++;
        Writes = 0;
        Reads = 0;
    }
    else
    {
        physmem = page_table_get_physmem(pt);
        Faults++;

        int result = FindPage(0, TotalFrames-1, page);
        int aux = lrand48() % TotalFrames;

        if (result > -1) 
        {
            page_table_set_entry(pt, page, result, PROT_READ|PROT_WRITE);
            Faults--;
        }
        else if (counter < TotalFrames) 
        {
            while (MemArray[aux] != -1) 
            {
                aux = lrand48() % TotalFrames;
                Faults++;
            }

            page_table_set_entry(pt, page, aux, PROT_READ);
            disk_read(disk, page, &physmem[aux * PAGE_SIZE]);

            Reads++;
            MemArray[aux] = page;
            counter++;
        } 
        else 
        {
            disk_write(disk, MemArray[aux], &physmem[aux * PAGE_SIZE]);
            disk_read(disk, page, &physmem[aux * PAGE_SIZE]);
            page_table_set_entry(pt, page, aux, PROT_READ);
            Writes++;
            Reads++;
            MemArray[aux] = page;
        }

    }

}


/******************END FIFO IMPL*************/

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

    /* Checks options provided */

    if (!strcmp(argv[3], "rand"))
    {
        setup_mem_array(nframes);
        UserOption = 1;
    }
    else if (!strcmp(argv[3], "fifo"))
    {
        setup_fifo(nframes);
        UserOption = 2;
    }
    else if (!strcmp(argv[3], "custom"))
    {
        setup_mem_array(nframes);
        UserOption = 3;
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

    Output = fopen("results.csv", "a");

    if(NULL != Output)
    {
        fseek(Output, 0, SEEK_END);
        int size = ftell(Output);

        if (size == 0)
        {
            fprintf(Output, "Frames, Pages, Faults, Reads, Writes\n");
        }
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
    }
    else if (UserOption == 3)
    {
            pt = page_table_create( npages, nframes, custom_fault_handler);
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


    if(!strcmp(program,"sort")) {
        sort_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"scan")) {
        scan_program(virtmem,npages*PAGE_SIZE);

    } else if(!strcmp(program,"focus")) {
        focus_program(virtmem,npages*PAGE_SIZE);

    } else {
        fprintf(stderr,"unknown program: %s\n",argv[3]);

    }

    printf("\nFrames: %d, Pages: %d, Faults: %d, Reads: %d, Writes: %d\n", nframes, npages, Faults, Reads, Writes);

    fprintf(Output, "%d, %d, %d, %d, %d\n", nframes, npages, Faults, Reads, Writes);

    fclose(Output);

    page_table_delete(pt);
    disk_close(disk);

    return 0;
}

void setup_mem_array(int cap)
{
    MemArray = (int *)malloc(cap * sizeof(int));

    for (unsigned i = 0; i < cap; i++)
    {
        MemArray[i] = -1;
    }
}

void setup_fifo(int cap)
{
    FifoQueue *q = (FifoQueue*)malloc(sizeof(FifoQueue));
    q->capacity = cap;
    q->size = 0;

    fifoq = q;
}
void push_fifo(int page)
{
    FifoNode *n = (FifoNode*)malloc(sizeof(FifoNode));
    n->data = page;

    if(fifoq->size > 0)
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
int pop_fifo(void)
{
    FifoNode *n = fifoq->front;
    int i = n->data;

    fifoq->front = n->next;
    free(n);
    fifoq->size = fifoq->size - 1;
    return i;
}
