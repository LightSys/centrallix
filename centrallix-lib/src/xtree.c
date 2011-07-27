#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
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

inline pXTreeNode xt_internal_InitNode(){
    pXTreeNode toReturn;
    toReturn = nmMalloc(sizeof(XTreeNode));
    if(toReturn){
        toReturn->greater = NULL;
        toReturn->less = NULL;
    }
    return toReturn;
}

inline void xt_internal_FreeNode(pXTreeNode toFree, int (*free_fn)(), void * freeFnArg){
    
    /** Do a postorder traversal of the tree to free data **/
    if(toFree->greater){
        xt_internal_FreeNode(toFree->greater, free_fn, freeFnArg);
    }
    if(toFree->less){
        xt_internal_FreeNode(toFree->greater, free_fn, freeFnArg);
    }
    if(free_fn){
        free_fn(toFree->data, freeFnArg);
    }
    nmFree(toFree, sizeof(XTreeNode));
}

inline int xt_internal_KeysSameBase(int from, char* key1, char* key2){
    int to = from;
    while(key1[to] == key2[to] && key1[to++] != 0){} /* Run through until they characters are both null chars or 0 */
    return to;
}

int xtInit(pXTree tree, int keyLength){
    tree->nItems = 0;
    tree->KeyLen = keyLength;
    tree->root = NULL;
    return 0;
}

int xtDeInit(pXTree tree){
    if(tree->root){
        xt_internal_FreeNode(tree->root, NULL, NULL);
    }
    return 0;
}

int xtAdd(pXTree this, char* key, char* data){
    pXTreeNode currentNode;
    char compareResult;
    
    if(this->root){
        currentNode = this->root;
        while(1){
            compareResult = strncmp(key, currentNode->key, this->KeyLen);
            if(compareResult < 0){
                if(currentNode->less){
                    currentNode = currentNode->less;
                }
                else{
                    currentNode->less = xt_internal_InitNode();
                    if(!currentNode->less){
                        return -1;
                    }
                    currentNode->less->data = data;
                    currentNode->less->key = key;
                    return 0;
                }
            }
            if(compareResult > 0){
                if(currentNode->greater){
                    currentNode = currentNode->greater;
                }
                else{
                    currentNode->greater = xt_internal_InitNode();
                    if(!currentNode->greater){
                        return -1;
                    }
                    currentNode->greater->data = data;
                    currentNode->greater->key = key;
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
        this->root = xt_internal_InitNode();
        if(!this->root){
            return -1;
        }
        this->root->key = key;
        this->root->data = data;
        return 0;
    }
}

int xtRemove(pXTree this, char* key){
    char comparisonResult;
    pXTreeNode currentNode = this->root, successorNode = NULL;
    pXTreeNode * parentNodePointer = &this->root, *successorNodeParent;
    
    while(currentNode){
        comparisonResult = strncmp(key, currentNode->key,this->KeyLen);
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
                while(successorNodeParent = &successorNode->greater, successorNode = successorNode->greater);
                *successorNodeParent = successorNode->less;
            }
            else if(currentNode->less){
                successorNode = currentNode->less;
            }
            else if(currentNode->greater){
                successorNode = currentNode->greater;
            }
            else{
                successorNode = NULL;
            }
            *parentNodePointer = successorNode;
            if(successorNode){
                successorNode->greater = currentNode->greater;
                successorNode->less = currentNode->less; 
            }
            currentNode->greater = NULL;
            currentNode->less = NULL;
            xt_internal_FreeNode(currentNode, NULL, NULL);
            return 0;
        }
    }
    return -1;
}

char* xtLookup(pXTree this, char* key){
    pXTreeNode currentNode;
    char compareResult;
    while(currentNode){
        compareResult = strncmp(key, currentNode->key, this->KeyLen);
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

int xtClear(pXTree this, int (*free_fn)(), void* free_arg){
    if(this->root){
        xt_internal_FreeNode(this->root, free_fn, free_arg);
    }
    return xtInit(this, this->KeyLen);
}

char* xtLookupBeginning(pXTree this, char* key){
    if(this->KeyLen == 0){
        int charFrom = 0, newCharFrom;
        pXTreeNode currentNode, closestParentNode = NULL;
        char compareResult;
        
        while(currentNode){
            if((newCharFrom = xt_internal_KeysSameBase(charFrom, key, currentNode->key)) > charFrom){
                charFrom = newCharFrom;
                closestParentNode = currentNode;
            }
            compareResult = strncmp(key, currentNode->key, this->KeyLen);
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
