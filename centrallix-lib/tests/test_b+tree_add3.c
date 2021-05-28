#include <stdio.h>
#include <assert.h>
#include "string.h"
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char **tname){

	*tname = "b+tree add test 4 - insert duplicate";

	pBPTree tree = bptNew();

	char* key = "hello\0";
	char* val = "hopeful\0";
	int iter = 9000000, i, t;

	t = bptAdd(tree, key, 5, val);
	assert (t == 0);
        t = bptAdd(tree, key, 5, val);
        assert (t == -1);

	for(i = 0; i < iter; i++){
		assert (1 == 1);
	}


	return iter*4;

}
