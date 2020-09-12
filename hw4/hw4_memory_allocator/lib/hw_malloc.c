#include "hw_malloc.h"

#define HEAP_SIZE 65536

void *start_sbrk=NULL;
void *end_sbrk=NULL;
struct chunk_header *bin[7];

void *hw_malloc(size_t bytes)
{
	if(start_sbrk == NULL) init_all();

	if(bytes==0) ++bytes;

	size_t chunk_size = (bytes+7)/8*8 + 40;
	void *chunk=NULL;

	if(chunk_size<=88) chunk=get_bin(chunk_size);

	if(chunk==NULL)	chunk=split(chunk_size);

	if(chunk) {
		bin_pop(chunk);
		set_next_chunk(chunk, 0);
		return (void*)(chunk-start_sbrk+40);
	} else {
		return NULL;
	}
}

int hw_free(void *mem)
{
	void *ptr = mem+(size_t)start_sbrk-40;
	if(is_header(ptr)==0)
		return 0;

	struct chunk_header *chunk = ptr;

	set_next_chunk(bin_add(merge(chunk)), 1);

	return 1;
}

void *get_start_sbrk(void)
{
	if (start_sbrk==NULL) init_all();
	return start_sbrk;
}

void init_all()
{
	start_sbrk = sbrk(HEAP_SIZE);
	end_sbrk = sbrk(0);

	for(int i=0; i<7; ++i) bin[i] = NULL;

	struct chunk_header *handle = start_sbrk;
	handle->prev = NULL;
	handle->next = NULL;
	handle->chunk_size = HEAP_SIZE;
	handle->pre_chunk_size = 0;
	handle->prev_free_flag = 0;
	bin_add(handle);
}

void *split(size_t chunk_size)
{
	if(bin[6]==NULL)
		return NULL;

	size_t target = chunk_size + 48;
	struct chunk_header *cur;

	cur=bin[6];
	do {
		if(cur->chunk_size==chunk_size) {
			bin_pop(cur);
			return cur;
		}
		cur = cur->next;
	} while(cur!=bin[6]);


	cur=bin[6]->prev;
	do {
		if(cur->chunk_size >= chunk_size) {
			size_t size = cur->chunk_size;
			while(cur->prev->chunk_size==size) {
				if(cur==bin[6])
					break;
				cur = cur->prev;
			}
			break;
		}
		cur = cur->prev;
	} while(cur!=bin[6]->prev);

	if (cur->chunk_size < chunk_size)
		return NULL;

	bin_pop(cur);

	if (cur->chunk_size < target)
		return cur;

	// split new chunk / upper address
	void *ptr = cur;
	struct chunk_header *temp = ptr + chunk_size;

	temp->prev = NULL;
	temp->next = NULL;
	temp->chunk_size = cur->chunk_size - chunk_size;
	temp->pre_chunk_size = chunk_size;
	temp->prev_free_flag = 0;
	bin_add(temp);
	set_next_chunk(temp, 1);

	// set new chunk size
	cur->chunk_size = chunk_size;
	return cur;
}

void *merge(struct chunk_header *chunk)
{
	void *ptr = chunk;
	struct chunk_header *prev, *next, *newc;
	prev = ptr - chunk->pre_chunk_size;
	next = ptr + chunk->chunk_size;
	newc = NULL;

	if(chunk->prev_free_flag==1)
		newc = bin_pop(prev);

	bin_pop(chunk);
	if(newc)
		newc->chunk_size += chunk->chunk_size;
	else
		newc = chunk;

	if(is_header(next)==1 && next->next!=NULL) {
		bin_pop(next);
		newc->chunk_size += next->chunk_size;
	}
	set_next_chunk(newc, 1);

	return newc;
}

void *bin_pop(struct chunk_header *chunk)
{
	if (!chunk || chunk->next==NULL) {
		return chunk;
	} else if(chunk->next == chunk) {
		switch(chunk->chunk_size) {
		case 48:
			bin[0]=NULL;
			break;
		case 56:
			bin[1]=NULL;
			break;
		case 64:
			bin[2]=NULL;
			break;
		case 72:
			bin[3]=NULL;
			break;
		case 80:
			bin[4]=NULL;
			break;
		case 88:
			bin[5]=NULL;
			break;
		default:
			bin[6]=NULL;
			break;
		}
	} else if(chunk==get_bin(chunk->chunk_size)) {
		switch(chunk->chunk_size) {
		case 48:
			bin[0]=bin[0]->next;
			break;
		case 56:
			bin[1]=bin[1]->next;
			break;
		case 64:
			bin[2]=bin[2]->next;
			break;
		case 72:
			bin[3]=bin[3]->next;
			break;
		case 80:
			bin[4]=bin[4]->next;
			break;
		case 88:
			bin[5]=bin[5]->next;
			break;
		default:
			bin[6]=bin[6]->next;
			break;
		}
	}
	chunk->prev->next = chunk->next;
	chunk->next->prev = chunk->prev;
	chunk->prev = NULL;
	chunk->next = NULL;
	return chunk;
}

void *bin_add(struct chunk_header *chunk)
{
	if(chunk->prev!=NULL)
		return chunk;
	struct chunk_header *head = get_bin(chunk->chunk_size);
	if(head==NULL) {
		chunk->prev = chunk;
		chunk->next = chunk;
		switch(chunk->chunk_size) {
		case 48:
			bin[0]=chunk;
			break;
		case 56:
			bin[1]=chunk;
			break;
		case 64:
			bin[2]=chunk;
			break;
		case 72:
			bin[3]=chunk;
			break;
		case 80:
			bin[4]=chunk;
			break;
		case 88:
			bin[5]=chunk;
			break;
		default:
			bin[6]=chunk;
			break;
		}
	} else if(chunk->chunk_size <= 88) {
		chunk->prev = head->prev;
		chunk->next = head;
		chunk->prev->next = chunk;
		chunk->next->prev = chunk;
	} else {
		struct chunk_header *cur=bin[6];
		do {
			if(chunk->chunk_size > cur->chunk_size)
				break;
			cur = cur->next;
		} while(cur!=bin[6]);
		if(chunk->chunk_size > bin[6]->chunk_size)
			bin[6] = chunk;
		chunk->next = cur;
		chunk->prev = cur->prev;
		chunk->next->prev = chunk;
		chunk->prev->next = chunk;
	}
	return chunk;
}

struct chunk_header *get_bin(size_t chunk_size)
{
	switch(chunk_size) {
	case 48:
		return bin[0];
	case 56:
		return bin[1];
	case 64:
		return bin[2];
	case 72:
		return bin[3];
	case 80:
		return bin[4];
	case 88:
		return bin[5];
	default:
		return bin[6];
	}
}

void set_next_chunk(struct chunk_header *chunk, size_t flag)
{
	void *ptr = chunk;
	struct chunk_header *next = ptr + chunk->chunk_size;
	if (is_header(next)==1) {
		next->prev_free_flag = flag;
		next->pre_chunk_size = chunk->chunk_size;
	}
}

int is_header(void *ptr)
{
	if(ptr<start_sbrk || ptr>=end_sbrk) return 0;

	struct chunk_header *cur = start_sbrk;
	while( ((void*)cur) < end_sbrk ) {
		if (cur==ptr) return 1;
		cur = ((void*)cur) + cur->chunk_size;
	}

	return 0;
}

void print_bin(const char *s)
{
	struct chunk_header *head=NULL;
	if(strcmp(s,"bin[0]")==0)		head = bin[0];
	else if(strcmp(s,"bin[1]")==0)	head = bin[1];
	else if(strcmp(s,"bin[2]")==0)	head = bin[2];
	else if(strcmp(s,"bin[3]")==0) 	head = bin[3];
	else if(strcmp(s,"bin[4]")==0) 	head = bin[4];
	else if(strcmp(s,"bin[5]")==0) 	head = bin[5];
	else if(strcmp(s,"bin[6]")==0) 	head = bin[6];

	if(head==NULL) return;

	struct chunk_header *cur=head;
	do {
		printf("0x%08lx", (uintptr_t)( ((void*)cur)-(size_t)start_sbrk ));
		printf("--------");
		printf("%lu\n", cur->chunk_size);
		cur = cur->next;
	} while(cur!=head);
}