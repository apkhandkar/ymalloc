#ifndef YMALLOC_H
#define YMALLOC_H

#include <stdlib.h>

void * ymalloc(ssize_t size);
void yfree(void * addr);
void yfree_safe(void * addr);

void mem_map(void);
void ymalloc_summary(void);

extern void * MEM;

#endif
