#if 0
 vim:softtabstop=4:shiftwidth=4:noexpandtab:
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "stparse.h"
#include "st_node.h"
#include "mtsession.h"
#include "centrallix.h"
#include "mtlexer.h"

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
/* Module: 	objdrv_http.c						*/
/* Author:	Jonathan Rupp						*/
/* Creation:	June 09, 2002						*/
/* Description:	A driver to take files and directories			*/
/*		available via the HTTP protocol and map them into	*/
/*		the Centrallix objectsystem				*/
/*									*/
/************************************************************************/

/**CVSDATA***************************************************************


 **END-CVSDATA***********************************************************/


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[256];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
    char	*ProxyServer;
    char	*ProxyPort;
    char	*ProxyAuthLine;
    char	*AuthLine;
    int		AllowSubDirs;
    int		AllowRedirects;
    pStructInf	Attr;
    int		NextAttr;
    char	*Server;
    char	*Port;
    char	*Path;
    pFile	Socket;
    char	Redirected;
    DateTime	LastModified;
    char	*Annotation;
    }
    HttpData, *pHttpData;

    
#define HTTP(x) ((pHttpData)(x))

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pHttpData	Data;
    char	NameBuf[256];
    int		ItemCnt;
    }
    HttpQuery, *pHttpQuery;

/*** GLOBALS ***/
#if 0
    struct
    {
    }
    HTTP_INF;
#endif
    
/*** http_internal_Cleanup - deallocates all memory allocated
 ***/
void*
http_internal_Cleanup(pHttpData inf,char *line)
    {
    if(inf)
	{
#define HTTP_CLEANUP(i) \
	if(i) \
	    free(i);
	HTTP_CLEANUP(inf->ProxyServer);
	HTTP_CLEANUP(inf->ProxyPort);
	HTTP_CLEANUP(inf->ProxyAuthLine);
	HTTP_CLEANUP(inf->Server);
	HTTP_CLEANUP(inf->Port);
	HTTP_CLEANUP(inf->Path);
	HTTP_CLEANUP(inf->AuthLine);
	HTTP_CLEANUP(inf->Annotation);
	if(inf->Socket)
	    netCloseTCP(inf->Socket,0,0);
	if(inf->Attr)
	    stFreeInf(inf->Attr);

	nmFree(inf,sizeof(HttpData));
	}
    mssError(0,"HTTP",line);
    return NULL;
    }

int
http_internal_ParseDate(pDateTime dt, const char *str)
    {
    //I quickly hacked this out because
    char* obj_short_months[] = {"Jan","Feb","Mar","Apr","May","Jun",
				"Jul","Aug","Sep","Oct","Nov","Dec"};
    char *p;
    char *p2;
    char **ar;
    int off=0;
    int i;
    p2=p=(char*)malloc(strlen(str)+1);
    ar=(char**)malloc(sizeof(char*)*8);
    if(!p || !ar) return -1;
    strcpy(p,str);
    ar[off]=p;
    while(p2-p<strlen(p))
	{
	if(p2[0]==' ' || p2[0]==':')
	    {
	    off++;
	    ar[off]=p2+1;
	    }
	p2++;
	}
    for(i=1;i<8;i++)
	{
	(ar[i]-1)[0]='\0';
	}
    dt->Part.Day=atoi(ar[1]);
    for(i=0;i<12;i++)
	{
	if(!strcmp(ar[2],obj_short_months[i]))
	    dt->Part.Month=i;
	}
    dt->Part.Year=atoi(ar[3])-1900;
    /** HACK FOR TIMEZONE -- FIXME!!!!! **/
    if(atoi(ar[4])-5<0) { dt->Part.Hour=24+atoi(ar[4])-5; dt->Part.Day--; } 
    else 
	dt->Part.Hour=atoi(ar[4])-5;
    dt->Part.Minute=atoi(ar[5]);
    dt->Part.Second=atoi(ar[6]);
    /** HACK -- it's displaying 1 day higher than it should -- FIXME**/
    dt->Part.Day--;
    free(p);
    free(ar);
    return 0;
    }

int
http_internal_GetPageStream(pHttpData inf)
    {
    pLxSession lex;
    char flag;
    int toktype;
#define BUF_SIZE 256
    char buf[BUF_SIZE];
    char *fullpath; // the path to be send to the server
    char *ptr;
    char *ptr2;
    int alloc;
    char *p1;
    char *p2;
    short status=0;
    pStructInf attr;

    /** decide what path we're going for.  Once we've been redirected, 
	SubCnt is locked, and the server path is altered 
	and will just follow redirects **/
    if(inf->Redirected || inf->Obj->SubCnt==1)
	ptr=NULL;
    else		    
        ptr=obj_internal_PathPart(inf->Obj->Pathname,inf->Obj->SubPtr,inf->Obj->SubCnt-1);
    if(ptr)
	{
	if(inf->Path[0])
	    {
	    fullpath=(char*)malloc(strlen(inf->Path)+strlen(ptr)+3);
	    sprintf(fullpath,"/%s/%s",inf->Path,ptr);
	    }
	else
	    {
	    fullpath=(char*)malloc(strlen(ptr)+2);
	    sprintf(fullpath,"/%s",ptr);
	    }
	}
    else
	{
	if(inf->Path[0])
	    {
	    fullpath=(char*)malloc(strlen(inf->Path)+2);
	    sprintf(fullpath,"/%s",inf->Path);
	    }
	else
	    {
	    fullpath=(char*)malloc(2);
	    sprintf(fullpath,"/");
	    }
	}

    //printf("requesting: %s\n",fullpath);

    /** Set up connection **/
    if(inf->ProxyServer[0])
	{
	inf->Socket=netConnectTCP(inf->ProxyServer,inf->ProxyPort,0);
	if(!inf->Socket)
	    return (int)http_internal_Cleanup(inf,"could not connect to proxy server");
	if(inf->Port[0])
	    {
	    snprintf(buf,256,"GET http://%s:%s%s HTTP/1.1\n",inf->Server,inf->Port,fullpath);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    sprintf(buf,"Host: %s:%s\n",inf->Server,inf->Port);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    }
	else
	    {
	    sprintf(buf,"GET http://%s%s HTTP/1.1\n",inf->Server,fullpath);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    sprintf(buf,"Host: %s\n",inf->Server);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    }
	if(inf->ProxyAuthLine[0])
	    {
	    fdWrite(inf->Socket,inf->ProxyAuthLine,strlen(inf->ProxyAuthLine),0,FD_U_PACKET);
	    fdWrite(inf->Socket,"\n",1,0,FD_U_PACKET);
	    }
	if(inf->AuthLine[0])
	    {
	    fdWrite(inf->Socket,inf->AuthLine,strlen(inf->AuthLine),0,FD_U_PACKET);
	    fdWrite(inf->Socket,"\n",1,0,FD_U_PACKET);
	    }
	fdWrite(inf->Socket,"\n",1,0,FD_U_PACKET);
	}
    else
	{
	if(inf->Port[0])
	    inf->Socket=netConnectTCP(inf->Server,inf->Port,0);
	else
	    inf->Socket=netConnectTCP(inf->Server,"80",0);
	if(!inf->Socket)
	    return (int)http_internal_Cleanup(inf,"could not connect to server");
	sprintf(buf,"GET %s HTTP/1.1\n",fullpath);
	fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	if(inf->Port[0])
	    {
	    sprintf(buf,"Host: %s:%s\n",inf->Server,inf->Port);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    }
	else
	    {
	    sprintf(buf,"Host: %s\n",inf->Server);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    }
	if(inf->AuthLine[0])
	    {
	    fdWrite(inf->Socket,inf->AuthLine,strlen(inf->AuthLine),0,FD_U_PACKET);
	    fdWrite(inf->Socket,"\n",1,0,FD_U_PACKET);
	    }
	fdWrite(inf->Socket,"\n",1,0,FD_U_PACKET);
	}

    lex=mlxOpenSession(inf->Socket,MLX_F_LINEONLY | MLX_F_NODISCARD);
    if(!lex)
	http_internal_Cleanup(inf,"Unable to open Lexer Session");

    flag=1;	// Normal status
    while(flag)
	{
	toktype=mlxNextToken(lex);
	if(toktype==MLX_TOK_ERROR)
	    {
	    mlxCloseSession(lex);
	    http_internal_Cleanup(inf,"Lexer Error");
	    }
	alloc=0;
	ptr2=ptr=mlxStringVal(lex,&alloc);
	if(ptr[0]=='\n' || (ptr[0]=='\r' && ptr[1]=='\n'))
	    flag=0;
	else
	    {
	    while(ptr2[0]!=':' && ptr2[0])
		ptr2++;
	    if(ptr2[0])
		{
		ptr2[0]='\0';   // ptr is now the header item, ptr2 is the value
		ptr2++;
		if(ptr2[0]==' ') ptr2++;
		if(ptr2[strlen(ptr2)-1]=='\n') ptr2[strlen(ptr2)-1]='\0';
		if(ptr2[strlen(ptr2)-1]=='\r') ptr2[strlen(ptr2)-1]='\0';
		if(status/100==3)
		    {
		    if(!strcmp(ptr,"Location"))
			{   // this is VERY trusting parsing code....
			    //   is there a module with some in?
			char *server;
			char *port;
			char *path;
			int numslash=0;
			int colon=0;
			/** Process ptr2 (the place to go to) **/
			server=ptr2;
			while(numslash<2)
			    {
			    if(server[0]=='/') numslash++;
			    server++;
			    }
			port=server;
			while(numslash<3 && colon==0)
			    {
			    if(port[0]=='/') numslash++;
			    if(port[0]==':') colon++;
			    if(numslash<3) port++;
			    }
			path=server;
			numslash=2;
			while(numslash<3)
			    {
			    if(path[0]=='/') numslash++;
			    path++;
			    }
			free(inf->Path);
			free(inf->Port);
			free(inf->Server);

			inf->Path=(char*)malloc(strlen(path)+1);
			if(!inf->Path) return (int)http_internal_Cleanup(inf,"malloc error");
			strcpy(inf->Path,path);

			(path-1)[0]='\0';
			inf->Port=(char*)malloc(strlen(port)+1);
			if(!inf->Port) return (int)http_internal_Cleanup(inf,"malloc error");
			strcpy(inf->Port,port);
			
			if(port[0]!='\0') (port-1)[0]='\0';
			inf->Server=(char*)malloc(strlen(server)+1);
			if(!inf->Server) return (int)http_internal_Cleanup(inf,"malloc error");
			strcpy(inf->Server,server);

			inf->Redirected=1;
			return 2; //unsuccessfull -- try again with updated params
			}
		    }
		if(status/100==2)
		    {
		    p1=(char*)nmMalloc(strlen(ptr)+1);
		    p2=(char*)nmMalloc(strlen(ptr2)+1);
		    if(!p1 || !p2) http_internal_Cleanup(inf,"malloc failed");
		    strcpy(p1,ptr);
		    strcpy(p2,ptr2);

		    attr=stAddAttr(inf->Attr,p1);
		    stAddValue(attr,p2,0);
		    if(!strcmp(p1,"Last-Modified"))
			{
			http_internal_ParseDate(&(inf->LastModified),p2);
		    /*** objDataToDateTime looked like what I should use, but if I used it,
		     ***   centrallix would hang on access to this object
		     ***/
			/*
			objDataToDateTime(DATA_T_STRING,p2,&(inf->LastModified),
					    "DDD, dd MMM yyyy HH:mm:ss GMT");
			*/
			}
		    }

		}
	    else
		{   /** Look for HTTP -- first line of header if server is >=HTTP/1.0 **/
		if(ptr[0]=='H' && ptr[1]=='T' && ptr[2]=='T' && ptr[3]=='P')
		    {
		    p1=ptr;
		    while(p1[0]!=' ' && p1-ptr<strlen(ptr)) p1++;
		    if(p1-ptr==strlen(ptr)) return -1;
		    p1++;
		    status=atoi(p1);
		   
		    if(status/100==2)
			{
			inf->Annotation=(char*)nmMalloc(strlen(inf->Server)+strlen(inf->Port)+strlen(inf->Path)+10);
			if(!inf->Annotation) http_internal_Cleanup(inf,"malloc failed");
			if(inf->Port[0])
			    sprintf(inf->Annotation,"http://%s:%s/%s",inf->Server,inf->Port,inf->Path);
			else
			    sprintf(inf->Annotation,"http://%s/%s",inf->Server,inf->Path);

			}	
		    if(status/100==4 || status/100==5 || (status/100==3 && !inf->AllowRedirects) )
			{
			if(inf->Redirected || inf->Obj->SubCnt<=1) // nothing more to try, fail
			    {//don't need to check AllowSubDirs here -- SubCnt==1 if !AllowSubDirs
			    if(alloc) nmSysFree(ptr);
			    mlxCloseSession(lex);
			    return 0;   //failure
			    }
			else
			    {
			    if(alloc) nmSysFree(ptr);
			    mlxCloseSession(lex);
			    inf->Obj->SubCnt--;
			    return 2;   // unsuccessful -- try up one more level
			    }
			}
		    }
		else
		    printf("Could not parse header line:%s\n",ptr);
		}
	    }
	if(alloc) nmSysFree(ptr);
	}
    mlxCloseSession(lex);

    return 1;

    }

/*** httpOpen - open an object.
 ***/
void*
httpOpen(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    pHttpData inf;
    short rval;
    pSnNode node = NULL;
    char* ptr;

	/** Allocate the structure **/
	inf = (pHttpData)nmMalloc(sizeof(HttpData));
	if (!inf) return NULL;
	memset(inf,0,sizeof(HttpData));
	inf->Obj = obj;
	inf->Mask = mask;

	//printf("objdrv_http.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
	//	obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	obj->SubCnt=obj->Pathname->nElements-obj->SubPtr+1; // Grab everything...

	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    nmFree(inf,sizeof(HttpData));
	    mssError(0,"HTTP","Could not open structure file");
	    return NULL;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;
	/** I was too lazy to retype this 7 times... **/
#define HTTP_PARAM_INIT(i,s) \
	if(stAttrValue(stLookup(node->Data,(s)),NULL,&ptr,0)<0) ptr=""; \
	(i)=(char*)malloc(strlen(ptr)+1); \
	if(!(i))\
	    return http_internal_Cleanup(inf,"malloc failure");\
	strcpy((i),ptr);

	HTTP_PARAM_INIT(inf->ProxyServer,"proxyserver");
	HTTP_PARAM_INIT(inf->ProxyPort,"proxyport");
	HTTP_PARAM_INIT(inf->ProxyAuthLine,"proxyauthline");
	HTTP_PARAM_INIT(inf->Server,"server");
	HTTP_PARAM_INIT(inf->Port,"port");
	HTTP_PARAM_INIT(inf->Path,"path");
	HTTP_PARAM_INIT(inf->AuthLine,"authline");
	if(stAttrValue(stLookup(node->Data,"allowsubdirs"),&inf->AllowSubDirs,NULL,0)<0)
	    inf->AllowSubDirs=1;
	if(stAttrValue(stLookup(node->Data,"allowredirects"),&inf->AllowRedirects,NULL,0)<0)
	    inf->AllowRedirects=1;

	if(!inf->AllowSubDirs)
	    inf->Obj->SubCnt=1;

	/** Ensure all required parameters are given **/
	if(!inf->Server[0])
	    return http_internal_Cleanup(inf,"server is required");
	if((inf->ProxyServer[0] && !inf->ProxyPort[0])||(!inf->ProxyServer[0] && inf->ProxyPort[0]))
	    return http_internal_Cleanup(inf,"either supply both proxyserver and proxyport, or neither");
	inf->Attr=stCreateStruct("","");
	while((rval=http_internal_GetPageStream(inf))==2)
	    printf("retrying...%i\n",inf->Redirected);
	if(rval==0)
	    return http_internal_Cleanup(inf,"failed to load target");

    return (void*)inf;
    }
/*** httpClose - close an open object.
 ***/
int
httpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);

	/** Release the memory **/
	inf->Node->OpenCnt --;
	nmFree(inf,sizeof(HttpData));

    return 0;
    }


/*** httpCreate - create a new object, without actually returning a
 *** descriptor for it.  For most drivers, it is safe to just call
 *** the Open method with create/exclude set, and then close the
 *** object immediately.
 ***/
int
httpCreate(pObject obj, int mask, pContentType systype, char* usrtype, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** httpDelete - delete an existing object.  For most drivers, it works to
 *** call open() first to make sure the thing exists and get information
 *** on it, and then "handle the close a bit differently" :)
 ***/
int
httpDelete(pObject obj, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** httpRead - Read from the TCP stream that is this file request
 ***/
int
httpRead(void* inf_v, char* buffer, int maxcnt, int offset, int flags, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);
    if(!inf->Socket) return -1;
    return fdRead(inf->Socket,buffer,maxcnt,offset,flags);
    }


/*** httpWrite - Can't write to a file over HTTP
 ***/
int
httpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** httpOpenQuery - open a directory query.  This driver is pretty 
 *** unintelligent about queries.  So, we leave the query matching logic
 *** to the ObjectSystem management layer in this case.
 ***/
void*
httpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** httpQueryFetch - get the next directory entry as an open object.
 ***/
void*
httpQueryFetch(void* qy_v, pObject obj, int mode, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** httpQueryClose - close the query.
 ***/
int
httpQueryClose(void* qy_v, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** httpGetAttrType - get the type (DATA_T_http) of an attribute by name.
 ***/
int
httpGetAttrType(void* inf_v, char* attrname, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);
    int i;
    pStructInf find_inf;

    	/** If name, it's a string **/
	if (!strcmp(attrname,"name")) return DATA_T_STRING;

	/** If 'content-type', it's also a string. **/
	if (!strcmp(attrname,"content_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"annotation")) return DATA_T_STRING;
	if (!strcmp(attrname,"inner_type")) return DATA_T_STRING;
	if (!strcmp(attrname,"outer_type")) return DATA_T_STRING;

	/** Check for attributes in the node object if that was opened **/
	if (inf->Obj->Pathname->nElements == inf->Obj->SubPtr)
	    {
	    }

	if(!strcmp(attrname,"last_modification"))
	    {
	    if(inf->LastModified.Value)
		return DATA_T_DATETIME;
	    else
		return 0;
	    }

	if(stLookup(inf->Attr,attrname)) return DATA_T_STRING;

	//if(xaFindItem(inf->Attributes,attrname)!=-1) return DATA_T_STRING;

	/** Put checking for your own attributes here. **/
	/** You will want to likely make a list of 'em in a global array **/
	/** or something like that. **/

    return -1;
    }


/*** httpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
httpGetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    *((char**)val) = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	if (!strcmp(attrname,"outer_type"))
	    {	// shouldn't we return something like application/http, not the inner type?
	    if(httpGetAttrValue(inf_v,"Content-Type", val, oxt)!=0)
		*((char**)val) = "application/http";
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	/** REPLACE MYOBJECT/TYPE WITH AN APPROPRIATE TYPE. **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if(httpGetAttrValue(inf_v,"Content-Type", val, oxt)!=0)
		*((char**)val) = "application/http";
	    return 0;
	    }

	if (!strcmp(attrname,"annotation"))
	    {
	    if(inf->Annotation)
		{
		*((char**)val)=inf->Annotation;
		return 0;
		}
	    else
		return -1;
	    }

	if(!strcmp(attrname,"last_modification"))
	    {
	    if(inf->LastModified.Value)
		*((pDateTime*)val)=&(inf->LastModified);
	    else
		return 0;
	    }

	if(stLookup(inf->Attr,attrname)) 
	    {
	    return stAttrValue(stLookup(inf->Attr,attrname),NULL,(char**)val,0);
	    }

	if(!strcmp(attrname,"inner_type"))
	    {
	    if(httpGetAttrValue(inf_v,"Content-Type", val, oxt)!=0)
		*((char**)val) = "application/http";
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    *(char**)val = "";
	    return 0;
	    }

    return -1;
    }


/*** httpGetNextAttr - get the next attribute name for this object.
 ***/
char*
httpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pHttpData inf = HTTP(inf_v);

    if(inf->NextAttr<0)
	{
	switch(inf->NextAttr++)
	    {
	    case -6: return "name";
	    case -5: return "content_type";
	    case -4: return "annotation";
	    case -3: return "inner_type";
	    case -2: return "outer_type";
	    case -1:
		     if(inf->LastModified.Value)
			 return "last_modification";
	    }
	}

    if(inf->NextAttr<inf->Attr->nSubInf)
        return inf->Attr->SubInf[inf->NextAttr++]->Name;

    return NULL;
    }


/*** httpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
httpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pHttpData inf = HTTP(inf_v);
    char* ptr;

	/** Set the current attribute. **/
	inf->NextAttr = -6;

	/** Return the next one. **/
	ptr = httpGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** httpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
httpSetAttrValue(void* inf_v, char* attrname, void* val, pObjTrxTree oxt)
    {
    return -1;
    }

/*** httpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
httpAddAttr(void* inf_v, char* attrname, int type, void* val, pObjTrxTree oxt)
    {
    return -1;
    }


/*** httpOpenAttr - open an attribute as if it were an object with content.
 *** Not all objects support this type of operation.
 ***/
void*
httpOpenAttr(void* inf_v, char* attrname, int mode, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** httpGetFirstMethod -- there are no methods yet, so this just always
 *** fails.
 ***/
char*
httpGetFirstMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** httpGetNextMethod -- same as above.  Always fails. 
 ***/
char*
httpGetNextMethod(void* inf_v, pObjTrxTree oxt)
    {
    return NULL;
    }


/*** httpExecuteMethod - No methods to execute, so this fails.
 ***/
int
httpExecuteMethod(void* inf_v, char* methodname, void* param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** httpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
httpInitialize()
    {
    pObjDriver drv;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Setup the structure **/
	strcpy(drv->Name,"HTTP - HTTP Protocol for objectsystem");
	drv->Capabilities = 0;
	xaInit(&(drv->RootContentTypes),1);
	xaAddItem(&(drv->RootContentTypes),"application/http");

	/** Setup the function references. **/
	drv->Open = httpOpen;
	drv->Close = httpClose;
	drv->Create = httpCreate;
	drv->Delete = httpDelete;
	drv->OpenQuery = httpOpenQuery;
	drv->QueryDelete = NULL;
	drv->QueryFetch = httpQueryFetch;
	drv->QueryClose = httpQueryClose;
	drv->Read = httpRead;
	drv->Write = httpWrite;
	drv->GetAttrType = httpGetAttrType;
	drv->GetAttrValue = httpGetAttrValue;
	drv->GetFirstAttr = httpGetFirstAttr;
	drv->GetNextAttr = httpGetNextAttr;
	drv->SetAttrValue = httpSetAttrValue;
	drv->AddAttr = httpAddAttr;
	drv->OpenAttr = httpOpenAttr;
	drv->GetFirstMethod = httpGetFirstMethod;
	drv->GetNextMethod = httpGetNextMethod;
	drv->ExecuteMethod = httpExecuteMethod;
	drv->PresentationHints = NULL;

	nmRegister(sizeof(HttpData),"HttpData");
	nmRegister(sizeof(HttpQuery),"HttpQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }


MODULE_INIT(httpInitialize);
MODULE_PREFIX("http");
MODULE_DESC("HTTP ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

