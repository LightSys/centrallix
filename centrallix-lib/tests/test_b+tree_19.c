#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "b+tree.h"
#include "newmalloc.h"

int KEYLEN = 3;
int VALLEN = 5;

int numFreed = 0;

int my_free_fn(void* free_arg, char* free_val)
	{
	nmSysFree(free_val);
	numFreed++;
	return 0;
	}

long long
test(char** tname)
    {
    int i,j,k,m; 
    int iter;
	int NUMLEVELS = 4;
	int LEVELNKEYS[] = {3, 2, 1, 3}; // all nodes in same level have same # of keys
	int LEVELNNODES[NUMLEVELS];
	LEVELNNODES[0] = 1;
	for (i=1; i<NUMLEVELS; i++) LEVELNNODES[i] = (LEVELNKEYS[i-1]+1)*(LEVELNNODES[i-1]);
	int TOTNODES = 0;
	for (i=0; i<NUMLEVELS; i++) TOTNODES += LEVELNNODES[i];
	char* KEYFMTSTR = malloc(sizeof(char)*10);
	sprintf(KEYFMTSTR, "%%0%dd", KEYLEN-1);
	char* VALFMTSTR = malloc(sizeof(char)*10);
	sprintf(VALFMTSTR, "%%0%dd", VALLEN-1);

	*tname = "b+tree-19 bptClear works on large tree (configurable)";

	iter = 8000;
	for(i=0;i<iter;i++)
	 	{
		pBPTree nodes[TOTNODES];

		int nodeIndex = 0;
		int parentIndex = 0;

		// do all except assigning children, next, prev
		for (j=0; j<NUMLEVELS; j++)
			{
			for (k=0; k<LEVELNNODES[j]; k++)
				{
				nodes[nodeIndex] = bptNew();
				bptInit(nodes[nodeIndex]);
				nodes[nodeIndex]->nKeys = LEVELNKEYS[j];
				nodes[nodeIndex]->IsLeaf = (j==NUMLEVELS-1);
				for (m=0; m<LEVELNKEYS[j]; m++)
					{
					nodes[nodeIndex]->Keys[m].Length = KEYLEN; // TODO
					nodes[nodeIndex]->Keys[m].Value = nmSysMalloc(KEYLEN);
					sprintf(nodes[nodeIndex]->Keys[m].Value, KEYFMTSTR, nodeIndex);
					}
				
				if (j!=0)
					{
					nodes[nodeIndex]->Parent = nodes[parentIndex];
					if ((k+1)%(LEVELNKEYS[j-1]+1)==0) parentIndex++;
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
				else
					{
						for (m=0; m<=LEVELNKEYS[j]; m++)
						{
						nodes[nodeIndex]->Children[m].Ref = nmSysMalloc(VALLEN);
						sprintf(nodes[nodeIndex]->Children[m].Ref, VALFMTSTR, childIndex++);
						}
					}

				if (k>0) nodes[nodeIndex]->Prev = nodes[nodeIndex-1];
				if (j!=0 && k<(LEVELNKEYS[j]*(LEVELNKEYS[j-1]+1)-1)) nodes[nodeIndex]->Next = nodes[nodeIndex+1];
				nodeIndex++;
				}
			}

		numFreed = 0;
		bptClear(nodes[0], *my_free_fn, NULL);

		assert(numFreed == LEVELNNODES[NUMLEVELS-1]*LEVELNKEYS[NUMLEVELS-1]);
		// Would be best if we could also test that nmFrees were performed
		assert (nodes[0]->Parent == NULL);
		assert (nodes[0]->Next == NULL);
		assert (nodes[0]->Prev == NULL);
		assert (nodes[0]->nKeys == 0);
		assert (nodes[0]->IsLeaf == 1);
		}
    	return iter;
    }



