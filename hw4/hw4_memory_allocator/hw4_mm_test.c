#include "hw4_mm_test.h"

int main()
{
	char command[1024], s[32];
	int size;
	void *ptr=NULL;
	while(scanf("%s", command)==1) {
// printf("%s ", command);
		if(strcmp(command, "alloc")==0) {
			scanf("%d", &size);
// printf("%d\n", size);
			printf("0x%08lx\n", (uintptr_t)hw_malloc(size));
		} else if(strcmp(command, "free")==0) {
			scanf("%p", &ptr);
// printf("0x%08lx\n", (uintptr_t)ptr);
			printf(hw_free(ptr)? "success\n": "fail\n");
		} else if(strcmp(command, "print")==0) {
			scanf("%s", s);
// printf("%s\n", s);
			print_bin(s);
		}
		fflush(stdout);
	}


}