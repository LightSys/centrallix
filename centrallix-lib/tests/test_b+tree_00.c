#include <stdio.h>
#include <assert.h>
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	int tmp;

	*tname = "b+tree-00 CURRENTLY A DUMMY TEST";
	iter = 8000000;
	for(i=0;i<iter;i++)
	    {
		tmp = bpt_i_Fake();
		assert (tmp == 0);
	    }

	printf("\nhello world\n\n");

    return iter*4;
    }

