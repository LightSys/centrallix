#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"
/** module definintions **/
#include "centrallix.h"
#include <libxml/parser.h>

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

/**CVSDATA***************************************************************

    $Id: objdrv_xml.c,v 1.1 2002/07/29 01:34:52 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/osdrivers/objdrv_xml.c,v $

    $Log: objdrv_xml.c,v $
    Revision 1.1  2002/07/29 01:34:52  jorupp
     * initial commit of XML support


 **END-CVSDATA***********************************************************/

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
	accessible as name/0, name/1, etc. (a diagram follows)
    3. XML Attributes are mapped to Centrallix attributes
    4. XML subnodes are mapped to Centrallix subobjects
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
    </roottag>

    Centrallix view of above (file is /file.xml):
	(/'s are the path separator, :'s are the property separator)
    /file.xml/tagA=Object
    /file.xml/tagA/0=Object
    /file.xml/tagA/0:prop="bob"
    /file.xml/tagA/0:propF="billy"
    /file.xml/tagA/0:tagB="Hello there"
    /file.xml/tagA/0:tagC="Harry"
    /file.xml/tagA/0/tagB=Object (content="Hello there")
    /file.xml/tagA/0/tagB/0=Object (content="Hello there")
    /file.xml/tagA/0/tagC=Object (content="Harry")
    /file.xml/tagA/0/tagC:propC="bob"
    /file.xml/tagA/0/tagC/0=Object (content="Harry")
    /file.xml/tagA/0/tagC/0:propC="bob"
    /file.xml/tagA/1:prop="house"
    /file.xml/tagA/1:propF="thekid"

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

/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    xmlNodePtr	CurNode;
    char	Element[XML_ELEMENT_SIZE];
    int		Offset;
    int		CurAttr;
    xmlAttrPtr	CurXmlAttr;
    xmlNodePtr	CurSubNode;
    /** GetAttrValue has to return a refence to memory that won't be free()ed **/
    char	AttrValue[XML_ATTR_SIZE];
    pXHashTable	Elements;
    }
    XmlData, *pXmlData;


#define XML(x) ((pXmlData)(x))

/** The element for the HashTable in the Query structure **/
typedef struct
    {
    int		count;
    char	processed;
    }
    XmlQueryHE, *pXmlQueryHE;
    
/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pXmlData	Data;
    //char	NameBuf[256];
    int		ItemCnt;
    xmlNodePtr	NextNode;
    pXHashTable	Elements;
    }
    XmlQuery, *pXmlQuery;

/*** GLOBALS ***/
struct
    {
    int		dmy_global_variable;
    }
    XML_INF;

#if 0
/*** xml_internal_DetermineType - determine the object type being opened and
 *** setup the table, row, etc. pointers. 
 ***/
int
xml_internal_DetermineType(pObject obj, pDatData inf)
    {
    int i;

	/** Determine object type (depth) and get pointers set up **/
	obj_internal_CopyPath(&(inf->Pathname),obj->Pathname);
	for(i=1;i<inf->Pathname.nElements;i++) *(inf->Pathname.Elements[i]-1) = 0;
	inf->TablePtr = NULL;
	inf->TableSubPtr = NULL;
	inf->RowColPtr = NULL;

	inf->TablePtr = inf->Pathname.Elements[obj->SubPtr-1];

	/** Set up pointers based on number of elements past the node object **/
	inf->Type = DAT_T_TABLE;
	obj->SubCnt = 1;
	if (inf->Pathname.nElements - 1 >= obj->SubPtr)
	    {
	    obj->SubCnt = 2;
	    inf->TableSubPtr = inf->Pathname.Elements[obj->SubPtr];
	    if (!strncmp(inf->TableSubPtr,"rows",4)) inf->Type = DAT_T_ROWSOBJ;
	    else if (!strncmp(inf->TableSubPtr,"columns",7)) inf->Type = DAT_T_COLSOBJ;
	    else 
		{
		mssError(1,"DAT","Only two child objects of a table are 'rows' and 'columns'");
		if (inf->Pathname.OpenCtlBuf) nmSysFree(inf->Pathname.OpenCtlBuf);
		inf->Pathname.OpenCtlBuf = NULL;
		return -1;
		}
	    }
	if (inf->Pathname.nElements - 2 >= obj->SubPtr)
	    {
	    obj->SubCnt = 3;
	    inf->RowColPtr = inf->Pathname.Elements[obj->SubPtr+1];
	    if (inf->Type == DAT_T_ROWSOBJ) inf->Type = DAT_T_ROW;
	    else if (inf->Type == DAT_T_COLSOBJ) inf->Type = DAT_T_COLUMN;
	    }
	}

    return 0;
    }
#endif

/*** xml_internal_GetNode - given an inf->CurNode as root, 
 ***   heads down the path in obj to find the node referened to
 ***/
void
xml_internal_GetNode(pXmlData inf,pObject obj)
    {
    xmlNodePtr p;

    /** don't want to go wandering off farther than we need **/
    if(obj->Pathname->nElements<obj->SubPtr+obj->SubCnt)
	return;
    
    p=inf->CurNode->children;
    if(p)
	{
	int i=0; /* Number of nodes that match */
	char *ptr; /* passed to strtol for so we can tell if a number was grabbed */
	int target=-1; /* the element number we're looking for */
	int flag=0; /* did we find the target? */
	char searchElement[XML_ELEMENT_SIZE];
	strncpy(searchElement,obj_internal_PathPart(obj->Pathname,obj->SubPtr+obj->SubCnt-1,1),XML_ELEMENT_SIZE);
	
	if(XML_DEBUG) printf("looking for %s\n",searchElement);
	if(obj->Pathname->nElements>obj->SubPtr+obj->SubCnt)
	    {
	    char* p1;
	    p1=obj_internal_PathPart(obj->Pathname,obj->SubPtr+obj->SubCnt,1);
	    if(XML_DEBUG) printf("subtarget: %s\n",p1);
	    target=strtol(p1,&ptr,10);
	    //target=atoi(p1);
	    /** if valid number was not grabbed, set target negative so it won't match **/
	    if(ptr[0]=='\0')
		{
		if(XML_DEBUG) printf("we've got a target: %i\n",target);
		}
	    else 
		{
		target=-1;
		if(XML_DEBUG) printf("unable to resolve target: %i\n",target);
		}
	    }
	do
	    {
	    if(!strcmp(p->name,searchElement))
		{
		if(i==target)
		    {
		    inf->CurNode=p;
		    flag=1;
		    break;
		    }
		i++;
		}
	    } while  ((p=p->next)!=NULL);
	if( (flag==0) && (i==0 || target>=i) )
	    {
	    /** we couldn't find the node in quetion.
	     **   Maybe there's some content here for another osdriver
	     **/
	    if(XML_DEBUG) printf("There were children, but couldn't find any nodes, or a high enough one (%i < %i)\n",i-1,target);
	    return;
	    }
	else
	    {
	    if(flag==1)
		{
		/** we found some nodes that match, and the specific requested one **/
		if(XML_DEBUG) printf("found matches, and exact target: %i\n",target);
		obj->SubCnt+=2;
		inf->CurNode=p;
		//inf->NumElements=0;
		inf->Element[0]='\0';
		/** maybe there are more **/
		xml_internal_GetNode(inf,obj);
		}
	    else
		{
		/** we found some nodes that match **/
		if(i==1 && target==-1)
		    {
		    /** there was only one match, and there wasn't a number at the next level
		     **  -- return the one match **/
		    p=inf->CurNode->children;
		    do
			{
			if(!strcmp(p->name,searchElement))
			    {
			    inf->CurNode=p;
			    flag=1;
			    break;
			    }
			} while  ((p=p->next)!=NULL);
		    
		    if(XML_DEBUG) printf("found matches, and assumed target\n");
		    obj->SubCnt+=1;
		    inf->CurNode=p;
		    //inf->NumElements=0;
		    inf->Element[0]='\0';
		    /** maybe there are more **/
		    xml_internal_GetNode(inf,obj);
		    }
		else
		    {
		    /** multiple matches w/ no target -- this node will have no content or attributes.
		     **   since it doesn't _really_ exist in the tree.  It will only have queries run on it
		     **/
		    if(XML_DEBUG) printf("found multiple matches, but had no target.\n");
		    strncpy(inf->Element,obj->Pathname->Elements[obj->SubPtr+obj->SubCnt-1],XML_ELEMENT_SIZE);
		    //inf->NumElements=i;
		    obj->SubCnt++;
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
	return;
	}
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
    char* ptr;
    xmlParserCtxtPtr ctxt;
    int bytes;

	/** Allocate the structure **/
	inf = (pXmlData)nmMalloc(sizeof(XmlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(XmlData));
	inf->Obj = obj;
	inf->Mask = mask;


	/** Set object params. **/
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));

	xmlKeepBlanksDefault (0);
	/** parse the document **/
	ptr=malloc(XML_BLOCK_SIZE);
	ctxt=xmlCreatePushParserCtxt(NULL,NULL,NULL,0,"unknown");
	objRead(obj->Prev,ptr,0,0,FD_U_SEEK);
	while((bytes=objRead(obj->Prev,ptr,XML_BLOCK_SIZE,0,0))>0)
	{
	    if(XML_DEBUG) printf("giving parser a chunk\n");
	    xmlParseChunk(ctxt,ptr,bytes,0);
	    if(XML_DEBUG) printf("parser done with the chunk\n");
	}
	xmlParseChunk(ctxt,NULL,0,1);
	/** get the document reference **/
	if(!ctxt->myDoc)
	    {
	    mssError(0,"XML","No Document in XML file!");
	    return NULL;
	    }
	inf->CurNode=xmlDocGetRootElement(ctxt->myDoc);
	xmlFreeParserCtxt(ctxt);

	obj->SubCnt=1;
	if(XML_DEBUG) printf("objdrv_xml.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	
	xml_internal_GetNode(inf,obj);
	
	if(XML_DEBUG) printf("objdrv_xml.c took: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	free(ptr);

    return (void*)inf;
    }


/*** xmlClose - close an open object.
 ***/
int
xmlClose(void* inf_v, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);

	if(inf->Elements)
	    {
	    /** this structure might not have been allocated **/
	    xhDeInit(inf->Elements);
	    nmFree(inf->Elements,sizeof(XHashTable));
	    }
    	/** Write the node first, if need be. **/
	//snWriteNode(inf->Node);
	
	/** Release the memory **/
	//inf->Node->OpenCnt --;
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
    char *buf;
    int i;

    /** get the data from the tree **/
    buf=xmlNodeListGetString(inf->CurNode->doc,inf->CurNode->xmlChildrenNode,1);

    if(!buf)
	return -1;
    
    /** 'seek' if we're supposed to **/
    if(flags & FD_U_SEEK)
	inf->Offset=offset;

    /** calcuate length to use -- the smaller of the remaining buffer after offset or the maxlen **/
    if(maxcnt>strlen(buf)-inf->Offset)
	i=strlen(buf)-inf->Offset;
    else
	i=maxcnt;

    if(i<0) return -1;
    
    if(buf)
        memcpy(buffer,buf+inf->Offset,i);

    inf->Offset+=i;

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

	/** Allocate the query structure **/
	qy = (pXmlQuery)nmMalloc(sizeof(XmlQuery));
	if (!qy) return NULL;
	memset(qy, 0, sizeof(XmlQuery));
	qy->Data = inf;
	qy->ItemCnt = 0;
	qy->NextNode = NULL;
	if(inf->Element[0]=='\0')
	    {
	    /** we're returning element names, not individual elements **/
	    xmlNodePtr p;
	    p=inf->CurNode->children;
	    qy->Elements=(pXHashTable)nmMalloc(sizeof(XHashTable));
	    memset(qy->Elements, 0, sizeof(XHashTable));
	    xhInit(qy->Elements,17,0);
	    while(p)
		{
		pXmlQueryHE pHE;
		if((pHE=(pXmlQueryHE)xhLookup(qy->Elements,(char*)p->name)))
		    {
		    pHE->count++;
		    }
		else
		    {
		    pHE=(pXmlQueryHE)nmSysMalloc(sizeof(XmlQueryHE));
		    pHE->count=1;
		    pHE->processed=0;
		    xhAdd(qy->Elements,(char*)p->name,(char*)pHE);
		    }
		p=p->next;
		}
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

	/** I _think_ these should be this way... **/
	obj->SubPtr=qy->Data->Obj->SubPtr;
	obj->SubCnt=qy->Data->Obj->SubCnt;
	
	if(XML_DEBUG) printf("QueryFetch was given: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));

	/** Alloc the structure **/
	inf = (pXmlData)nmMalloc(sizeof(XmlData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(XmlData));

	do
	    {
	    if(!qy->NextNode)
		qy->NextNode=qy->Data->CurNode->children;
	    else
		qy->NextNode=qy->NextNode->next;
	    if(!qy->NextNode) 
		{
		nmFree(inf,sizeof(XmlData));
		return NULL;
		}
	    if(qy->Data->Element[0]!='\0')
		{
		/** we're filtering on the element name (because of a non-specific open) **/
		if(!strcmp(qy->Data->Element,qy->NextNode->name))
		    flag=1;
		cnt = snprintf(name,256,"%i",qy->ItemCnt);
		}
	    else
		{
		/** we only want to return each unique element name once **/
		pXmlQueryHE pHE;
		if((pHE=(pXmlQueryHE)xhLookup(qy->Elements,(char*)qy->NextNode->name)))
		    {
		    if(pHE->processed==1)
			flag=0;
		    else
			{
			pHE->processed=1;
			flag=1;
			cnt = snprintf(name,256,"%s",qy->NextNode->name);
			if(pHE->count>1)
			    strncpy(inf->Element,qy->NextNode->name,XML_ELEMENT_SIZE);
			}
		    }
		else
		    {
		    /** An element that wasn't here during the preload, but is now..
		     **  I guess we'll ignore it
		     **/
		    flag=0;
		    }
		}
	    } while (flag==0);


	/** make sure we didn't overflow on the path copy **/
	if (cnt<0 || cnt>=256 ) 
	    {
	    mssError(1,"XML","Query result pathname exceeds internal representation");
	    return NULL;
	    }


	/** Shamelessly stolen from objdrv_sybase.c :) **/
	ptr = memchr(obj->Pathname->Elements[obj->Pathname->nElements-1],'\0',256);
	if ((ptr - obj->Pathname->Pathbuf) + 1 + strlen(name) >= 255)
	    {
	    mssError(1,"XML","Pathname too long for internal representation");
	    nmFree(inf,sizeof(XmlData));
	    return NULL;
	    }
	*(ptr++) = '/';
	strcpy(ptr,name);
	obj->Pathname->Elements[obj->Pathname->nElements++] = ptr;
	obj->SubCnt++;

	if(XML_DEBUG) printf("QueryFetch gave back: (%i,%i,%i) %s\n",obj->SubPtr,
		obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	inf->Obj = obj;
	inf->CurNode=qy->NextNode;
	qy->ItemCnt++;

    return (void*)inf;
    }


/*** xmlQueryClose - close the query.
 ***/
int
xmlQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    pXmlQuery qy = ((pXmlQuery)(qy_v));

	if(qy->Elements)
	    {
	    /** this structure might not have been allocated **/
	    xhDeInit(qy->Elements);
	    nmFree(qy->Elements,sizeof(XHashTable));
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

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"last_modification")) return DATA_T_DATETIME;

	/** everything else is a string **/
	return DATA_T_STRING;

    return -1;
    }

void
xml_internal_BuildSubNodeHashTable(pXmlData inf)
    {
    xmlNodePtr p;
    int* pHE;

    if(!inf->Elements && inf->Element[0]=='\0')
	{
	/** if we've built this once already, there's no need to build it all over again... **/
	/**   we're noting element names, not individual elements **/
	/**   in GetNextAttr, we'll only use records from here where the count was 1 **/
	p=inf->CurNode->children;
	inf->Elements=(pXHashTable)nmMalloc(sizeof(XHashTable));
	memset(inf->Elements, 0, sizeof(XHashTable));
	xhInit(inf->Elements,17,sizeof(int*));

	if(XML_DEBUG) printf("walking children to set up hash table\n");
	while(p)
	    {
	    if((pHE=(int*)xhLookup(inf->Elements,(char*)p->name)))
		*pHE++;
	    else
		{
		pHE=(int*)nmSysMalloc(sizeof(int*));
		*pHE=1;
		xhAdd(inf->Elements,(char*)p->name,(char*)pHE);
		}
	    p=p->next;
	    }
#if 0
	else
	    {
	    /** we're returing individual elements, so we need a count of how many are under each **/
	    while(p)
		{
		if(!strcmp(p->name,inf->Element))
		    {
		    /** this is a candidate to be returned **/

		    }
		p=p->next;
		}
	    
	    }
#endif
	}

    }

/*** xmlGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
xmlGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pXmlData inf = XML(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;
    int* pHE;

	/** inner_type is an alias for content_type **/
	if(!strcmp(attrname,"inner_type"))
	    return xmlGetAttrValue(inf_v,"content_type",val,oxt);
    
	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    //printf("returning %s\n",*((char**)val));
	    return 0;
	    }

	/** see if the XML doc has it **/
	ptr=xmlGetProp(inf->CurNode,attrname);
	if(ptr)
	    {
	    strncpy(inf->AttrValue,ptr,XML_ATTR_SIZE);
	    free(ptr);
	    *((char**)val) = inf->AttrValue;
	    return 0;
	    }
	
	/** needed in case this isn't a GetFirstAttribute-style request **/
	xml_internal_BuildSubNodeHashTable(inf);

	/** see if a subnode has it **/
	if(inf->Elements && (pHE=(int*)xhLookup(inf->Elements,attrname)) && *pHE==1)
	    {
	    xmlNodePtr p;
	    p=inf->CurNode->children;
	    while(p && strcmp(p->name,attrname)) p=p->next;
	    if(p)
		{
		ptr=xmlNodeListGetString(p->doc,p->xmlChildrenNode,1);
		if(ptr)
		    {
		    strncpy(inf->AttrValue,ptr,XML_ATTR_SIZE);
		    free(ptr);
		    *((char**)val) = inf->AttrValue;
		    return 0;
		    }
		}
	    }
	

	/** If content-type or outer type, and it wasn't specified in the XML **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"outer_type"))
	    {
	    *((char**)val) = "text/plain";
	    return 0;
	    }

	/** take last_modification from underlying object if it has one **/
	if(!strcmp(attrname,"last_modification"))
	    if(objGetAttrValue(inf->Obj->Prev,"last_modification",val)==0)
		return 0;

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

	mssError(1,"XML","Could not locate requested attribute: %s",attrname);

    return -1;
    }


/*** xmlGetNextAttr - get the next attribute name for this object.
 ***/
char*
xmlGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    char *p;

    switch(inf->CurAttr++)
	{
	case 0: return "name";
	case 1: return "content_type";
	case 2: return "annotation";
	case 3: return "inner_type";
	case 4: return "outer_type";
	case 5: 
	    if(objGetAttrValue(inf->Obj->Prev,"last_modification",((void*)p))==0)
		return "last_modification";
	}
    if(inf->CurXmlAttr)
	{
	p=(char *)inf->CurXmlAttr->name;
	inf->CurXmlAttr=inf->CurXmlAttr->next;
	return p;
	}

    /** just for safety's sake -- really shouldn't be needed **/
    xml_internal_BuildSubNodeHashTable(inf);

    if(inf->CurSubNode && inf->Elements)
	{
	int *pHE;
	if((pHE=(int*)xhLookup(inf->Elements,(char*)inf->CurSubNode->name)) && *pHE==1)
	    {
	    p=(char *)inf->CurSubNode->name;
	    inf->CurSubNode=inf->CurSubNode->next;
	    return p;
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

	xml_internal_BuildSubNodeHashTable(inf);

	/** Set the current attribute. **/
	inf->CurAttr = 0;
	inf->CurXmlAttr=inf->CurNode->properties;
	inf->CurSubNode=inf->CurNode->children;

	/** Return the next one. **/
	ptr = xmlGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** xmlSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
xmlSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    pXmlData inf = XML(inf_v);
    pStructInf find_inf;

    return -1;

	/** Choose the attr name **/
	/** Changing name of node object? **/
	if (!strcmp(attrname,"name"))
	    {
	    if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	        {
	        if (!strcmp(inf->Obj->Pathname->Pathbuf,".")) return -1;
	        if (strlen(inf->Obj->Pathname->Pathbuf) - 
	            strlen(strrchr(inf->Obj->Pathname->Pathbuf,'/')) + 
		    strlen(*(char**)(val)) + 1 > 255)
		    {
		    mssError(1,"XML","SetAttr 'name': name too large for internal representation");
		    return -1;
		    }
	        strcpy(inf->Pathname, inf->Obj->Pathname->Pathbuf);
	        strcpy(strrchr(inf->Pathname,'/')+1,*(char**)(val));
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
xmlExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
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
	XML_INF.dmy_global_variable = 0;

	/** Setup the structure **/
	strcpy(drv->Name,"XML - XML OS Driver");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),16);
	xaAddItem(&(drv->RootContentTypes),"text/xml");

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

