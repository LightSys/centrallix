#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	printf("\n");
	int iter, i, tmp;
	int d1, d2, d3;
	int idx;
        pBPTree dummy;

	*tname = "b+tree-41 Analyze depth of tree";
	iter = 8000000;
	
	pBPTree this1 = bptBulkLoad("tests/bpt_bl_10e1.dat", 10);
	depthG = 0;
	tmp = bpt_i_Find(this1, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this1, "00000005", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this1, "00000010", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 10\tdepth = %d\n", d2);
	
	pBPTree this2 = bptBulkLoad("tests/bpt_bl_10e2.dat", 100);
	depthG = 0;
	tmp = bpt_i_Find(this2, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this2, "00000050", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this2, "00000100", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 100\tdepth = %d\n", d2);


	pBPTree this3 = bptBulkLoad("tests/bpt_bl_10e3.dat", 1000);
	depthG = 0;
	tmp = bpt_i_Find(this3, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this3, "00000500", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this3, "00001000", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 1000\tdepth = %d\n", d2);

	pBPTree this4 = bptBulkLoad("tests/bpt_bl_10e4.dat", 10000);
	depthG = 0;
	tmp = bpt_i_Find(this4, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this4, "00005000", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this4, "00010000", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 10000\tdepth = %d\n", d2);

	pBPTree this5 = bptBulkLoad("tests/bpt_bl_10e5.dat", 100000);
	depthG = 0;
	tmp = bpt_i_Find(this5, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this5, "00050000", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this5, "00100000", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 100000\tdepth = %d\n", d2);

	pBPTree this6 = bptBulkLoad("tests/bpt_bl_10e6.dat", 1000000);
	depthG = 0;
	tmp = bpt_i_Find(this6, "00000001", 8, &dummy, &idx);
	assert (tmp == 0);	
	d1 = depthG;
	depthG = 0;
	tmp = bpt_i_Find(this6, "00500000", 8, &dummy, &idx); 
	assert (tmp == 0);  
	d2 = depthG; 
	depthG = 0;   
	tmp = bpt_i_Find(this6, "01000000", 8, &dummy, &idx);            
	assert (tmp == 0);                   
	d3 = depthG;
	assert (d1 == d2);
	assert (d2 == d3);
	printf("n = 1000000\tdepth = %d\n", d2);
	
	
	for(i=0;i<iter;i++)
	 	{
		assert (0 == 0);
		}

	printf("\n");
	
    	return iter*4;
    	}

