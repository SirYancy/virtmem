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

#define check_bit(var,pos) ((var) & (1 << (pos)))

struct disk *disk = NULL;
char *physmem = NULL;


/****************FIFO IMPL********************/
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

void fifo_fault_handler( struct page_table *pt, int page)
{
    printf("FIFO page fault on page #%d\n",page);

    int frame, bits;
    page_table_get_entry(pt, page, &frame, &bits);

    // if this page is not in memory
    if(!bits&PROT_READ)
    {
        // If not all frames are being used
        if(fifoq->size < fifoq->capacity)
        {
            frame = fifoq->size;
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
        bits = PROT_READ|PROT_WRITE;
    }
    page_table_set_entry(pt,page,frame,bits);
}

/******************END FIFO IMPL*************/

void page_fault_handler( struct page_table *pt, int page )
{
	printf("page fault on page #%d\n",page);
    page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
    page_table_print(pt);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);

    setup_fifo(nframes);

	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	// struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	struct page_table *pt = page_table_create( npages, nframes, fifo_fault_handler);
	if(!pt) {
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

	page_table_delete(pt);
	disk_close(disk);

	return 0;
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
