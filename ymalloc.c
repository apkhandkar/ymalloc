#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* 32MB ought to be plenty to begin with */
#define YALLOC_BLOCK_SIZE 1024*1024*32

struct bhead {
	char free;
	void * next;
};

void * MEM;
unsigned int SIZE;

int INITIATED = 0;
int WARNING_PRINTED = 0;

void * yalloc(ssize_t size) 
{
	printf("\n***warning: this program uses ymalloc(), use allocated heap VERY carefully***\n\n"); 
	void * MEM = malloc(size);
	SIZE = size;
	struct bhead * header = MEM;
	header->free = '1';
	header->next = (MEM + SIZE);
 
	return MEM;
}
 
void * ymalloc(ssize_t size)
{
	if(INITIATED == 0) {
		INITIATED = 1;
		MEM = yalloc(YALLOC_BLOCK_SIZE);
  	}

  	struct bhead * header;
  	void * temp = MEM;
 	void * alloc_addr;
  	void * curr_next;
  	ssize_t blocksize;
  
  	while(temp != (MEM + SIZE)) {

		header = temp;
		
		/* blocksize cannot be negative */
		assert((blocksize = (header->next) - (temp + sizeof(struct bhead))) >= 0);

		if((size + sizeof(struct bhead)) < blocksize && (header->free == '1')) {

			/* block is larger than what we need, split it into two */
	  		alloc_addr = temp + sizeof(struct bhead);
	  
	  		curr_next = header->next;

	  		/* resized block isn't free anymore */
	  		header->free = '0';
	  		/* point to the free block that was created after the split */
	  		header->next = temp + sizeof(struct bhead) + size;

	  		/* create a header for the newly created free block */
	  		header = (temp + sizeof(struct bhead) + size);
			/* mark it as a free block */
	  		header->free = '1';
	  		header->next = curr_next;

	  		return alloc_addr;
	  
		} else if(size == blocksize && (header->free == '1')) {

	  		/* perfect fit: get the address we need and flip the status bit */
	  		alloc_addr = temp + sizeof(struct bhead);
  
	 	 	header->free = '0';
	  
	  		return alloc_addr;
	  
		} else {
	  		/* block isn't free or isn't large enough */
		}

		temp = header->next;
	}

	/* follow the malloc() spec and return NULL upon failure to allocate */
  	return NULL;
}

void merge()
{
  	struct bhead * header, * next_header;
  	void * temp = MEM;
  	void * next_block;
  
  	while(temp != (MEM + SIZE)) {
		header = temp;
  
		next_block = header->next;
		next_header = next_block;
  
		/* check if both current and next blocks are free */
		if((header->free == '1') && (next_header->free == '1')) {

	  		/* if so, merge them into one large free block */
	  		header->next = next_header->next;    
		} 

		temp = header->next;
  	}
}

void yfree(void * addr)
{
  	struct bhead * header;
  	void * temp = MEM;

	int visited = 0;

  	while(temp != (MEM + SIZE)) {
		header = temp;

		if((temp + sizeof(struct bhead)) == addr) {

			if(header->free == '0') {

				/* this is the block we wish to free */
				header->free = '1';
				merge();

			} else {
				/* seems we're trying to free a free block, do nothing for now */
			}

			/* the address passed was allocated using ymalloc() */
			visited = 1;		

		}

		temp = header->next;  
  	}

	if(visited == 0) {
		/* the address passed wasn't allocated using ymalloc() */
		fprintf(stderr, "ymalloc: error: yfree(): invalid pointer: %p\n",
			addr);
		/* we can't deal with this shit anymore */
		exit(-1);
	}
}

void mem_map()
{
  	struct bhead * header;
 	void * temp = MEM;
  	ssize_t blocksize;

	printf("\n");
	printf("***ymalloc: allocation map***\n");
  	while(temp != (MEM + SIZE)) {
		header = temp;

		blocksize = (header->next) - (temp + sizeof(struct bhead)); 

		printf("\n-------Block-------\n");
		printf("Address: %p\n", temp);
		printf("Header Information:\n  'Free': %s\n", ((header->free == '1') ? "yes" : "no"));
		printf("  'Next': %p\n", header->next);
		printf("Block size: %ld\n", blocksize);
		printf("-------------------\n\n");
 
		temp = header->next;
  	}

	printf("\n");
}

void ymalloc_summary()
{
	ssize_t sz_total = 0;
	ssize_t sz_headr = 0;
	ssize_t sz_hoccs = 0;
	ssize_t sz_hfree = 0;
	ssize_t sz_bloks = 0;
	ssize_t sz_blkoc = 0;
	ssize_t sz_blkfr = 0;

	ssize_t blocksize;

	void * temp = MEM;
	struct bhead *header;	

	while (temp != (MEM + SIZE)) {
		header = temp;

		blocksize = (header->next) - (temp + sizeof(struct bhead));

		if(header->free == '1') {
			sz_blkfr += blocksize;
			sz_hfree += sizeof(struct bhead);
		} else {
			sz_blkoc += blocksize;
			sz_hoccs += sizeof(struct bhead);
		}

		sz_headr += sizeof(struct bhead);
		sz_bloks += blocksize;

		sz_total += sizeof(struct bhead) + blocksize;

		temp = header->next;
	}

	printf("\n");
	printf("***ymalloc: usage summary***\n");
	printf("Block size           : %d\n", sz_bloks);
	printf("     [in free blocks]: %d\n", sz_blkfr);
	printf(" [in occupied blocks]: %d\n", sz_blkoc);
	printf("Header size          : %d\n", sz_headr);
	printf("    [in free headers]: %d\n", sz_hfree);
	printf("[in occupied headers]: %d\n", sz_hoccs);
	printf("Total size           : %d\n", sz_total);
	printf("\n");

	return;
}
