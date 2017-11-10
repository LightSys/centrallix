#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h"
/** module definintions **/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#ifdef USE_LIBXML1
#include <parser.h>
#else
#include <libxml/parser.h>
#endif
/** regular expressions **/
#include <sys/types.h>
#include <regex.h>

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	XML Objectsystem driver					*/
/* Author:	Jonathan Rupp						*/
/* Creation:	July 28, 2002						*/
/* Description:	This in an XML osdriver, using libxml2			*/
/*									*/
/************************************************************************/


#if 0
/**
    Documentation

    Here's where I claim everything is 'a feature, not a bug', and such.

    Implimented:
	Open
	Query
	Read

    Unimplimented:
	Write
	Set
	Create
	Delete
	Caching of opens

    How it all should work:
    Basically, the idea I worked off of was trying to get it to present the 
    XML tree to centrallix in the way that I thought would make most sense
    to someone looking at the document.  So, with that in mind, here's how
    it works:

    1. It does not work off a structure file telling it the location of the
	document.  It uses objRead to get it from the underlying object
    2. If there are multiple nodes of the same name at a level, they are
	accessible as name|0, name|1, etc. (a diagram follows)
    3. XML Attributes are mapped to Centrallix attributes
    4. XML subnodes are mapped to Centrallix subobjects (except if the XML
        subnode's name is unique and it has no attributes or element subnodes
	-- in this case, it is available as an object referenced directly, but
	not as the result of a QueryFetch().)
    5. If a node has subnodes (such that there is only one subnode with each
	name), those subnodes are also mapped to Centrallix attributes
    
    Example:
    Simplified XML doc:
    <?xml>
    <roottag>
	<tagA prop="bob" propF="billy">
	    <tagB>Hello there</tagB>
	    <tagC propC="bob">Harry</tagC>
	</tagA>
	<tagA prop="house" propF="thekid">
	</tagA>
	<tagB>Harry</tagB>
	<tagC propQ="Quincy">Tom</tagC>
    </roottag>

    Centrallix view of above (file is /file.xml):
	(/'s are the path separator, :'s are the property separator)
    /file.xml/tagA (invalid reference)
    /file.xml/tagA|0=Object
    /file.xml/tagA|0:prop="bob"
    /file.xml/tagA|0:propF="billy"
    /file.xml/tagA|0:tagB="Hello there"
    /file.xml/tagA|0:tagC="Harry"
    /file.xml/tagA|0/tagB=Object (content="Hello there")
    /file.xml/tagA|0/tagB|0=Object (content="Hello there") <-- not returned by a query
    /file.xml/tagA|0/tagC=Object (content="Harry")
    /file.xml/tagA|0/tagC:propC="bob"
    /file.xml/tagA|0/tagC|0=Object (content="Harry") <-- not returned by a query
    /file.xml/tagA|0/tagC|0:propC="bob"
    /file.xml/tagA|1:prop="house"
    /file.xml/tagA|1:propF="thekid"
    /file.xml/tagB=Object (content="Harry") <-- not returned by a query
    /file.xml/tagB|0=Object (content="Harry") <-- not returned by a query
    /file.xml/tagC=Object (content="Tom") <-- _would_ be returned by a query
    /file.xml/tagC|0=Object (content="Tom") <-- not returned by a query
    /file.xml/tagC:propQ="Quincy"

    If you ran the query:
	select :name,:prop,:propF from /file.xml/tagA
    you should get:
	[name]	[prop]	[propF]
	0	bob	billy
	1	house	thekid

    I think that covers it.
**/
#endif

#define XML_BLOCK_SIZE 8092
#define XML_ELEMENT_SIZE 64
#define XML_ATTR_SIZE 256

#define XML_DEBUG 0

#define XML_ATTR 1
#define XML_SUBOBJ 2

/** the element used in the document cache **/
typedef struct
    {
    xmlDocPtr	document;
    DateTime	lastmod;
    int		LinkCnt;
    }
    XmlCacheObj, *pXmlCacheObj;

/** the element of the inf->Attributes hash **/
typedef struct
    {
    int type;
    int count;
    void* ptr;
    }
    XmlAttrObj, *pXmlAttrObj;

/** the element of the query->Types hash **/
typedef struct
    {
    int current;
    int total;
    }
    XmlCountObj, *pXmlCountObj;

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    xmlNodePtr	CurNode;
    //char	Element[XML_ELEMENT_SIZE];
    int		Offset;
    int		CurAttr;
    xmlAttrPtr	CurXmlAttr;
    xmlNodePtr	CurSubNode;
    /** GetAttrValue has to return a refence to memory that won't be free()ed **/
    char*	AttrValue;
    pXHashTable	Attributes;
    pXmlCacheObj    CacheObj;
    }
    XmlData, *pXmlData;


#define XML(x) ((pXmlData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pXmlData	Data;
    //char	NameBuf[256];
    int		ItemCnt;
    xmlNodePtr	NextNode;
    pXHashTable	Types;
    }
    XmlQuery, *pXmlQuery;

/*** GLOBALS ***/
struct
    {
    XHashTable	cache;
    }
    XML_INF;

/** Forward declaration **/
void xml_internal_BuildAttributeHashTable(pXmlData inf);

/*** xml_internal_GetChildren - obtains a reference to the children/childs
 *** object in an XML tree structure.  libxml 1.x called it "childs" [sic]
 *** while libxml 2.x calls it "children".
 ***/
#if 0
#ifdef USE_LIBXML1
#define xml_internal_GetChildren(p) p->childs
#else
#define xml_internal_GetChildren(p) p->children
#endif
#else
    xmlNodePtr
xml_internal_GetChildren(xmlNodePtr parent)
    {
#ifdef USE_LIBXML1
    return parent->childs;
#else
    return parent->children;
#endif
    }
#endif

/*** This is dumb, but is needed for type safety (second argument to xhClear)
 ***/
int
free_wrapper(void* ptr, void* arg)
    {
        arg = NULL ; /* avoid complaints from picky compilers */
        free(ptr)  ;
        return 0   ; /* meaningless, but is needed for type safety */
    }


/*** xml_internal_IsObject
 ***   decides if an XML node will be a centrallix object
 ***   criteria (must meet one):
 ***     has at least 1 attribute
 **      has at least 1 child whose type = XML_ELEMENT_NODE
 *** returns 0 if not an object, -1 if it is
 ***
 *** NOTE: this only applies to objects that have unique names
 ***/
int
xml_internal_IsObject(xmlNodePtr p)
{
    if(p->properties) return -1;
    p=xml_internal_GetChildren(p);
    if(!p) return 0;
    while(p)
    {
	if(p->type==XML_ELEMENT_NODE) return -1;
	p=p->next;
    }
    return 0;
}

/*** xml_internal_GetNode - given an inf->CurNode as root, 
 ***   heads down the path in obj to find the node refered to
 *** returns 0 on success, -1 on failure -- fails only on regex failure
 ***/
int
xml_internal_GetNode(pXmlData inf,pObject obj)
    {
    xmlNodePtr p;

    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"XML","Could not lookup XML node: resource exhaustion occurred");
	return -1;
	}

    /** don't want to go wandering off farther than we need **/
    if(obj->Pathname->nElements<obj->SubPtr+obj->SubCnt)
	return 0;
    
    p= xml_internal_GetChildren(inf->CurNode);
    if(p)
	{
	int i=0; /* Number of nodes that match */
	char *ptr; /* passed to strtoi for so we can tell if a number was grabbed */
	int target=-1; /* the element number we're looking for */
	int flag=0; /* did we find the target? */
	char searchElement[XML_ELEMENT_SIZE];
	regex_t namematch;
	regmatch_t pmatch[3];

	ptr=obj_internal_PathPart(obj->Pathname,obj->SubPtr+obj->SubCnt-1,1);
	if((i=regcomp(&namematch,"^([^|]*)\\|*([0123456789]*)$",REG_EXTENDED)))
	    {
	    /** this is a critical failure -- this regex should compile just fine **/
	    mssError(0,"XML","Error while building namematch");
	    return -1;
	    }
	if(regexec(&namematch,ptr,3,pmatch,0)!=0)
	    {
	    /** be optimistic: maybe this is for another driver **/
	    mssError(0,"XML","Warning: pattern didn't match!");
	    return 0;
	    }
	if(pmatch[1].rm_so==-1 || pmatch[1].rm_so==pmatch[1].rm_eo)
	    {
	    /** be optimistic: maybe this is for another driver **/
	    mssError(0,"XML","Warning: there was no base text matched!");
	    return 0;
	    }
	i=pmatch[1].rm_eo-pmatch[1].rm_so; /* length of the string we're going to copy */
	i=i>XML_ELEMENT_SIZE-1?XML_ELEMENT_SIZE-1:i;
	memset(searchElement,0,i+1);
	memcpy(searchElement,ptr+pmatch[1].rm_so,i);

	if(pmatch[2].rm_so!=-1 && pmatch[2].rm_so!=pmatch[2].rm_eo)
	    {
	    if(XML_DEBUG) printf("there appears to be a numeric offset -- reading it\n");
	    /** this will find a number there -- the regexec matched **/
	    target=atoi(ptr+pmatch[2].rm_so);
	    }
	
	if(XML_DEBUG) printf("looking for %s -- %i\n",searchElement,target);
	i=0; /** 'i' was a temp variable above -- reset to 0 **/
	do
	    {
	    if(!strcmp((char*)p->name,searchElement))
		{
		if(i==target)
		    {
		    inf->CurNode=p;
		    flag=1;
		    break;
		    }
		i++;
		/** if we had no target and we've seen two matches -- early abort **/
		if(target==-1 && i==2) 
		    {
		    if(XML_DEBUG) printf("performing early abort with %i,%i\n",target,i);
		    break;
		    }
		}
	    } while  ((p=p->next)!=NULL);
	if( (flag==0) && (i==0 || target>=i) )
	    {
	    /** we couldn't find the node in quetion.
	     **   Maybe there's some content here for another osdriver
	     **/
	    if(XML_DEBUG) printf("There were children, but couldn't find any nodes, or a high enough one (%i < %i)\n",i-1,target);
	    return 0;
	    }
	else
	    {
	    if(flag==1)
		{
		/** we found some nodes that match, and the specific requested one **/
		if(XML_DEBUG) printf("found matches, and exact target: %i\n",target);
		obj->SubCnt++;
		inf->CurNode=p;
		/** maybe there are more **/
		return xml_internal_GetNode(inf,obj);
		}
	    else
		{
		/** we found some nodes that match **/
		if(i==1 && target==-1)
		    {
		    /** there was only one match, and there wasn't a number at the next level
		     **  -- return the one match **/
		    p=xml_internal_GetChildren(inf->CurNode);
		    do
			{
			if(!strcmp((char*)p->name,searchElement))
			    {
			    inf->CurNode=p;
			    flag=1;
			    break;
			    }
			} while  ((p=p->next)!=NULL);
		    
		    if(XML_DEBUG) printf("found matches, and assumed target\n");
		    obj->SubCnt++;
		    inf->CurNode=p;
		    /** maybe there are more **/
		    return xml_internal_GetNode(inf,obj);
		    }
		else
		    {
		    /** multiple matches w/ no target -- this is an _invalid_ node reference 
		     **   assume the best -- that it's going to be matched by another osdriver
		     **/
		    if(XML_DEBUG) printf("There were too many choices\n");
		    return 0;
		    }
		}
	    }
	}
    else
	{
	/** we couldn't find the node in quetion.
	 **   Maybe there's some content here for another osdriver
	 **/
	if(XML_DEBUG) printf("no children found...\n");
	return 0;
	}
    /** should _never_ get here **/
    return -1;
    }


int
xml_internal_CloseCachedDocument(pXmlCacheObj document)
    {

	document->LinkCnt--;
	if (document->LinkCnt > 0)
	    return 0;

	xmlFreeDoc(document->document);
	nmFree(document, sizeof(XmlCacheObj));

    return 0;
    }


pXmlCacheObj
xml_internal_ReadDoc(pObject obj)
    {
    xmlParserCtxtPtr ctxt;
    char* ptr;
    char* path;
    int bytes;
    pXmlCacheObj pCache;
    pDateTime pDT=0;

	/** Determine path of just the XML file itself **/
	path=obj_internal_PathPart(obj->Pathname,0,obj->SubPtr);

	/** Check cache for an existing copy already **/
	if((pCache=(pXmlCacheObj)xhLookup(&XML_INF.cache, path)))
	    {
	    if(XML_DEBUG) printf("found %s in cache\n", path);

	    /** found match in cache -- check modification time **/
	    if(objGetAttrValue(obj->Prev, "last_modification", DATA_T_DATETIME, POD(&pDT))==0)
		{
		if(pDT && pDT->Value!=pCache->lastmod.Value)
		    {
		    /** modification time changed -- update **/
		    xhRemove(&XML_INF.cache, path);
		    xml_internal_CloseCachedDocument(pCache);
		    pCache = NULL;
		    }
		}
	    }

	/** Not in cache, or cache was stale **/
	if (!pCache)	
	    {
	    if(XML_DEBUG) printf("couldn't find %s in cache\n",path);
	    pCache=(pXmlCacheObj)nmMalloc(sizeof(XmlCacheObj));
	    if(!pCache) return NULL;
	    memset(pCache,0,sizeof(XmlCacheObj));
	    ptr=(char*)malloc(strlen(path)+1);
	    strcpy(ptr,path);
	    xhAdd(&XML_INF.cache,ptr,(char*)pCache);
	    pCache->LinkCnt = 1;
	    }

	/** Need to load the content itself **/
	if(!pCache->document)
	    {
#ifndef USE_LIBXML1
	    xmlKeepBlanksDefault (0);
	    xmlLineNumbersDefault(1);
#endif
	    /** parse the document **/
	    ptr=(char*)malloc(XML_BLOCK_SIZE);
	    ctxt=xmlCreatePushParserCtxt(NULL,NULL,NULL,0,"unknown");
	    objRead(obj->Prev,ptr,0,0,FD_U_SEEK);
	    while((bytes=objRead(obj->Prev,ptr,XML_BLOCK_SIZE,0,0))>0)
	    {
		if(XML_DEBUG) printf("giving parser a chunk\n");
		xmlParseChunk(ctxt,ptr,bytes,0);
		if(XML_DEBUG) printf("parser done with the chunk\n");
	    }
	    free(ptr);
	    xmlParseChunk(ctxt,NULL,0,1);
	    /** get the document reference **/
	    if(!ctxt->myDoc)
		{
		mssError(0,"XML","No Document in XML file!");
		xmlFreeParserCtxt(ctxt);
		return NULL;
		}
	    pCache->document=ctxt->myDoc;
	    xmlFreeParserCtxt(ctxt);
	}

    return pCache;
    }

/*** xmlOpen - open an object.
 ***/
void*
xmlOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pXmlData inf;
    int rval;
    char* node_path;
    pSnNode node = NULL;
    pXmlCacheObj pCache;

	/** Allocate the structure **/
	inf = (pXmlData)nmMalloc(sizeof(XmlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(XmlData));
	inf->Obj = obj;
	inf->Mask = mask;


	/** Set object params. **/
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));

	if (!(inf->CacheObj=xml_internal_ReadDoc(obj)))
	    {
	    nmFree(inf,sizeof(XmlData));
	    return NULL;
	    }

	inf->CurNode=xmlDocGetRootElement(inf->CacheObj->document);

	obj->SubCnt=1;
	if(XML_DEBUG) printf("objdrv_xml.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	
	if(xml_internal_GetNode(inf,obj) < 0)
	    {
	    nmFree(inf,sizeof(XmlData));
	    mssError(0,"XML","Unable to find correct XML node!");
	    return NULL;
	    }
	
	if(XML_DEBUG) printf("objdrv_xml.c took: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	inf->CacheObj->LinkCnt++;

    return (void*)inf;
    }


/*** xmlClose - close an open object.
 ***/
int
xmlClose(void* inf_v, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);

	if(inf->Attributes)
	    {
	    /** this structure might not have been allocated **/
	    if(XML_DEBUG) printf("Clearing Attributes hash\n");
	    xhClear(inf->Attributes,free_wrapper,NULL);
	    if(XML_DEBUG) printf("Done Clearing Attributes hash\n");
	    xhDeInit(inf->Attributes);
	    nmFree(inf->Attributes,sizeof(XHashTable));
	    }
	/** Write the node first, if need be. **/
	//snWriteNode(inf->Node);
	
	//xmlFreeDoc(inf->CurrNode);
	if(XML_DEBUG) printf("objdrv_xml.c closing: (%i,%i,%i) %s\n",inf->Obj->SubPtr,
		inf->Obj->SubCnt,inf->Obj->Pathname->nElements,obj_internal_PathPart(inf->Obj->Pathname,0,0));
	
	/** free any memory used to return an attribute **/
	if(inf->AttrValue)
	    {
	    free(inf->AttrValue);
	    inf->AttrValue=NULL;
	    }

	/** Release the memory **/
	xml_internal_CloseCachedDocument(inf->CacheObj);
	nmFree(inf,sizeof(XmlData));

    return 0;
    }


/*** xmlCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
xmlCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    int fd;
    void* inf;

    	/** Call open() then close() **/
	obj->Mode = O_CREAT | O_EXCL;
	inf = xmlOpen(obj, mask, systype, usrtype, oxt);
	if (!inf) return -1;
	xmlClose(inf, oxt);

    return 0;
    }


/*** xmlDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
xmlDelete(pObject obj, pObjTrxTree* oxt)
    {
    struct stat fileinfo;
    pXmlData inf, find_inf, search_inf;
    int is_empty = 1;
    int i;

    /** Unimplimented **/
    return -1;
#if 0

    	/** Open the thing first to get the inf ptrs **/
	obj->Mode = O_WRONLY;
	inf = (pXmlData)xmlOpen(obj, 0, NULL, "", oxt);
	if (!inf) return -1;

	/** Check to see if user is deleting the 'node object'. **/
	if (obj->Pathname->nElements == obj->SubPtr)
	    {
	    if (inf->Node->OpenCnt > 1) 
	        {
		xmlClose(inf, oxt);
		mssError(1,"XML","Cannot delete structure file: object in use");
		return -1;
		}
	    /** Need to do some checking to see if, for example, a non-empty object can't be deleted **/
	    /** YOU WILL NEED TO REPLACE THIS CODE WITH YOUR OWN. **/
	    is_empty = 0;
	    if (!is_empty)
	        {
		xmlClose(inf, oxt);
		mssError(1,"XML","Cannot delete: object not empty");
		return -1;
		}
	    stFreeInf(inf->Node->Data);

	    /** Physically delete the node, and then remove it from the node cache **/
	    unlink(inf->Node->NodePath);
	    snDelete(inf->Node);
	    }
	else
	    {
	    /** Delete of sub-object processing goes here **/
	    }

	/** Release, don't call close because that might write data to a deleted object **/
	nmFree(inf,sizeof(XmlData));
#endif
    return 0;
    }


/*** xmlRead - Read from the XML element
 ***/
int
xmlRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    unsigned char *buf;
    int i;

    /** get the data from the tree **/
    buf=xmlNodeListGetString(inf->CurNode->doc,xml_internal_GetChildren(inf->CurNode),1);

    if(!buf)
	return -1;
    
    /** 'seek' if we're supposed to **/
    if(flags & FD_U_SEEK)
	inf->Offset=offset;

    /** calcuate length to use -- the smaller of the remaining buffer after offset or the maxlen **/
    if(maxcnt>strlen((char*)buf)-inf->Offset)
	i=strlen((char*)buf)-inf->Offset;
    else
	i=maxcnt;

    if(i<0) return -1;
    
    memcpy(buffer,buf+inf->Offset,i);

    inf->Offset+=i;

    free(buf);

    return i;
    }


/*** xmlWrite - Write to the XML element
 ***/
int
xmlWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    /** Unimplimented **/
    return -1;
    }


/*** xmlOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
xmlOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    pXmlQuery qy;
    xmlNodePtr p;
    pXmlCountObj pHE;

	/** Allocate the query structure **/
	qy = (pXmlQuery)nmMalloc(sizeof(XmlQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(XmlQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	qy->NextNode = NULL;

	p=xml_internal_GetChildren(inf->CurNode);
	qy->Types=(pXHashTable)nmMalloc(sizeof(XHashTable));
	memset(qy->Types, 0, sizeof(XHashTable));
	xhInit(qy->Types,17,0);

	if(p)
	    {
	    do
		{
		if(p->name && p->name[0])
		    {
		    if(!(pHE=(pXmlCountObj)xhLookup(qy->Types,(char*)p->name)))
			{
			pHE=malloc(sizeof(XmlCountObj));
			if(!pHE) return NULL;
			pHE->current=-1; /* this will be incremented for the first record */
			pHE->total=0;
			xhAdd(qy->Types,(char*)p->name,(char*)pHE);
			}
		    pHE->total++;
		    }
		} while((p=p->next));
	    }

    return (void*)qy;
    }


/*** xmlQueryFetch - get the next directory entry as an open object.
 ***/
void*
xmlQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    pXmlQuery qy = ((pXmlQuery)(qy_v));
    pXmlData inf;
    int cnt;
    int flag=0;
    char name[256];
    char *ptr;
    pXmlCountObj pHE;

	/** I _think_ these should be this way... **/
	obj->SubPtr=qy->Data->Obj->SubPtr;
	obj->SubCnt=qy->Data->Obj->SubCnt;
	
	if(XML_DEBUG) printf("QueryFetch was given: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	/** Alloc the structure **/
	inf = (pXmlData)nmMalloc(sizeof(XmlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(XmlData));
	inf->CacheObj=qy->Data->CacheObj;

	while(flag==0)
	    {
	    if(!qy->NextNode)
		qy->NextNode=xml_internal_GetChildren(qy->Data->CurNode);
	    else
		qy->NextNode=qy->NextNode->next;
	    if(!qy->NextNode) 
		{
		nmFree(inf,sizeof(XmlData));
		return NULL;
		}

	    /** figure out which number the one we're returning now is **/
	    if((pHE=(pXmlCountObj)xhLookup(qy->Types,(char*)qy->NextNode->name)))
		{
		pHE->current++;
		}
	    else
		{
		mssError(0,"XML","Node not found in qy->Types");
		nmFree(inf,sizeof(XmlData));
		return NULL;
		}
	    if(pHE->total==1 && pHE->current==0) 
		{
		if(xml_internal_IsObject(qy->NextNode))
		    {
		    flag=1;
		    cnt = snprintf(name,256,"%s",qy->NextNode->name);
		    }
		}
	    else
		{
		flag=1;
		/** the structure file driver will return the node's literal name, even if it isn't unique
		 **   -- for now, we'll follow suit -- should we not do this?
		 **/
		cnt = snprintf(name,256,"%s|%i",qy->NextNode->name,pHE->current);
		//cnt = snprintf(name,256,"%s",qy->NextNode->name);
		}

	    }


	/** make sure we didn't overflow on the path copy **/
	if (cnt<0 || cnt>=256 ) 
	    {
	    mssError(1,"XML","Query result pathname exceeds internal representation");
	    nmFree(inf,sizeof(XmlData));
	    return NULL;
	    }

	/** Shamelessly stolen from objdrv_sybase.c :) **/
	if (obj_internal_AddToPath(obj->Pathname, name) < 0)
	    {
	    mssError(1,"XML","Query result pathname exceeds internal limits");
	    nmFree(inf,sizeof(XmlData));
	    return NULL;
	    }
	obj->SubCnt++;

	if(XML_DEBUG) printf("QueryFetch gave back: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	inf->Obj = obj;
	inf->CurNode=qy->NextNode;
	qy->ItemCnt++;

	inf->CacheObj->LinkCnt++;

    return (void*)inf;
    }


/*** xmlQueryClose - close the query.
 ***/
int
xmlQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pXmlQuery qy = ((pXmlQuery)(qy_v));

	/** this structure should have been allocated **/
	if(qy->Types)
	    {
	    if(XML_DEBUG) printf("Clearing Types hash\n");
	    xhClear(qy->Types,free_wrapper,NULL);
	    if(XML_DEBUG) printf("Done Clearing Types hash\n");
	    xhDeInit(qy->Types);
	    nmFree(qy->Types,sizeof(XHashTable));
	    }
	/** Free the structure **/
	nmFree(qy,sizeof(XmlQuery));

    return 0;
    }


/*** xmlGetAttrType - get the type (DATA_T_xml) of an attribute by name.
 ***/
int
xmlGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    int i;
    pStructInf find_inf;
    char* ptr;
    char *ptr2;
    pXmlAttrObj pHE;
    xmlAttrPtr ap;
    xmlNodePtr np;

    	/** If name or type, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;
	if (!strcmp(attrname,"internal_type")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"last_modification")) return DATA_T_DATETIME;

	/** literal escape? **/
	if (!strncmp(attrname, "__cx_literal_", 13))
	    attrname += 13;

	/** needed in case this isn't a GetFirstAttribute-style request **/
	xml_internal_BuildAttributeHashTable(inf);

	/** see if the XML doc has it and only has one possibility **/
	if((pHE=(pXmlAttrObj)xhLookup(inf->Attributes,attrname)) && pHE->count==1)
	    {
	    /** it does **/
	    if(pHE->type==XML_ATTR)
		{
		ap=(xmlAttrPtr)pHE->ptr;
		/*ptr2=(char*)xml_internal_GetChildren(ap);*/
		/* I consider this a hack -- I can't figure out where to get the text! */
		inf->AttrValue=(char*)xmlGetProp(ap->parent,ap->name);
		}
	    else if(pHE->type==XML_SUBOBJ)
		{
		np=(xmlNodePtr)pHE->ptr;
		inf->AttrValue=(char*)xmlNodeListGetString(np->doc,xml_internal_GetChildren(np),1);
		}
	    else
		{
		/** This should never happen, but... **/
		mssError(0,"XML","Critical error! pHE->type is not valid! --  %s:%i",__FILE__,__LINE__);
		return -1;
		}

	    if(inf->AttrValue)
		{
		(void)strtoi(inf->AttrValue,&ptr,10);
		if(ptr && !*ptr)
		    {
		    free(inf->AttrValue);
		    inf->AttrValue=NULL;
		    return DATA_T_INTEGER;
		    }
		free(inf->AttrValue);
		inf->AttrValue=NULL;
		return DATA_T_STRING;
		}
	    else
		{
		/** if there's no text, we're going to return the empty string **/
		return DATA_T_STRING;
		}
	    }

	/** everything else is a string **/
	return DATA_T_STRING;

    return -1;
    }

void
xml_internal_BuildAttributeHashTable(pXmlData inf)
    {
    xmlNodePtr np;
    xmlAttrPtr ap;
    pXmlAttrObj pHE;
    unsigned char* p;

    if(!inf->Attributes)
	{
	inf->Attributes=(pXHashTable)nmMalloc(sizeof(XHashTable));
	memset(inf->Attributes, 0, sizeof(XHashTable));
	xhInit(inf->Attributes,17,0);
	/** if we've built this once already, there's no need to build it all over again... **/
	/**   in GetNextAttr, we'll only use records from here where the count was 1 **/
	ap=inf->CurNode->properties;
	if(XML_DEBUG) printf("walking attributes to set up hash table\n");
	while(ap)
	    {
	    pHE=(pXmlAttrObj)nmMalloc(sizeof(XmlAttrObj));
	    if(!pHE)
		{
		mssError(0,"XML","nmMalloc failed!");
		return;
		}
	    pHE->type=XML_ATTR;
	    pHE->count=1;
	    pHE->ptr=ap;
	    xhAdd(inf->Attributes,(char*)ap->name,(char*)pHE);
	    ap=ap->next;
	    }

	np=xml_internal_GetChildren(inf->CurNode);
	if(XML_DEBUG) printf("walking children to set up hash table\n");
	while(np)
	    {
	    /** see if there's text there **/
	    p=xmlNodeListGetString(np->doc,xml_internal_GetChildren(np),1);
	    /** add if it has a valid name and either 
	     **  1. text (p is set)
	     **   or
	     **  2. no children, no text, and no attributes (np->children is null)
	     **/
	    if(np->name && (p || !np->children) )
		{
		if((pHE=(pXmlAttrObj)xhLookup(inf->Attributes,(char*)np->name)))
		    {
		    if(pHE->type==XML_SUBOBJ)
			pHE->count++;
		    }
		else
		    {
		    pHE=(pXmlAttrObj)nmMalloc(sizeof(XmlAttrObj));
		    pHE->type=XML_SUBOBJ;
		    pHE->count=1;
		    pHE->ptr=np;
		    xhAdd(inf->Attributes,(char*)np->name,(char*)pHE);
		    }
		}
	    if(p) free(p);
	    np=np->next;
	    }
	}
    }

/*** xmlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
xmlGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    pStructInf find_inf;
    char* ptr;
    char* ptr2;
    int i;
    XmlAttrObj* pHE;
    xmlNodePtr np;
    xmlAttrPtr ap;

    if(!inf)
	{
	mssError(0,"XML","Null pointer passed as object to xmlGetAttrValue for %s!",attrname);
	return -1;
	}

	if(inf->AttrValue)
	    {
	    free(inf->AttrValue);
	    inf->AttrValue=NULL;
	    }

	/** inner_type is an alias for content_type **/
	if(!strcmp(attrname,"inner_type"))
	    return xmlGetAttrValue(inf_v,"content_type",datatype,val,oxt);
    
	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"XML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** for the top level one -- return the inner name, not the outer one **/
	    if(inf->Obj->SubCnt==1)
		val->String = (char*)inf->CurNode->name;
	    else
		val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	/** internal_type is the XML node name **/
	if (!strcmp(attrname,"internal_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"XML","Type mismatch getting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    val->String = (char*)inf->CurNode->name;
	    return 0;
	    }

	/** If content-type, and it wasn't specified in the XML **/
	if (!strcmp(attrname,"content_type"))
	    {
	    val->String = "text/plain";
	    return 0;
	    }

	/** If outer type, and it wasn't specified in the XML **/
	if (!strcmp(attrname,"outer_type"))
	    {
	    val->String = "text/xml-node";
	    return 0;
	    }

	/** take last_modification from underlying object if it has one **/
	if(!strcmp(attrname,"last_modification"))
	    if(objGetAttrValue(inf->Obj->Prev,"last_modification",DATA_T_DATETIME,val)==0)
		return 0;

	/** literal escape? **/
	if (!strncmp(attrname, "__cx_literal_", 13))
	    attrname += 13;

	/** needed in case this isn't a GetFirstAttribute-style request **/
	xml_internal_BuildAttributeHashTable(inf);

	/** see if the XML doc has it and only has one possibility **/
	if((pHE=(pXmlAttrObj)xhLookup(inf->Attributes,attrname)) && pHE->count==1)
	    {
	    /** it does **/
	    if(pHE->type==XML_ATTR)
		{
		ap=(xmlAttrPtr)pHE->ptr;
		/*ptr2=(char*)xml_internal_GetChildren(ap);*/
		/* I consider this a hack -- I can't figure out where to get the text! */
		inf->AttrValue=(char*)xmlGetProp(ap->parent,ap->name);
		}
	    else if(pHE->type==XML_SUBOBJ)
		{
		np=(xmlNodePtr)pHE->ptr;
		inf->AttrValue=(char*)xmlNodeListGetString(np->doc,xml_internal_GetChildren(np),1);
		}
	    else
		{
		/** This should never happen, but... **/
		mssError(0,"XML","Critical error! pHE->type is not valid! --  %s:%i",__FILE__,__LINE__);
		return -1;
		}

	    if(inf->AttrValue)
		{
		if(datatype==DATA_T_INTEGER)
		    {
		    val->Integer=strtoi(inf->AttrValue,&ptr,10);
		    if(ptr && !*ptr)
			{
			free(inf->AttrValue);
			inf->AttrValue=NULL;
			return 0;
			}
		    free(inf->AttrValue);
		    inf->AttrValue=NULL;
		    mssError(0,"XML","%s is not an integer",attrname);
		    return -1;
		    }
		else if (datatype==DATA_T_STRING)
		    {
		    val->String = inf->AttrValue;
		    return 0;
		    }
		else
		    {
		    mssError(0,"XML","%i is not a valid type for %s",datatype,attrname);
		    return -1;
		    }
		}
	    else
		{
		/** if there's no text, we're going to return the empty string **/
		inf->AttrValue=(char*)malloc(1);
		inf->AttrValue[0]='\0';
		val->String = inf->AttrValue;
		return -0;
		}
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    val->String = "";
	    return 0;
	    }

	/*if(XML_DEBUG) mssError(1,"XML","Could not locate requested attribute: %s",attrname);*/

    return 1; /* null if not there presently */
    }


/*** xmlGetNextAttr - get the next attribute name for this object.
 ***/
char*
xmlGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    char *p;
    
    /** don't return this from the iterator --
     **   it causes problems for the connector **/
#if 0
    switch(inf->CurAttr++)
	{
	case 0: return "internal_type";
	}
#endif

    /** just for safety's sake -- really shouldn't be needed **/
    xml_internal_BuildAttributeHashTable(inf);
    
    /** all XML attributes are Centrallix attributes **/
    if(inf->CurXmlAttr)
	{
	p=(char *)inf->CurXmlAttr->name;
	inf->CurXmlAttr=inf->CurXmlAttr->next;
	return p;
	}
    
    /** only subobjects of a unique type are attributes **/
    if(inf->CurSubNode && inf->Attributes)
	{
	pXmlAttrObj pHE;
	while(inf->CurSubNode)
	    {
	    if(inf->CurSubNode->name && inf->CurSubNode->name[0] && /** verify a valid name **/
		/** _never_ return one of the 'special' attributes, even it it was overriden in the document **/
		/** without this, connectors act wierd ... **/
		strcmp((char*)inf->CurSubNode->name,"outer_type") &&
		strcmp((char*)inf->CurSubNode->name,"inner_type") &&
		strcmp((char*)inf->CurSubNode->name,"content_type") &&
		strcmp((char*)inf->CurSubNode->name,"last_modification") &&
		strcmp((char*)inf->CurSubNode->name,"annotation") &&
		strcmp((char*)inf->CurSubNode->name,"name") &&
		(pHE=(pXmlAttrObj)xhLookup(inf->Attributes,(char*)inf->CurSubNode->name)) && /** get AttrObj **/
		pHE->count==1) /** only one subobject of this type **/
		{
		p=(char *)inf->CurSubNode->name;
		inf->CurSubNode=inf->CurSubNode->next;
		return p;
		}
	    else
		inf->CurSubNode=inf->CurSubNode->next;
	    }
	}

    return NULL;
    }

/*** xmlGetFirstAttr - get the first attribute name for this object.
 ***/
char*
xmlGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    char* ptr;

	xml_internal_BuildAttributeHashTable(inf);

	/** Set the current attribute. **/
	inf->CurAttr = 0;
	inf->CurXmlAttr=inf->CurNode->properties;
	inf->CurSubNode=xml_internal_GetChildren(inf->CurNode);

	/** Return the next one. **/
	ptr = xmlGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** xmlSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
xmlSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    pStructInf find_inf;

    return -1;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"XML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(val->String) + 1 > 255)
		    {
		    mssError(1,"XML","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,val->String);
	        if (rename(inf->Obj->Pathname->Pathbuf, inf->Pathname) < 0) 
		    {
		    mssError(1,"XML","SetAttr 'name': could not rename structure file node object");
		    return -1;
		    }
	        strcpy(inf->Obj->Pathname->Pathbuf, inf->Pathname);
		}
	    return 0;
	    }

	/** Set content type if that was requested. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"XML","Type mismatch setting attribute '%s' (should be string)", attrname);
		return -1;
		}
	    /** SET THE TYPE HERE, IF APPLICABLE, AND RETURN 0 ON SUCCESS **/
	    return -1;
	    }

	/** DO YOUR SEARCHING FOR ATTRIBUTES TO SET HERE **/

	/** Set dirty flag **/
	//inf->Node->Status = SN_NS_DIRTY;

    return 0;
    }


/*** xmlAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
xmlAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    pStructInf new_inf;
    char* ptr;

    return -1;
    }


/*** xmlOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
xmlOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xmlGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
xmlGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xmlGetNextMethod -- same as above.  Always fails. 
 ***/
char*
xmlGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** xmlExecuteMethod - No methods to execute, so this fails.
 ***/
int
xmlExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }

/*** xmlInfo - Return the capabilities of the object
 ***/
int
xmlInfo(void* inf_v, pObjectInfo info)
    {
    pXmlData inf = XML(inf_v);

	info->Flags |= ( OBJ_INFO_F_CAN_HAVE_SUBOBJ | OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CAN_SEEK_FULL |
	    OBJ_INFO_F_CAN_SEEK_REWIND );
	return 0;
    }

/*** xmlInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
xmlInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&XML_INF,0,sizeof(XML_INF));
	xhInit(&XML_INF.cache,17,0);

	/** Setup the structure **/
	strcpy(drv->Name,"XML - XML OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	xaAddItem(&(drv->RootContentTypes),"text/xml");
	xaAddItem(&(drv->RootContentTypes),"application/xml");

	/** Setup the function references. **/
	drv->Open = xmlOpen;
	drv->Close = xmlClose;
	drv->Create = xmlCreate;
	drv->Delete = xmlDelete;
	drv->OpenQuery = xmlOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = xmlQueryFetch;
	drv->QueryClose = xmlQueryClose;
	drv->Read = xmlRead;
	drv->Write = xmlWrite;
	drv->GetAttrType = xmlGetAttrType;
	drv->GetAttrValue = xmlGetAttrValue;
	drv->GetFirstAttr = xmlGetFirstAttr;
	drv->GetNextAttr = xmlGetNextAttr;
	drv->SetAttrValue = xmlSetAttrValue;
	drv->AddAttr = xmlAddAttr;
	drv->OpenAttr = xmlOpenAttr;
	drv->GetFirstMethod = xmlGetFirstMethod;
	drv->GetNextMethod = xmlGetNextMethod;
	drv->ExecuteMethod = xmlExecuteMethod;
	drv->PresentationHints = NULL;
	drv->Info = xmlInfo;

	nmRegister(sizeof(XmlData),"XmlData");
	nmRegister(sizeof(XmlQuery),"XmlQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;


    return 0;
    }

MODULE_INIT(xmlInitialize);
MODULE_PREFIX("xml");
MODULE_DESC("XML ObjectSystem Driver");
MODULE_VERSION(0,0,1);
MODULE_IFACE(CX_CURRENT_IFACE);

