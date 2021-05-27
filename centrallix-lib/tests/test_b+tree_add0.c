#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 1 - leaf insert";
	printf("\n Adding Test #0\n");

	pBPTree tree = bptNew();

	char key[] = "hello\0";
	char val[] = "hopeful\0";

	pBPNode root = tree->root;

	int t = bptInsert(tree, key, 5, val);
	int iter = 9000000, i;
	char* str = (char*)root->Children[0].Ref;
	for(i = 0; i < iter; i++){
		assert (str == val);
		assert(t==0);
	}
	
	printf("\n");
	printTree(root,1);
	printf("\n");
	return iter*4;

}
