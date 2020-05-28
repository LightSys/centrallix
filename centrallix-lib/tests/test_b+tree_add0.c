#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 1 - leaf insert";
	printf("\n Adding Test #0\n");

	pBPTree root = bptNew();

	char key[] = "hello\0";
	char val[] = "hopeful\0";

	int t = bpt_i_LeafInsert(root, key, 5, val, 0);
	int iter = 9000000, i;
	for(i = 0; i < iter; i++){

		assert(t==0);
	}
	
	printf("\n");
	bpt_PrintTree(root);
	printf("\n");
	return iter*4;

}
