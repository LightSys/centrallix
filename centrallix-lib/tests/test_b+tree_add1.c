#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 2- leaf insert";
	printf("\n Adding Test #1\n");

	pBPTree root = bpt_i_new_BPNode();

	char* key = "2\0";
	char* val = "second one\0";

	int t = bpt_i_Insert(root, key, 1, val, 0);
	int iter = 9000000, i;
	for(i = 0; i < iter; i++){

		assert(t==0);
	}
	
	bpt_PrintTree(&root);
	printf("\n");
	return iter*4;

}

