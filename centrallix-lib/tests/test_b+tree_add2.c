#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 3 - insert with bptAdd";

	pBPTree tree = bptNew();

	char* key = "hi\0";
	char* val = "hopeful\0";

	int t = bptAdd(tree, key, 2, val);
	int iter = 9000000, i;


	for (i=0; i<iter; i++)
		assert (t == 0);
		
	return iter*4;

}
