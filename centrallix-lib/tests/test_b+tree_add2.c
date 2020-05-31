#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 1 - insert from main function";

	BPTreeRoot root ;
	bptInitRoot(&root);

	char* key = "hi\0";
	char* val = "hopeful\0";

	int t = bptAdd(&root, key, 2, val);
	int iter = 9000000, i;
	printf("%s%d\n", "t: ", t);
	assert(t==1);
	
	//bpt_PrintTree(&root);
	printf("\n");
	return iter*4;

}

