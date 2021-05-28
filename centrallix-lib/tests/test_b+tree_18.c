#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

long long
test(char** tname)
    {
    int i,j,k,m; 
    int iter;
	int NUMLEVELS = 1;
	int LEVELNKEYS[] = {10, 10, 10, 10}; // all nodes in same level have same # of keys
	int LEVELNNODES[NUMLEVELS];
	LEVELNNODES[0] = 1;
	for (i=1; i<NUMLEVELS; i++) LEVELNNODES[i] = (LEVELNKEYS[i-1]+1)*(LEVELNNODES[i-1]);
	int TOTNODES = 0;
	for (i=0; i<NUMLEVELS; i++) TOTNODES += LEVELNNODES[i];
	int KEYLEN = 5;

	*tname = "b+tree-18 bptFree deinits large tree (configurable)";

	iter = 80000;
	for(i=0;i<iter;i++)
	 	{
		pBPNode nodes[TOTNODES];

		int nodeIndex = 0;

		// do all except assigning children, next, prev
		for (j=0; j<NUMLEVELS; j++)
			{
			for (k=0; k<LEVELNNODES[j]; k++)
				{
				nodes[nodeIndex] = bpt_i_new_BPNode();
				bptInit_I_Node(nodes[nodeIndex]);
				nodes[nodeIndex]->nKeys = LEVELNKEYS[j];
				nodes[nodeIndex]->IsLeaf = (j==NUMLEVELS-1);
				for (m=0; m<LEVELNKEYS[j]; m++)
					{
					nodes[nodeIndex]->Keys[m].Length = KEYLEN;
					nodes[nodeIndex]->Keys[m].Value = nmSysMalloc(KEYLEN);
					}
				
			
				nodeIndex++;
				}
			}

		int childIndex = 1;
		nodeIndex = 0;
		
		// assign children, next, prev
		for (j=0; j<NUMLEVELS; j++)
			{
			for (k=0; k<LEVELNNODES[j]; k++)
				{				
				if (!nodes[nodeIndex]->IsLeaf)
					{
					for (m=0; m<=LEVELNKEYS[j]; m++)
						{
						nodes[nodeIndex]->Children[m].Child = nodes[childIndex++];
						}
					}

				if (k>0) nodes[nodeIndex]->Prev = nodes[nodeIndex-1];
				if (j!=0 && k<(LEVELNKEYS[j]*(LEVELNKEYS[j-1]+1)-1)) nodes[nodeIndex]->Next = nodes[nodeIndex+1];
				nodeIndex++;
				}
			}

		bpt_I_FreeNode(nodes[0]);
		for (j=0; j<TOTNODES; j++)
			{
			assert (nodes[j]->Next == NULL);
			assert (nodes[j]->Prev == NULL);
			assert (nodes[j]->nKeys == 0);	
			}
		}
    return iter;
    }



