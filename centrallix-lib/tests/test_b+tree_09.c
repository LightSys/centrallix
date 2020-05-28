#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "newmalloc.h"
#include "b+tree.h"

/** Declare internal function to test **/
pBPTree bpt_i_Split(pBPTree node, int split_loc);

static void create_leaf_node(pBPTree leaf_node, int nKeys, int max_key_len)
	{
	int * new_val;
	int j;
	for(j = 0; j < nKeys; j++)
		{
		leaf_node->Keys[ j ].Length = max_key_len;
		leaf_node->Keys[ j ].Value = nmMalloc(sizeof(char) * max_key_len);
		new_val = nmMalloc(sizeof(*new_val));
		*new_val = j;
		leaf_node->Children[ j + 1 ].Ref = new_val;
		snprintf(leaf_node->Keys[ j ].Value, max_key_len, "%d", j);
		}
	leaf_node->nKeys = BPT_SLOTS;
	}

static void validate_split(int split_loc, pBPTree leaf_node, pBPTree right_node, int total_nkeys, int max_key_len)
	{
	assert(right_node->IsLeaf == 1);
	assert(right_node->nKeys == BPT_SLOTS - split_loc);
	assert(right_node->Prev == leaf_node);
	assert(right_node->Next == NULL);
	assert(leaf_node->Next == right_node);
	assert(leaf_node->Prev == NULL);
	assert(leaf_node->nKeys == split_loc);

	char expected_key[ max_key_len ];
	int * expected_value;
	int adjusted_index;
	int j;
	for(j = 0; j < total_nkeys; j++)
		{
		snprintf(expected_key, max_key_len, "%d", j);
		if(j < split_loc)
			{
			expected_value = leaf_node->Children[ j + 1 ].Ref;
			assert(*expected_value == j);
			assert(0 == strncmp( leaf_node->Keys[ j ].Value, expected_key, max_key_len));
			nmFree(leaf_node->Keys[ j ].Value, sizeof(char) * max_key_len);
			nmFree(leaf_node->Children[ j + 1 ].Ref, sizeof(int));
			}
		else
			{
			adjusted_index = j - split_loc;
			expected_value = right_node->Children[ adjusted_index + 1 ].Ref;
			assert(*expected_value == j);
			assert(0 == strncmp(right_node->Keys[ adjusted_index ].Value, expected_key, max_key_len));
			nmFree(right_node->Keys[ adjusted_index ].Value, sizeof(char) * max_key_len);
			nmFree(right_node->Children[ adjusted_index + 1 ].Ref, sizeof(int));
			}
		}
	}

long long
test(char** tname)
    {
    int i;
    int iter;
	pBPTree leaf_node, right_node;
	int max_key_len = 2;

	*tname = "b+tree-09 bptSplit returns 0";

	iter = 1000;
	for(i=0;i<iter;i++)
	 	{
		leaf_node = bptNew();

		create_leaf_node(leaf_node, BPT_SLOTS, max_key_len);

		/** Test that no node is created if split location is < 0 **/
		right_node = bpt_i_Split(leaf_node, -1 );
		assert(right_node == NULL);

		/** Test that no node is created if split location is > nKeys **/
		right_node = bpt_i_Split(leaf_node, BPT_SLOTS + 1);
		assert(right_node == NULL);

		/** Test that node is successfully created if split location is valid **/ 
		right_node = bpt_i_Split(leaf_node, CEIL_HALF_OF_LEAF_SLOTS);
		validate_split(CEIL_HALF_OF_LEAF_SLOTS, leaf_node, right_node, BPT_SLOTS, max_key_len);
		nmFree(leaf_node, sizeof(*leaf_node));

		/** Test that node is successfully created if split location creates an empty left node **/ 
		leaf_node = bptNew();
		create_leaf_node(leaf_node, BPT_SLOTS, max_key_len);
		right_node = bpt_i_Split(leaf_node, 0);
		validate_split(0, leaf_node, right_node, BPT_SLOTS, max_key_len);
		nmFree(leaf_node, sizeof( *leaf_node));

		/** Test that node is successfully created if split location creates an empty left node **/ 
		leaf_node = bptNew();
		create_leaf_node(leaf_node, BPT_SLOTS, max_key_len);
		right_node = bpt_i_Split(leaf_node, BPT_SLOTS );
		validate_split(BPT_SLOTS, leaf_node, right_node, BPT_SLOTS, max_key_len);
		nmFree(leaf_node, sizeof(*leaf_node));

		/** Test that node is successfully created if split before node is full (tests case of index node) **/ 
		leaf_node = bptNew();
		create_leaf_node(leaf_node, IDX_SLOTS, max_key_len);
		right_node = bpt_i_Split(leaf_node, CEIL_HALF_OF_IDX_SLOTS);
		validate_split(CEIL_HALF_OF_IDX_SLOTS, leaf_node, right_node, IDX_SLOTS, max_key_len);
		nmFree(leaf_node, sizeof(*leaf_node));
		}
    return iter;
    }

