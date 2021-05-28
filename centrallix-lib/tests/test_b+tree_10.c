#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "newmalloc.h"
#include "b+tree.h"

long long
test(char** tname)
    {
    int i;
    int iter;
	int ret_val;
	pBPTree tree;
	pBPNode leaf_node;

	*tname = "b+tree-10: Test bptInsert";

	iter = 100000;
	for(i=0;i<iter;i++)
	 	{
		leaf_node = bpt_i_new_BPNode();
		
		/** For explicitness **/
		leaf_node->IsLeaf = 1;
		tree = bptNew();
		tree->root = leaf_node;

		char key[] = "new_key";
		int key_1_value = 0xDEAD;

		/** Test that insert fails if input leaf_node is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(NULL, key, sizeof( key ), &key_1_value);
		assert( ret_val != 0 );


		/** Test that insert fails if key is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, NULL, sizeof( key ), &key_1_value);
		assert( ret_val != 0 );

		/** Test that insert fails if key size is 0 **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, key, 0, &key_1_value);
		assert( ret_val != 0 );

		/** Test that insert fails if data is NULL **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, key, 0, NULL);
		assert( ret_val != 0 );

		/** Test that insert fails if indekey_1_value is out of bounds (less than 0) **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, key, 0, &key_1_value);
		assert( ret_val != 0 );

		/** Test that insert fails if indekey_1_value is out of bounds (greater than nKeys) **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, key, 0, &key_1_value);
		assert( ret_val != 0 );

		/** Dummy test stub for assuming nmSysMalloc failed **/
		//global_fail_malloc = 1;
		//ret_val = bpt_i_Insert(leaf_node, key, 0, &x, 0);
		//assert( ret_val != 0 );

		/** Test a successful insertion at 0 **/
		assert(leaf_node->nKeys == 0);
		ret_val = bptInsert(tree, key, sizeof(key), &key_1_value);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 1);
		assert(0 == memcmp(key, leaf_node->Keys[ 0 ].Value, sizeof(key)));
		assert(leaf_node->Children[ 0 ].Ref == &key_1_value);

		/** Test a successful insertion at end **/
		char second_key[] = "second_key";
		int key_2_value = 0xBEEF;
		assert(leaf_node->nKeys == 1); /** assumes that a value has been already added **/
		ret_val = bptInsert(tree, second_key, sizeof(second_key), &key_2_value);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 2);
		assert(0 == memcmp(second_key, leaf_node->Keys[ 1 ].Value, sizeof(second_key)));
		assert(leaf_node->Children[ 1 ].Ref == &key_2_value);

		/** Test a successful insertion at in between two keys **/
		char mid_key[] = "mid_key";
		int mid_key_value = 0xABC;
		assert(leaf_node->nKeys == 2);
		ret_val = bptInsert(tree, mid_key, sizeof(mid_key), &mid_key_value);
		assert(ret_val == 0);
		assert(leaf_node->nKeys == 3);
		
		/** Test a successful insertion into an index node **/
		pBPNode index_node = bpt_i_new_BPNode();
		index_node->IsLeaf = 0;
		char leaf_node_key[] = "leaf_node";
		assert(index_node->nKeys == 0);
		}
    return iter;
    }
