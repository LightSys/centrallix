#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "newmalloc.h"
#include "b+tree.h"

/** Declare internal function to test **/
int bpt_i_Insert(pBPTree this, char* key, int key_len, void* data, int idx);

long long
test(char** tname)
    {
    int i;
    int iter;
	int ret_val;
	pBPTree leaf_node;

	*tname = "b+tree-10: Test bpt_i_Insert";

	iter = 100000;
	for(i=0;i<iter;i++)
	 	{
		leaf_node = bptNew();

		/** For explicitness **/
		leaf_node->IsLeaf = 1;

		char key[] = "new_key";
		int key_1_value = 0xDEAD;

		/** Test that insert fails if input leaf_node is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(NULL, key, sizeof( key ), &key_1_value, 0);
		assert( ret_val != 0 );

		/** Test that insert fails if key is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, NULL, sizeof( key ), &key_1_value, 0);
		assert( ret_val != 0 );

		/** Test that insert fails if key size is 0 **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, key, 0, &key_1_value, 0);
		assert( ret_val != 0 );

		/** Test that insert fails if data is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, key, 0, NULL, 0);
		assert( ret_val != 0 );

		/** Test that insert fails if indekey_1_value is out of bounds (less than 0) **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, key, 0, &key_1_value, -1);
		assert( ret_val != 0 );

		/** Test that insert fails if indekey_1_value is out of bounds (greater than nKeys) **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, key, 0, &key_1_value, 1);
		assert( ret_val != 0 );

		/** Dummy test stub for assuming nmSysMalloc failed **/
		//global_fail_malloc = 1;
		//ret_val = bpt_i_Insert(leaf_node, key, 0, &x, 0);
		//assert( ret_val != 0 );

		/** Test a successful insertion at 0 **/
		assert(leaf_node->nKeys == 0);
		ret_val = bpt_i_Insert(leaf_node, key, sizeof(key), &key_1_value, 0);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 1);
		assert(0 == memcmp(key, leaf_node->Keys[ 0 ].Value, sizeof(key)));
		assert(leaf_node->Children[ 0 ].Ref == &key_1_value);

		/** Test a successful insertion at end **/
		char second_key[] = "second_key";
		int key_2_value = 0xBEEF;
		assert(leaf_node->nKeys == 1); /** assumes that a value has been already added **/
		ret_val = bpt_i_Insert(leaf_node, second_key, sizeof(second_key), &key_2_value, 1);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 2);
		assert(0 == memcmp(second_key, leaf_node->Keys[ 1 ].Value, sizeof(second_key)));
		assert(leaf_node->Children[ 1 ].Ref == &key_2_value);

		/** Test a successful insertion at in between two keys **/
		char mid_key[] = "mid_key";
		int mid_key_value = 0xABC;
		assert(leaf_node->nKeys == 2);
		ret_val = bpt_i_Insert(leaf_node, mid_key, sizeof(mid_key), &mid_key_value, 1);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 3);
		assert(0 == memcmp(mid_key, leaf_node->Keys[ 1 ].Value, sizeof(mid_key)));
		assert(leaf_node->Children[ 1 ].Ref == &mid_key_value);

		/** Test a successful insertion into an index node **/
		pBPTree index_node = bptNew();
		index_node->IsLeaf = 0;
		char leaf_node_key[] = "leaf_node";
		assert(leaf_node->Parent == NULL);
		assert(index_node->nKeys == 0);
		bpt_i_Insert(index_node, leaf_node_key, sizeof(leaf_node_key), leaf_node, 0 );
		assert(index_node->nKeys == 1);
		assert(index_node->Children[ 1 ].Child == leaf_node);
		assert(index_node->Children[ 1 ].Child->Parent == index_node);
		assert(0 == memcmp(leaf_node_key, index_node->Keys[ 0 ].Value, sizeof(leaf_node_key)));
		}
    return iter;
    }
