#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 5 - insert alphabet in order ";

	pBPTree tree = bptNew();
	int t, i, iter = 9000000;
	char* data = "data";

	t = bptInsert(tree, "a", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "b", 1, data);
                assert (t == 0);
		t = bptInsert(tree, "c", 1, data);
                assert (t == 0);
		t = bptInsert(tree, "d", 1, data);
                assert (t == 0);
		t = bptInsert(tree, "e", 1, data);
                assert (t == 0);
		t = bptInsert(tree, "f", 1, data);
                assert (t == 0);
                t = bptInsert(tree, "g", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "h", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "i", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "j", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "k", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "l", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "m", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "n", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "o", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "p", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptInsert(tree, "q", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptInsert(tree, "r", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptInsert(tree, "s", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "t", 1, data);
		assert (t == 0);
		t = bptInsert(tree, "u", 1, data);
		assert (t == 0);
                t = bptInsert(tree, "v", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptInsert(tree, "w", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptInsert(tree, "x", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptInsert(tree, "y", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptInsert(tree, "z", 1, data);                                                                                                                                                                              assert (t == 0);

	for(i = 0; i < iter; i++)
		{
		assert (strcmp(tree->root->Children[0].Child->Keys[0].Value, "a") == 0);
		assert (strcmp((char*)tree->root->Children[0].Child->Children[0].Ref, "data") == 0);
		}

	return iter*4;

}
