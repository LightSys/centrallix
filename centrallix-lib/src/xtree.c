#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "newmalloc.h"
#include "xtree.h"


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

inline pXTreeNode xt_internal_AllocAndInitNode(){
    pXTreeNode toReturn;
    toReturn = nmMalloc(sizeof(XTreeNode));
    if(toReturn){
        toReturn->greater = NULL;
        toReturn->less = NULL;
    }
    return toReturn;
}

inline void xt_internal_FreeNode(pXTreeNode toFree, int (*free_fn)(), void * freeFnArg){
    
    if(toFree){
        /** Do a postorder traversal of the tree to free data **/
        xt_internal_FreeNode(toFree->greater, free_fn, freeFnArg);
        xt_internal_FreeNode(toFree->less, free_fn, freeFnArg);
        if(free_fn){
            free_fn(toFree->data, freeFnArg);
        }
        nmFree(toFree, sizeof(XTreeNode));
    }
}

inline int xt_internal_KeysSameBase(char* key1, char* key2){
    int to = 0;
    while(key1[to] == key2[to] && key1[to++] != 0){} /* Run through until they characters are both null chars or 0 */
    return to;
}

inline int xt_internal_KeysCompare(char *key1, char *key2, int keyLength){
    if(keyLength == INT_MAX){
        return strncmp(key1, key2, keyLength);
    }
    else{
        int c = -1;
        char compareResult;
        while(c++ < keyLength){
             compareResult = key1[c] - key2[c];
             if(compareResult != 0){
                 return compareResult;
             }
        }
        return 0;
    }
}

/** \brief A recursive function for traversing the tree in an inorder traversal.
 */
inline void xt_internal_Traverse(pXTreeNode node, void (*iterFunc)(char *key, char *data, void *userData), void *userData){
    if(node){
        xt_internal_Traverse(node->less, iterFunc, userData);
        iterFunc(node->key, node->data, userData);
        xt_internal_Traverse(node->greater, iterFunc, userData);
    }
}

int xtInit(pXTree tree, int keyLength){
    tree->nItems = 0;
    tree->KeyLen = keyLength ? keyLength : INT_MAX;
    tree->root = NULL;
    return 0;
}

int xtDeInit(pXTree tree){
    if(tree->root){
        xt_internal_FreeNode(tree->root, NULL, NULL);
    }
    return xtInit(tree, tree->KeyLen);
}

int xtAdd(pXTree this, char* key, char* data){
    pXTreeNode currentNode;
    int compareResult;
    
    if(this->root){
        currentNode = this->root;
        while(1){
            compareResult = xt_internal_KeysCompare(key, currentNode->key, this->KeyLen);
            if(compareResult < 0){
                if(currentNode->less){
                    currentNode = currentNode->less;
                }
                else{
                    currentNode->less = xt_internal_AllocAndInitNode();
                    if(!currentNode->less){
                        return -1;
                    }
                    currentNode->less->data = data;
                    currentNode->less->key = key;
                    this->nItems++;
                    return 0;
                }
            }
            else if(compareResult > 0){
                if(currentNode->greater){
                    currentNode = currentNode->greater;
                }
                else{
                    currentNode->greater = xt_internal_AllocAndInitNode();
                    if(!currentNode->greater){
                        return -1;
                    }
                    currentNode->greater->data = data;
                    currentNode->greater->key = key;
                    this->nItems++;
                    return 0;
                }
            }
            else{ /* The two are equal - overwrite */
                currentNode->data = data;
                currentNode->key = key; /* For memory purposes */
                return 0;
            }
        }
    }
    else{
        this->root = xt_internal_AllocAndInitNode();
        if(!this->root){
            return -1;
        }
        this->root->key = key;
        this->root->data = data;
        this->nItems++;
        return 0;
    }
}

int xtRemove(pXTree this, char* key){
    int comparisonResult;
    pXTreeNode currentNode = this->root, successorNode = NULL;
    pXTreeNode* parentNodePointer = &this->root, *successorNodeParent;
    
    while(currentNode){
        comparisonResult = xt_internal_KeysCompare(key, currentNode->key,this->KeyLen);
        if(comparisonResult < 0){
            parentNodePointer = &currentNode->less;
            currentNode = currentNode->less;
        }
        else if(comparisonResult > 0){
            parentNodePointer = &currentNode->greater;
            currentNode = currentNode->greater;
        }
        else{                                    /* Found node - delete it */
            if(currentNode->greater && currentNode->less){
                /* Find successor - Could randomize which side this takes stuff from eventually! */
                successorNode = currentNode->less;
                successorNodeParent = &currentNode->less;
                while(successorNode->greater){
                    successorNodeParent = &successorNode->greater;
                    successorNode = successorNode->greater;
                }
                *successorNodeParent = successorNode->less;
                successorNode->less = currentNode->less;
                successorNode->greater = currentNode->greater;
                //printf("Swapping successors\n");
            }
            else if(currentNode->less){
                successorNode = currentNode->less;
                //printf("Taking less node\n");
            }
            else if(currentNode->greater){
                successorNode = currentNode->greater;
                //printf("Taking greater node\n");
            }
            else{
                //printf("Taking no node\n");
                successorNode = NULL;
            }
            *parentNodePointer = successorNode;
            
            /** Free the node that is it that we found **/
            currentNode->greater = NULL;
            currentNode->less = NULL;
            xt_internal_FreeNode(currentNode, NULL, NULL);
            //printf("Freeing items\n");
            this->nItems--;
            return 0;
        }
    }
    return -1;
}

char* xtLookup(pXTree this, char* key){
    pXTreeNode currentNode = this->root;
    int compareResult;
    while(currentNode){
        compareResult = xt_internal_KeysCompare(key, currentNode->key, this->KeyLen);
        if(compareResult < 0){
            currentNode = currentNode->less;
        }
        else if(compareResult > 0){
            currentNode = currentNode->greater;
        }
        else{
            break;
        }
    }
    if(currentNode){
        return currentNode->data;
    }
    else{
        return NULL;
    }
}

int xtClear(pXTree this, int (*free_fn)(char *data, void *free_arg), void* free_arg){
    if(this->root){
        xt_internal_FreeNode(this->root, free_fn, free_arg);
    }
    return xtInit(this, this->KeyLen);
}

char* xtLookupBeginning(pXTree this, char* key){
    if(this->KeyLen == INT_MAX){
        int charFrom = 0, newCharFrom;
        pXTreeNode currentNode = this->root, closestParentNode = NULL;
        int compareResult;
        
        while(currentNode){
            if((newCharFrom = xt_internal_KeysSameBase(key, currentNode->key)) > charFrom){
                charFrom = newCharFrom;
                closestParentNode = currentNode;
            }
            compareResult = xt_internal_KeysCompare(key, currentNode->key, this->KeyLen);
            if(compareResult < 0){
                currentNode = currentNode->less;
            }
            else if(compareResult > 0){
                currentNode = currentNode->greater;
            }
            else{
                break;
            }
        }
        if(currentNode){
            return currentNode->data;
        }
        else{
            if(closestParentNode){
                return closestParentNode->data;
            }
            else{
                return NULL;
            }
        }
    }
    else{
        return NULL; /* It would not really make sense to do this comparison on anything but a string. */
    }
}

void xtTraverse(pXTree tree, void (*iterFunc)(char *key, char *data, void *userData), void *userData){
    xt_internal_Traverse(tree->root, iterFunc, userData);
}
