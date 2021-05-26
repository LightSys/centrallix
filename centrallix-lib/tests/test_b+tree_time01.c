#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
   	{
	int i, iter, tmp;

	*tname = "b+tree-time01 testing time of searching tree size 10";
	iter = 4000000;
	
	pBPNode this = bptBulkLoad("tests/bpt_bl_10e1.dat", 10);
	pBPNode dummy = NULL;
	int idx = 0;
	
	
	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Find(this, "00000001", 8, &dummy, &idx);
                assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000002", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000003", 8, &dummy, &idx);
		assert (tmp == 0);    
		tmp = bpt_i_Find(this, "00000004", 8, &dummy, &idx);
		assert (tmp == 0); 
		tmp = bpt_i_Find(this, "00000005", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000006", 8, &dummy, &idx);
		assert (tmp == 0);  
		tmp = bpt_i_Find(this, "00000007", 8, &dummy, &idx);
		assert (tmp == 0);
		tmp = bpt_i_Find(this, "00000008", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00000009", 8, &dummy, &idx);
                assert (tmp == 0);
                tmp = bpt_i_Find(this, "00000010", 8, &dummy, &idx);
                assert (tmp == 0);
		}
	
    	return iter;//do I need *4
    	}

