#ifdef HAVE_CONFIG_H
#include "cxlibconfig-internal.h"
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "newmalloc.h"
#include "xtree.h"
#include "xhash.h"
#include "xarray.h"
#include "mtask.h"
#include "mtsession.h"


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
#define FLG_NODE (void *)0xdeafcab9
#define FLG_ROOT (void *)0xfea7beef

//util for path conversions

//breaks key into array over sep
pXArray keytopath(char *key, char sep){
    char *tmp, *del, **rr=0;
    pXArray path = nmMalloc(sizeof(XArray));
    xaInit(path,8);
    del = malloc(2);
    del[0] = sep;
    del[1] = 0;
    key = strdup(key);
    tmp = strtok(key,del);
    while(1){
        if(!tmp)break;
        xaAddItem(path,nmSysStrdup(tmp));
        tmp = strtok(NULL,del);
    }//end break path
    free(del);
    free(key);
    return path;
}//end keyto path

//rebuilds key from a path (using sep)
char *pathtokey(pXArray path, char sep){
    int i;
    char *key, *del;
    int length=0;
    del = malloc(2);
    del[0] = sep;
    del[1] = 0;
    for(i=0;i<xaCount(path);i++)
        length += strlen((char*)xaGetItem(path,i));
    key = nmSysMalloc(length+i);
    for(i=0;i<xaCount(path);i++){
        strcat(key,del);
        strcat(key,xaGetItem(path,i));
    }
    free(del);
    return key;
}//end pathtokey

//frees an array such as returned above
void freePath(pXArray path){
    int i;
    for(i=0;i<xaCount(path);i++)
        nmSysFree(xaGetItem(path,i));
    return;
}//end freePath

// Initialization and deinitialization functions
pXTreeNode xt_internal_CreateNode(){
    pXTreeNode toReturn;
    toReturn = nmMalloc(sizeof(XTreeNode));
    if(toReturn){//initialze internals
        toReturn->subObjs = nmMalloc(sizeof(XHashTable));
        xhInit(toReturn->subObjs,8,0);
        toReturn->items = 0;
        toReturn->key = NULL;
        toReturn->data = FLG_NODE;
    }
    return toReturn;
}//end xt_internal_CreateNode

int xtInit(pXTree tree, char separator){
    if(tree){
        tree->separator = separator;
        tree->rootNode = xt_internal_CreateNode();
        tree->rootNode->data = FLG_ROOT;
    }
    return 0;
}//end xtInit

void xt_internal_FreeNode(pXTreeNode node){
    if(!node)return;
    xhDeInit(node->subObjs);
    nmFree(node->subObjs,sizeof(XHashTable));
    nmSysFree(node->key);
    nmFree(node,sizeof(XTreeNode));
    return;
}//end xt_internal_FreeNode

int xtDeInit(pXTree tree){
    xtClear(tree, NULL, NULL);
    nmSysFree(tree->rootNode);
    return 0;
}

//recursivly places an item into the tree
int xt_internal_add(pXTreeNode node, pXArray path, int depth, char *data){
    if(depth<0 || depth>xaCount(path))
        return -3;
    if(depth == xaCount(path)){
        node->data=data;
        node->key=nmSysStrdup(xaGetItem(path,depth-1));
        return 0;
    }//end base case
    if(thExcessiveRecursion()){
        mssError(0,"xtree","Could not add item, out of stack space!");
        return -2;
    }//end check for stack space
    if(!xhLookup(node->subObjs,xaGetItem(path,depth))){
        pXTreeNode tmp = xt_internal_CreateNode();
        xhAdd(node->subObjs,(char *)xaGetItem(path,depth),(char *)tmp);
        node->items++;
        tmp->key = nmSysStrdup(xaGetItem(path,depth));
    }//end if node not exist
    return xt_internal_add((pXTreeNode)xhLookup(node->subObjs,xaGetItem(path,depth)),
            path,depth+1,data);
}//end xt_internal_add

int xtAdd(pXTree this, char* key, char* data){
    int ret;
    pXArray path = keytopath(key,this->separator);
    ret = xt_internal_add(this->rootNode,path,0,data);
    freePath(path);
    return ret;
}//end xtAdd

int xt_internal_del(pXTreeNode node, pXArray path, int depth){
    if(depth<0 || depth>xaCount(path))
        return -3;
    if(depth == xaCount(path)){
        //should not be removing root!
        if(node->data == FLG_ROOT)
            return -5;
        //re-delete? that like double free!
        if(node->data == FLG_NODE)
            return -5;
        node->data = FLG_NODE;
        return 0;
    }//end base case
    if(thExcessiveRecursion()){
        mssError(0,"xtree","Ironically, we could not delete a item, because we are out of stack space.");
        return -2;
    }//end check for stack space
    if(!xhLookup(node->subObjs,xaGetItem(path,depth)))
        return -1;
    return xt_internal_del((pXTreeNode)xhLookup(node->subObjs,xaGetItem(path,depth)),
            path,depth+1);
}//end xt_internal_delete

int xtRemove(pXTree this, char* key){
    int toReturn;
    pXArray path = keytopath(key,this->separator);
    toReturn = xt_internal_del(this->rootNode,path,0);
    if(!this->rootNode){
        toReturn &= xtInit(this, this->separator);
    }
    freePath(path);
    return toReturn;
}

pXTreeNode xt_internal_lookup(pXTreeNode node, pXArray path, int depth){
    if(depth<0 || depth>xaCount(path))
        return NULL;
    if(depth == xaCount(path))
        return node;
    if(thExcessiveRecursion()){
        mssError(0,"xtree","Search dropped, out of stack space.");
        return NULL;
    }//end check for stack space
    if(!xhLookup(node->subObjs,xaGetItem(path,depth)))
        return node;
    return xt_internal_lookup((pXTreeNode)xhLookup(node->subObjs,xaGetItem(path,depth)),
            path,depth+1);
}//end xt_internal_lookup

char *xtLookup(pXTree this, char* key){
    char *data;
    pXTreeNode node;
    pXArray path = keytopath(key,this->separator);
    node = xt_internal_lookup(this->rootNode,path,0);
    //only return sane data
    if(node->data==FLG_ROOT || node->data==FLG_NODE || node->data<=0)
        data = NULL;
    else data=node->data;
    if(strcmp(node->key,xaGetItem(path,xaCount(path)-1)))
        data = NULL;
    freePath(path);
    return data;
}//end xtLookup

int xtClear(pXTree this, int (*free_fn)(char *data, void *free_arg), void* free_arg){
    return -5;
}

char* xtLookupBeginning(pXTree this, char* key){
    char *data;
    pXTreeNode node;
    pXArray path = keytopath(key,this->separator);
    node = xt_internal_lookup(this->rootNode,path,0);
    //only return sane data
    if(node->data==FLG_ROOT || node->data==FLG_NODE || node->data<=0)
        data = NULL;
    else data=node->data;
    freePath(path);
    return data;
}//end xtLookupBeginning

void xtTraverse(pXTree tree, void (*iterFunc)(char *key, char *data, void *userData), void *userData){
    return;
}
