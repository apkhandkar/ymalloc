#ifndef YMALLOC_H
#define YMALLOC_H

void * yalloc(ssize_t size);
void * ymalloc(ssize_t size);
void yfree(void * addr);

void mem_map(void);
void ymalloc_summary(void);

extern void * MEM;

#endif
