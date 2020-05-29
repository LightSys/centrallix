#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "b+tree.h"
#include "newmalloc.h"
long long
test(char** tname)
   	{
	int i, iter, tmp, idx;

	*tname = "b+tree-25 full test of bpt_i_Scan";
	iter = 50000;
	
	//cant recieve null ptr right?

	pBPTree root = bptNew();
        root->Keys[0].Length = 5;
        root->Keys[0].Value = "Allie\0"; 
	root->nKeys++;	
	root->Keys[1].Length = 4;
	root->Keys[1].Value = "Brad\0";
	root->nKeys++;
	root->Keys[2].Length = 7;
	root->Keys[2].Value = "Bradley\0";
	root->nKeys++;
	root->Keys[3].Length = 7;
	root->Keys[3].Value = "Brandon\0";
	root->nKeys++;
	root->Keys[4].Length = 7;
	root->Keys[4].Value = "Charlie\0";
	root->nKeys++;
	root->Keys[5].Length = 6;
        root->Keys[5].Value = "Debbie\0";
        root->nKeys++;
        root->Keys[6].Length = 7;
        root->Keys[6].Value = "Deborah\0";
        root->nKeys++;
        root->Keys[7].Length = 3;
        root->Keys[7].Value = "Eli\0";
        root->nKeys++;
        root->Keys[8].Length = 6;
        root->Keys[8].Value = "Elijah\0";
        root->nKeys++;
        root->Keys[9].Length = 5;
        root->Keys[9].Value = "Frank\0";
        root->nKeys++;

	pBPTree one = bptNew();
	one->Keys[0].Length = 5;
	one->Keys[0].Value = "Tommy\0";
	one->nKeys = 1;

	/*numbers as keys would not be in number line order
	pBPTree root2 = bptNew();
        root2->Keys[0].Length = 1;
        root2->Keys[0].Value = "1\0";
        root2->nKeys++;
        root2->Keys[1].Length = 1;
        root2->Keys[1].Value = "4\0";
        root2->nKeys++;
        root2->Keys[2].Length = 3;
        root2->Keys[2].Value = "100\0";
        root2->nKeys++;
        root2->Keys[3].Length = 3;
        root2->Keys[3].Value = "102\0";
        root2->nKeys++;
        root2->Keys[4].Length = 5;
        root2->Keys[4].Value = "72539\0";
        root2->nKeys++;*/


	for(i=0;i<iter;i++)
	 	{
		tmp = bpt_i_Scan(root, "Alex", 4, &idx);
		assert (tmp < 0);
		assert (idx == 0);
		tmp = bpt_i_Scan(root, "Allie", 5, &idx);
                assert (tmp == 0); 
                assert (idx == 0);
		tmp = bpt_i_Scan(root, "Charlie", 7, &idx);
                assert (tmp == 0);
                assert (idx == 4);
		tmp = bpt_i_Scan(root, "Deb", 3, &idx);
                assert (tmp < 0);  
                assert (idx == 5);
		tmp = bpt_i_Scan(root, "Debbie", 6, &idx);
                assert (tmp == 0);
		assert (idx == 5);
		tmp = bpt_i_Scan(root, "Elij", 4, &idx);
                assert (tmp < 0);
		assert (idx == 8);
		tmp = bpt_i_Scan(root, "Frank", 5, &idx);   
		assert (tmp == 0); 
		assert (idx == 9);
		tmp = bpt_i_Scan(root, "Franklin", 8, &idx); 
		assert (tmp > 0);
                assert (idx == 10);

		tmp = bpt_i_Scan(one, "T", 1, &idx);
                assert (tmp < 0);
                assert (idx == 0);
		tmp = bpt_i_Scan(one, "Tommy", 5, &idx); 
                assert (tmp == 0);      
		assert (idx == 0);
		tmp = bpt_i_Scan(one, "U", 1, &idx);
		assert (tmp > 0); 
		assert (idx == 1);

		/*tmp = bpt_i_Scan(root2, "0", 1, &idx);
                assert (tmp < 0);
                assert (idx == 0);
                tmp = bpt_i_Scan(root2, "1", 1, &idx);
                assert (tmp == 0);
                assert (idx == 0);
                tmp = bpt_i_Scan(root2, "3", 1, &idx);
                assert (tmp < 0);
                assert (idx == 1);
                tmp = bpt_i_Scan(root2, "100", 3, &idx);
                assert (tmp == 0);
                assert (idx == 2);
                tmp = bpt_i_Scan(root2, "101", 3, &idx);
                assert (tmp < 0);
                assert (idx == 3);*/

		}

	printf("\n");
	
    	return iter*4;
    	}

