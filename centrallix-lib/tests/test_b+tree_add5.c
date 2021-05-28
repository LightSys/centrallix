#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 6 - insert partial (a-f) alphabet in order ";

	pBPTree tree = bptNew();
	int t, i, iter = 9000000;
	
	char* data = "data";

	t = bptAdd(tree, "a", 1, data);
		assert (t == 0);
		t = bptAdd(tree, "b", 1, data);
                assert (t == 0);
		t = bptAdd(tree, "c", 1, data);
                assert (t == 0);
		t = bptAdd(tree, "d", 1, data);
                assert (t == 0);
		t = bptAdd(tree, "e", 1, data);
                assert (t == 0);
		t = bptAdd(tree, "f", 1, data);
                assert (t == 0);
	
	for(i = 0; i < iter; i++)
		{
		assert (0 == 0);
		}
		
	return iter*4;

}

