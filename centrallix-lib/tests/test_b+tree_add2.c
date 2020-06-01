#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 3 - insert with bptAdd";

	pBPTree root = bptNew();

	char* key = "hi\0";
	char* val = "hopeful\0";

	int t = bptAdd(root, key, 2, val);
	int iter = 9000000, i;
	

	for (i=0; i<iter; i++)
		assert (t == 0);
	bpt_PrintTree(root);
	printf("\n");
	return iter*4;

}

