#ifndef XTREE_H
#define	XTREE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	xtree.c, xtree.h  					*/
/* Author:	Daniel Rothfus  					*/
/* Creation:	July 26, 2011   					*/
/* Description:	Tree based associative table for centrallix-lib.  This  */
/* 		enables one to find the closest match if so desired.    */
/************************************************************************/

/** \defgroup xtree XTree
 \brief A basic tree-based map module.

 Please note that you are responsible for the memory management of the keys and
 data you pass into the trees.  The tree does not copy or otherwise back up the
 data or keys!
 \{
 */

typedef struct _XTLLND  XTreeLinkedListNode,    *pXTreeLinkedListNode;
typedef struct _XTND    XTreeNode,              *pXTreeNode;

struct _XTLLND{
    char                        *key;
    char                        *data;
    pXTreeLinkedListNode        next;
    pXTreeNode                  node;
};

/** \brief A node in an XTree object. 
 * 
 This is an internal data type.  Please do not use this!
 */
struct _XTND{
    pXTreeLinkedListNode        firstNode;
    unsigned int                numObjs;
};

/** \brief The XTree object.
 
 The nItems field should be updated regularly, but it is not that useful to
 most people, so a function was not added to access this data, so only access
 this if you must at the risk of your program being broken if the member is 
 removed!
 */
typedef struct{
    pXTreeNode                  rootNode;
    char                        separator;
} XTree, *pXTree;

/** \brief Initialize an already allocated tree.
 \param tree The tree to set up with valid initial values.
 \param keyLength The length of the key that will be passed in.  If this is 0,
 then the key is assumed to be null terminated,  otherwise the first keyLength
 characters are always compared.
 \return This returns 0 if it was successful or negative on failure.
 */
int xtInit(pXTree tree, char separator);

/** \brief Deinitialize a tree.  This gets rid of the internal data with no
 free function.
 \param tree The tree object to kill.
 \return This returns 0 when the function succeeds and negative otherwise.
 */
int xtDeInit(pXTree tree);

/** \brief Add a new associated pair to the tree. 
 
 If the key already exists in the tree, this replaces the data associated with
 the key with the new data.
 \param this The tree to add the data and the key to.
 \param key The key to add to the tree.
 \param data The data to associate with key key.
 \return This returns 0 on success and negative on failure.
 */
int xtAdd(pXTree this, char* key, char* data);

/** \brief Remove a key and its associated data from the tree.
 \param this The tree to remove data from.
 \param key The key to remove along with its associated data.
 \return This return 0 on success and negative on error.  A common error is not
 finding the object to remove.
 */
int xtRemove(pXTree this, char* key);

/** \brief Find the data associated with a certain key in the tree.
 \param tree The tree to find the data in.
 \param key The key to find the data for.
 \return This returns the data on success or NULL on error, though the data
 could be NULL if you are crazy about what you insert!
 */
char* xtLookup(pXTree this, char* key);

/** \brief Clear all things in the tree using a certain function.
 
 The function should look somewhat like void freeFunction(char * data, void *
 freeArg){..}.
 \param free_fn The function to pass the data to to free.
 \param free_arg A custom argument to pass to the function.
 \return This returns 0 on success and negative on error.
 */
int xtClear(pXTree this, int (*free_fn)(char *data, void *free_arg), void* free_arg);

/** \brief Lookup the item with the largest common beginning string.  This does
 not work on keys that are of a set length (Meaning you set the keyLength to 
 soemthing other than 0 on xtInit.)
 
 For example, if you gave key as "/asdf/asdf.dumb", and set keys "/asdf", 
 "/sdf", and "mwahaha!", this would return the data for "/asdf/".  If you had
 set "/asdf/asdf.dumb", then that would be returned.
 \param this The tree to look the data up in.
 \param key The key to look up the nearest data by.
 */
char* xtLookupBeginning(pXTree this, char* key);

/** \brief Traverse over all elements in an XTree in order of the key.
 \param tree The tree to traverse over.
 \param iterFunc The function that will iterate over the data in the tree.  The
 first argument of this function is the key, the second the data, and the third
 is the user data passed to the xtTraverse function.
 \param userData The user data passed to the function.
 */
void xtTraverse(pXTree tree, void (*iterFunc)(char *key, char *data, void *userData), void *userData);

/** \}
 */

#endif	/* XTREE_H */

