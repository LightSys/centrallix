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

// Initialization and deinitialization functions
pXTreeNode xt_internal_CreateNode(){
    pXTreeNode toReturn;
    toReturn = nmSysMalloc(sizeof(XTreeNode));
    if(toReturn){
        toReturn->numObjs = 0;
        toReturn->firstNode = NULL;
    }
    return toReturn;
}

pXTreeLinkedListNode xt_internal_CreateLinkedListNode(){
    pXTreeLinkedListNode toReturn;
    toReturn = nmSysMalloc(sizeof(XTreeLinkedListNode));
    if(toReturn){
        toReturn->next = NULL;
        toReturn->node = NULL;
    }
    return toReturn;
}

char *xt_internal_NextChunk(char **str, char separator){
    char *result, *toReturn;
    result = strchr(*str, separator);
    if(result){
        toReturn = malloc(result - *str + 1);
        if(toReturn){
            strncpy(result, *str, result - *str);
            result[result - *str] = '\0';
        }
        return toReturn;
    }
    else{
        return strdup(*str);
    }
}

char xt_internal_ComapreSegment(char **path, char *other, char separator){
    while(*other == **path){
        if(**path == separator){
            *path++;
            return 1;
        }
        if(**path == '\0'){
            return 1;
        }
        other++;
        *path++;
    }
    return 0;
}

int xtInit(pXTree tree, char separator){
    tree->rootNode = xt_internal_CreateNode();
    if(tree){
        tree->separator = separator;
    }
    return tree != NULL;
}

int xtDeInit(pXTree tree){
    xtClear(tree, NULL, NULL);
    nmSysFree(tree->rootNode);
    return 0;
}

int xtAdd(pXTree this, char* key, char* data){
    
}

char xt_internal_RemoveInNode(pXTreeNode *ptrToNode, pXTreeNode node, char *key, char separator){
    if(node){
        pXTreeLinkedListNode currentLinkedListNode;
        pXTreeLinkedListNode *ptrToLinkedListNode;
        char *resetKey, toReturn;
        
        // Iterate through linked list trying to find the current key.
        ptrToLinkedListNode = &node->firstNode;
        currentLinkedListNode = node->firstNode;
        while(currentLinkedListNode){
            resetKey = key;
            if(xt_internal_ComapreSegment(&resetKey, currentLinkedListNode->key, separator)){
                if((toReturn = xt_internal_RemoveInNode(&currentLinkedListNode->node, currentLinkedListNode->node, resetKey, separator))){
                    node->numObjs--; // Decrement the reference count
                    if(node->numObjs == 0){
                        // Remove this linked list from the list
                        *ptrToNode = NULL;
                        nmSysFree(node);
                        free(currentLinkedListNode->key);
                        (*ptrToLinkedListNode)->next = currentLinkedListNode->next;
                    }
                }
                return toReturn;
            }
            // Check the next
            ptrToLinkedListNode = &currentLinkedListNode->next;
            currentLinkedListNode = currentLinkedListNode->next;
        }
        return 0;
    }
    else{
        if(*key == '\0'){
            return 1; // We hit the end of the path, so we did find it!
        }
        else{
            return 0; // We did not find the requested node!
        }
    }
}

int xtRemove(pXTree this, char* key){
    int toReturn;
    toReturn = !xt_internal_RemoveInNode(&this->rootNode, this->rootNode, key, this->separator);
    if(!this->rootNode){
        toReturn &= xtInit(this, this->separator);
    }
    return toReturn;
}

char *xt_internal_LookupInNode(pXTreeNode node, char *key, char separator){
    if(node){
        pXTreeLinkedListNode currentLinkedListNode;
        char *resetKey;
        
        // Iterate through linked list trying to find the current key.
        currentLinkedListNode = node->firstNode;
        while(currentLinkedListNode){
            resetKey = key;
            if(xt_internal_ComapreSegment(&resetKey, currentLinkedListNode->key, separator)){
                if(*key == '\0'){ // If we hit this, we have reached the end of the given path
                    return currentLinkedListNode->data;
                }
                else{
                    return xt_internal_LookupInNode(currentLinkedListNode->node, resetKey, separator);
                }
            }
            currentLinkedListNode = currentLinkedListNode->next;
        }
        return NULL;
    }
    else{
        return NULL;
    }
}

char *xtLookup(pXTree this, char* key){
    return xt_internal_LookupInNode(this->rootNode, key, this->separator);
}

int xtClear(pXTree this, int (*free_fn)(char *data, void *free_arg), void* free_arg){
    
}
    
char *xt_internal_LookupBeginning(pXTreeNode node, char *key, char separator){
    pXTreeLinkedListNode currentLinkedListNode;
    char *resetKey;

    // Iterate through linked list trying to find the current key.
    currentLinkedListNode = node->firstNode;
    while(currentLinkedListNode){
        resetKey = key;
        if(xt_internal_ComapreSegment(&resetKey, currentLinkedListNode->key, separator)){
            if(*key == '\0' || !currentLinkedListNode->node){ // If we hit this, we have reached the end of the given path
                return currentLinkedListNode->data;
            }
            else{
                return xt_internal_LookupInNode(currentLinkedListNode->node, resetKey, separator);
            }
        }
        currentLinkedListNode = currentLinkedListNode->next;
    }
    return NULL;
}

char* xtLookupBeginning(pXTree this, char* key){
    return xt_internal_LookupBeginning(this->rootNode, key, this->separator);
}

void xt_internal_Traverse(pXTreeNode node, void (*iterFunc)(char *key, char *data, void *userData), void *userData){
    pXTreeLinkedListNode currentLLNode = node->firstNode;
    
    //Iterate through all items in the current linked list
    while(currentLLNode){
        // Call function
        iterFunc(currentLLNode->key, currentLLNode->data, userData);
        if(currentLLNode->node){
            xt_internal_Traverse(currentLLNode->node, iterFunc, userData);
        }
        currentLLNode = currentLLNode->next;
    }
}

void xtTraverse(pXTree tree, void (*iterFunc)(char *key, char *data, void *userData), void *userData){
    xt_internal_Traverse(tree->rootNode, iterFunc, userData);
}
