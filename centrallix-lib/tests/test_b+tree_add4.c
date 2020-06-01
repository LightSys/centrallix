#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){


	*tname = "b+tree add test 5 - insert alphabet in order ";

	pBPTree root = bptNew();
	int t, i, iter = 9000000;
	
	char* data = "data";

	t = bptAdd(root, "a", 1, data);
		assert (t == 0);
		t = bptAdd(root, "b", 1, data);
                assert (t == 0);
		t = bptAdd(root, "c", 1, data);
                assert (t == 0);
		t = bptAdd(root, "d", 1, data);
                assert (t == 0);
		t = bptAdd(root, "e", 1, data);
                assert (t == 0);
		t = bptAdd(root, "f", 1, data);
                assert (t == 0);
                t = bptAdd(root, "g", 1, data);
		assert (t == 0);  
                t = bptAdd(root, "h", 1, data);
		assert (t == 0); 
		t = bptAdd(root, "i", 1, data);
		assert (t == 0);
		t = bptAdd(root, "j", 1, data);
		assert (t == 0);
		t = bptAdd(root, "k", 1, data); 
		assert (t == 0);
                t = bptAdd(root, "l", 1, data);
		assert (t == 0);
                t = bptAdd(root, "m", 1, data);      
		assert (t == 0);
                t = bptAdd(root, "n", 1, data); 
		assert (t == 0);
                t = bptAdd(root, "o", 1, data);
		assert (t == 0);
                t = bptAdd(root, "p", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(root, "q", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(root, "r", 1, data);                                                                                                                                                                                    assert (t == 0);
                t = bptAdd(root, "s", 1, data); 
		assert (t == 0);
		t = bptAdd(root, "t", 1, data);
		assert (t == 0);
		t = bptAdd(root, "u", 1, data);          
		assert (t == 0);
                t = bptAdd(root, "v", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(root, "w", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(root, "x", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(root, "y", 1, data);                                                                                                                                                                              assert (t == 0);
                t = bptAdd(root, "z", 1, data);                                                                                                                                                                              assert (t == 0);
	
	for(i = 0; i < iter; i++)
		{
		assert (0 == 0);
		}
		
	bpt_PrintTree(root);
	printf("\n");
	return iter*4;

}


