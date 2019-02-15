/** 
 * YMALLOC-EXPERIMENTAL
 * --------------------
 * Blocks are linked in two interspersed forward-link chains and
 * one backward-link chain.
 * 
 * Also, yfree_safe: the more agressive version of yfree() that 
 * zeroes out the memory being freed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

/* 16MB ought to be plenty to begin with */
#define _YM_SET_SIZE 1024*1024*16

struct bhead {
	short int integrity_chk;
	char free;
	/* 
		headers for free blocks forward-link to headers to next free block,
		and the same for occupied blocks
	 */
	void * next;
	/* 	
		headers for a block backwards-link to the previous block, regardless 
		of its occupancy
	*/
	void * prev;
};

void * _ym_base;
unsigned int _ym_size;

void *HEAD, *TAIL;
void *FOHEAD, *FOTAIL;
void *FFHEAD, *FFTAIL;
void *BHEAD, *BTAIL;

int _ym_init = 0;



void
*_get_mem(size_t size)
{
	int zerofd = -1;
	void * mem = NULL;

	assert((zerofd = open("/dev/zero", O_RDWR, 0)) > 0);

#if defined(__linux__)

	mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, zerofd, 0);

#elif defined(__unix__) || defined((__APPLE__) && defined(__MACH__))

	mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);

#endif 

	/* in case we fail to get memory */
	assert(mem != MAP_FAILED || mem != NULL);

	return mem;
}



void 
*_ymalloc_init(size_t size) 
{

#ifdef SUPRESS_YMALLOC_WARNING
#else
	printf("\n***warning: this program uses ymalloc(), use allocated heap VERY carefully***\n\n"); 
#endif

	void *got_addr = _get_mem(size);

	_ym_size = size;
	struct bhead * bh = got_addr;
	bh->integrity_chk = SHRT_MAX;
	bh->free = '1';
	bh->next = (got_addr + _ym_size);
 
	return got_addr;
}


 
void 
*ymalloc(size_t size)
{
	if(!_ym_init) {
		_ym_init = 1;
		_ym_base = _ymalloc_init(_YM_SET_SIZE);
  	}

  	struct bhead * bh;
  	void * tmp = _ym_base;
 	void * retadd;
  	void * curr_next;
  	size_t bsize;
  
  	while(tmp != (_ym_base + _ym_size)) {

		bh = tmp;

		/* perform a header integrity check */
		assert(bh->integrity_chk == SHRT_MAX);

		/* sanity check: 'next' address must be higher than current */
		/* zero-sized blocks are not allowed */
		assert((bh->next) > (tmp + sizeof(struct bhead)));

		bsize = (bh->next) - (tmp + sizeof(struct bhead));

		if(((sizeof(struct bhead) + size) < bsize) && (bh->free == '1')) {

			/* block is larger than what we need, split it into two */
	  		retadd = tmp + sizeof(struct bhead);
	  
	  		curr_next = bh->next;

	  		/* resized block isn't free anymore */
	  		bh->free = '0';
	  		/* point to the free block that was created after the split */
	  		bh->next = tmp + sizeof(struct bhead) + size;

	  		/* create a header for the newly created free block */
	  		bh = (tmp + sizeof(struct bhead) + size);
			bh->integrity_chk = SHRT_MAX;
			/* mark it as a free block */
	  		bh->free = '1';
	  		bh->next = curr_next;

	  		return retadd;
	  
		} else if((size == bsize) && (bh->free == '1')) {

	  		/* perfect fit OR allocate a little more than what was asked */
	  		retadd = tmp + sizeof(struct bhead);
  
	 	 	bh->free = '0';
	  
	  		return retadd;
	  
		} else {
			/* block isn't large enough to accomodate allocation request */
		}

		/* sanity check - are we jumping out of our block? */
		assert((tmp = bh->next) <= (_ym_base + _ym_size));
	}

	/* return NULL upon failure to allocate */
  	return NULL;
}



void 
_merge(void)
{
  	struct bhead * bh, * next_bh;
  	void * tmp = _ym_base;
  	void * next_block;
  
  	while(tmp != (_ym_base + _ym_size)) {
		bh = tmp;
  
		assert(bh->integrity_chk == SHRT_MAX);

		next_block = bh->next;
		next_bh = next_block;
  
		/* check if both current and next blocks are free */
		if((bh->free == '1') && (next_bh->free == '1')) {

	  		/* if so, coalesce them into one large free block */
	  		bh->next = next_bh->next;    
		} 

		assert((tmp = bh->next) <= (_ym_base + _ym_size));
  	}
}



int
_yfree_internal(void *addr, size_t *bsize)
{
  	struct bhead *bh;
  	void *tmp = _ym_base;

  	while(tmp != (_ym_base + _ym_size)) {

		bh = tmp;

		assert(bh->integrity_chk == SHRT_MAX);
		assert((bh->next) > (tmp + sizeof(struct bhead)));

		*bsize = (bh->next) - (tmp + sizeof(struct bhead));

		if((tmp + sizeof(struct bhead)) == addr) {

			if(bh->free == '0') {

				/* this is the block we wish to free */
				bh->free = '1';
				_merge();

				return 0;
			}

			return 1;
		}

		assert((tmp = bh->next) <= (_ym_base + _ym_size)); 
  	}

	return -1;
}



void
yfree(void *at)
{
	size_t blocksize;
	int yfree_internal_status = _yfree_internal(at, &blocksize);

	if(yfree_internal_status < 0) {
		fprintf(stderr, "yfree: invalid pointer: %p\n", at);
	}
}



void 
yfree_safe(void *at)
{
	size_t blocksize;
	int yfree_internal_status = _yfree_internal(at, &blocksize);

	if(!yfree_internal_status) {
		memset(at, (int) '\0', blocksize);
	} else if(yfree_internal_status < 0) {
		fprintf(stderr, "yfree_safe: invalid pointer: %p\n", at);	
	}
}



void 
mem_map()
{
  	struct bhead * bh;
 	void * tmp = _ym_base;
  	size_t blocksize;

	printf("\n");
	printf("***ymalloc: allocation map***\n");
  	while(tmp != (_ym_base + _ym_size)) {
		bh = tmp;

		assert(bh->integrity_chk == SHRT_MAX);

		assert((bh->next) > (tmp + sizeof(struct bhead)));
		blocksize = (bh->next) - (tmp + sizeof(struct bhead));

		printf("\n-------Block-------\n");
		printf("Address: %p\n", tmp);
		printf("Header Information:\n  'Free': %s\n", ((bh->free == '1') ? "yes" : "no"));
		printf("  'Next': %p\n", bh->next);
		printf("Block size: %ld\n", blocksize);
		printf("[with header]: %ld\n", blocksize + sizeof(struct bhead));
		printf("-------------------\n\n");
 
		assert((tmp = bh->next) <= (_ym_base + _ym_size));
  	}

	printf("\n");
}



void 
ymalloc_summary()
{
	/* in bytes, */
	/* total allocation */
	size_t sz_total = 0;
	/* space taken up by block headers */
	size_t sz_headr = 0;
	/* out of that, space taken up by headers of occupied blocks */
	size_t sz_hoccs = 0;
	/* space taken up by headers of free blocks */
	size_t sz_hfree = 0;
	/* space available to  blocks */
	size_t sz_bloks = 0;
	/* space taken up by occupied blocks */
	size_t sz_blkoc = 0;
	/* total space in free block(s) */
	size_t sz_blkfr = 0;

	size_t blocksize = 0;

	float pcfree = 0;

	void * tmp = _ym_base;
	struct bhead *bh;	

	while (tmp != (_ym_base + _ym_size)) {
		bh = tmp;

		assert(bh->integrity_chk == SHRT_MAX);

		assert((bh->next) > (tmp + sizeof(struct bhead)));
		blocksize = (bh->next) - (tmp + sizeof(struct bhead));

		if(bh->free == '1') {
			sz_blkfr += blocksize;
			sz_hfree += sizeof(struct bhead);
		} else {
			sz_blkoc += blocksize;
			sz_hoccs += sizeof(struct bhead);
		}

		sz_headr += sizeof(struct bhead);
		sz_bloks += blocksize;

		sz_total += sizeof(struct bhead) + blocksize;

		assert((tmp = bh->next) <= (_ym_base + _ym_size));
	}

	pcfree = ((float)(sz_total - (sz_blkoc + sz_headr)) / (float)sz_total) * 100;

	printf("\n");
	printf("***ymalloc: usage summary***\n");
	printf("Block size           : %d\n", sz_bloks);
	printf("     [in free blocks]: %d\n", sz_blkfr);
	printf(" [in occupied blocks]: %d\n", sz_blkoc);
	printf("Header size          : %d\n", sz_headr);
	printf("  [for free block(s)]: %d\n", sz_hfree);
	printf("[for occupied blocks]: %d\n", sz_hoccs);
	printf("Total size           : %d\n", sz_total);
	printf("Free                 ~ %.0f%%\n", pcfree);

	printf("\n");

	return;
}
