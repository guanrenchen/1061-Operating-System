#ifndef HW_MALLOC_H
#define HW_MALLOC_H

#include <stddef.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct chunk_header {
	struct chunk_header *prev;
	struct chunk_header *next;
	size_t chunk_size;			//header + data
	size_t pre_chunk_size;
	size_t prev_free_flag;
};

extern void *hw_malloc(size_t bytes);
extern int hw_free(void *mem);
extern void *get_start_sbrk(void);

void init_all();
void *split(size_t chunk_size);
void *merge(struct chunk_header *chunk);
void *bin_pop(struct chunk_header *chunk);
void *bin_add(struct chunk_header *chunk);
struct chunk_header *get_bin(size_t chunk_size);
void set_next_chunk(struct chunk_header *chunk, size_t flag);
int is_header(void *ptr);
void print_bin(const char *s);

#endif
