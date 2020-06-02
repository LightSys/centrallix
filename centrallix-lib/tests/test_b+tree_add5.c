#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 6 - insert partial (a-f) alphabet in order ";

	pBPTree root = NULL;
	int t, i, iter = 9000000;
	
	char* data = "data";

	t = bptAdd(&root, "a", 1, data);
		assert (t == 0);
		t = bptAdd(&root, "b", 1, data);
                assert (t == 0);
		t = bptAdd(&root, "c", 1, data);
                assert (t == 0);
		t = bptAdd(&root, "d", 1, data);
                assert (t == 0);
		t = bptAdd(&root, "e", 1, data);
                assert (t == 0);
		t = bptAdd(&root, "f", 1, data);
                assert (t == 0);
	
	for(i = 0; i < iter; i++)
		{
		assert (0 == 0);
		}
		
	bpt_PrintTree(&root);
	printf("\n");
	return iter*4;

}

