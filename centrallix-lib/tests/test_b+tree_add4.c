#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 5 - insert alphabet in order ";

	pBPTree tree = NULL;
	int t, i, iter = 9000000;
	char* data = "data";

	t = bptAdd(&tree, "a", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "b", 1, data);
                assert (t == 0);
		t = bptAdd(&tree, "c", 1, data);
                assert (t == 0);
		t = bptAdd(&tree, "d", 1, data);
                assert (t == 0);
		t = bptAdd(&tree, "e", 1, data);
                assert (t == 0);
		t = bptAdd(&tree, "f", 1, data);
                assert (t == 0);
                t = bptAdd(&tree, "g", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "h", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "i", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "j", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "k", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "l", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "m", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "n", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "o", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "p", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(&tree, "q", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(&tree, "r", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(&tree, "s", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "t", 1, data);
		assert (t == 0);
		t = bptAdd(&tree, "u", 1, data);
		assert (t == 0);
                t = bptAdd(&tree, "v", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(&tree, "w", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(&tree, "x", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(&tree, "y", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(&tree, "z", 1, data);                                                                                                                                                                              assert (t == 0);

	for(i = 0; i < iter; i++)
		{
		assert (strcmp(tree->Children[0].Child->Keys[0].Value, "a") == 0);
		assert (strcmp((char*)tree->Children[0].Child->Children[0].Ref, "data") == 0);
		}

	bpt_PrintTree(&tree);
	printf("\n");
	return iter*4;

}
