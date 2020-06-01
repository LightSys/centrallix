#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 2 - insert duplicate";

	BPTreeRoot root;
	bptInitRoot(&root);

	char* key = "hello\0";
	char* val = "hopeful\0";

	int t = bptAdd(&root, key, 5, val);
	t = bptAdd(&root, key, 5, val);
	int iter = 9000000, i;
	printf("%s%d\n", "t: ", t);
	//for(i = 0; i < iter; i++){

		assert(t==1);
	//}
	
	//bpt_PrintTree(root);
	printf("\n");
	return iter*4;

}


