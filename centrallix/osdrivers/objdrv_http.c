#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <regex.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "stparse.h"
#include "st_node.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "cxlib/mtlexer.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxss/cxss.h"
#include "param.h"
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <time.h>
#include "config.h"
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif

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


/*** one HTTP header ***/
typedef struct
    {
    char	Name[32];
    char*	Value;
    int		ValueAlloc:1;
    }
    HttpHeader, *pHttpHeader;


/*** HTTP object parameter ***/
typedef struct
    {
    pParam	Parameter;	/* standard centrallix parameter data */
    int		Usage;		/* HTTP_PUSAGE_T_xxx */
    int		Source;		/* HTTP_PSOURCE_T_xxx */
    int		PathPart;	/* subpart of path, 0=this, 1=first subelement, 2=second subelement */
    }
    HttpParam, *pHttpParam;

#define HTTP_PUSAGE_T_URL	1	/* parameter is encoded into the url */
#define HTTP_PUSAGE_T_POST	2	/* parameter is encoded into the post data */
#define HTTP_PUSAGE_T_HEADER	3	/* parameter is encoded into an HTTP request header */

#define HTTP_PSOURCE_T_NONE	0	/* parameter has no source - only the provided default */
#define HTTP_PSOURCE_T_PATH	1	/* parameter comes from pathname subpart */
#define HTTP_PSOURCE_T_PARAM	2	/* parameter comes from OSML open parameter */

#define HTTP_DEFAULT_MEM_CACHE	1024*1024


/*** Structure used by this driver internally. ***/
typedef struct 
    {
    char	Pathname[OBJSYS_MAX_PATH];
    int		Flags;
    pObject	Obj;
    int		Mask;
    int		CurAttr;
    pSnNode	Node;
    char	*Server;
    char	*Port;
    char	*Path;
    char	*Method;
    char	*Protocol;
    char	*Cipherlist;
    char	*ProxyServer;
    char	*ProxyPort;
    int		AllowSubDirs;
    int		AllowRedirects;
    int		AllowBadCert;
    int		NextAttr;
    pFile	Socket;
    pFile	SSLReporting;
    int		SSLpid;
    char	Redirected;
    DateTime	LastModified;
    char	*Annotation;
    int		ContentLength;
    int		NetworkOffset;
    int		ReadOffset;
    char*	ContentType;
    XArray	RequestHeaders;		/* of pHttpHeader */
    XArray	ResponseHeaders;	/* of pHttpHeader */
    int		ModDateAlwaysNow;
    XArray	Params;			/* of pHttpParam */
    int		ParamSubCnt;
    int		CacheMinTime;		/* in milliseconds */
    int		CacheMaxTime;		/* in milliseconds */
    pXString	ContentCache;
    int		ContentCacheTS;
    int		ContentCacheMaxLen;
    pParamObjects ObjList;
    }
    HttpData, *pHttpData;

#define HTTP_F_CONTENTCOMPLETE	1

    
#define HTTP(x) ((pHttpData)(x))
#define HTTP_OS_DEBUG		0
#define HTTP_REDIR_MAX		4

/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pHttpData	Data;
    char	NameBuf[OBJSYS_MAX_PATH];
    int		ItemCnt;
    }
    HttpQuery, *pHttpQuery;


/*** GLOBALS ***/
struct
    {
    regex_t	parsehttp;
    regex_t	httpheader;
    SSL_CTX*	SSL_ctx;
    pCxssKeystreamState NonceData;
    }
    HTTP_INF;

char* http_internal_GetHeader(pXArray xa, char* name);


/*** http_internal_GetParamType() - get the data type of a parameter
 ***/
int
http_internal_GetParamType(void* inf_v, char* attrname)
    {
    pHttpData inf = HTTP(inf_v);
    pHttpParam http_param;
    pParam param;
    int i;

	/** Find the param **/
	for(i=0;i<inf->Params.nItems;i++)
	    {
	    http_param = (pHttpParam)inf->Params.Items[i];
	    param = http_param->Parameter;
	    if (!strcmp(attrname, param->Name))
		{
		if (!param->Value)
		    return -1;
		return param->Value->DataType;
		}
	    }

    return -1;
    }


/*** http_internal_GetParamValue() - get the value of a parameter
 ***/
int
http_internal_GetParamValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    pHttpData inf = HTTP(inf_v);
    pHttpParam http_param;
    pParam param;
    int i;

	/** Find the param **/
	for(i=0;i<inf->Params.nItems;i++)
	    {
	    http_param = (pHttpParam)inf->Params.Items[i];
	    param = http_param->Parameter;
	    if (!strcmp(attrname, param->Name))
		{
		if (!param->Value)
		    return 1;
		if (datatype != param->Value->DataType)
		    {
		    mssError(1,"HTTP","Type mismatch accessing parameter '%s'", param->Name);
		    return -1;
		    }
		if (param->Value->Flags & DATA_TF_NULL)
		    return 1;
		objCopyData(&(param->Value->Data), val, datatype);
		return 0;
		}
	    }

    return -1;
    }


/*** http_internal_SetParamValue() - set the value of a parameter - not
 *** supported.
 ***/
int
http_internal_SetParamValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    return -1;
    }


/*** http_internal_GetHdrType() - get the data type of a  request header;
 *** this will always be a string (unless the header does not exist)
 ***/
int
http_internal_GetHdrType(void* inf_v, char* attrname)
    {
    //pHttpData inf = HTTP(inf_v);
    return DATA_T_STRING;
    }


/*** http_internal_GetHdrValue() - get the value of a request header
 ***/
int
http_internal_GetHdrValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    pHttpData inf = HTTP(inf_v);
    char* hdrval;

	/** Look up the header **/
	hdrval = http_internal_GetHeader(&inf->RequestHeaders, attrname);

	/** No such header?  Return null. **/
	if (!hdrval)
	    return 1;

	/** Must be requesting a string value **/
	if (datatype != DATA_T_STRING)
	    {
	    mssError(1,"HTTP","Type mismatch accessing header '%s'", attrname);
	    return -1;
	    }
	val->String = hdrval;

    return 0;
    }


/*** http_internal_SetHdrValue() - set the value of a request header - not
 *** supported.
 ***/
int
http_internal_SetHdrValue(void* inf_v, char* attrname, int datatype, pObjData val)
    {
    return -1;
    }


/*** http_internal_AddRequestHeader - add a header to the HTTP request
 ***/
int
http_internal_AddRequestHeader(pHttpData inf, char* hdrname, char* hdrvalue, int hdralloc, int first)
    {
    pHttpHeader hdr;
    int i;

	/** Already present? **/
	for(i=0;i<inf->RequestHeaders.nItems;i++)
	    {
	    hdr = (pHttpHeader)inf->RequestHeaders.Items[i];
	    if (!strcasecmp(hdr->Name, hdrname))
		{
		if (hdr->ValueAlloc)
		    nmSysFree(hdr->Value);

		/** If null new header value - delete the existing one **/
		if (!hdrvalue)
		    {
		    xaRemoveItem(&inf->RequestHeaders, i);
		    nmFree(hdr, sizeof(HttpHeader));
		    goto done;
		    }

		/** Replace existing header value **/
		hdr->Value = hdrvalue;
		hdr->ValueAlloc = hdralloc;
		goto done;
		}
	    }

	/** Don't add a NULL header **/
	if (!hdrvalue)
	    goto done;
	
	/** Allocate the header **/
	hdr = (pHttpHeader)nmMalloc(sizeof(HttpHeader));
	if (!hdr)
	    return -1;

	/** Set it up and add it **/
	strtcpy(hdr->Name, hdrname, sizeof(hdr->Name));
	hdr->Value = hdrvalue;
	hdr->ValueAlloc = hdralloc;
	if (first)
	    xaInsertBefore(&inf->RequestHeaders, 0, (void*)hdr);
	else
	    xaAddItem(&inf->RequestHeaders, (void*)hdr);

    done:
	if (inf->ObjList)
	    expModifyParam(inf->ObjList, "headers", (void*)inf);
	return 0;
    }


/*** http_internal_FreeHeaders - release HTTP headers from an xarray
 ***/
int
http_internal_FreeHeaders(pXArray xa)
    {
    pHttpHeader hdr;
    int i;

	/** Loop through, removing and freeing the headers **/
	while (xa->nItems)
	    {
	    hdr = (pHttpHeader)xaGetItem(xa, xa->nItems - 1);
	    if (hdr)
		{
		if (hdr->ValueAlloc)
		    nmSysFree(hdr->Value);
		nmFree(hdr, sizeof(HttpHeader));
		}
	    xaRemoveItem(xa, xa->nItems - 1);
	    }

    return 0;
    }


/*** http_internal_GetHeader - find a header with the given name
 ***/
char*
http_internal_GetHeader(pXArray xa, char* name)
    {
    int i;
    pHttpHeader hdr;

	/** Loop through array **/
	for(i=0;i<xa->nItems;i++)
	    {
	    hdr = (pHttpHeader)xa->Items[i];
	    if (!strcasecmp(hdr->Name, name))
		return hdr->Value;
	    }

    return NULL;
    }


/*** http_internal_CloseConnection - shuts down the network
 *** connection used to retrieve data.
 ***/
int
http_internal_CloseConnection(pHttpData inf)
    {

	inf->ContentCacheTS = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	if (inf->SSLpid > 0)
	    {
	    cxssFinishTLS(inf->SSLpid, inf->Socket, inf->SSLReporting);
	    inf->SSLpid = 0;
	    inf->Socket = NULL;
	    inf->SSLReporting = NULL;
	    }
	else
	    {
	    if (inf->Socket)
		netCloseTCP(inf->Socket,1,0);
	    inf->Socket = NULL;
	    }
    
    return 0;
    }

    
/*** http_internal_Cleanup - deallocates all memory allocated
 ***/
void*
http_internal_Cleanup(pHttpData inf)
    {

	if(inf)
	    {
	    /** Free headers **/
	    http_internal_FreeHeaders(&inf->RequestHeaders);
	    xaDeInit(&inf->RequestHeaders);
	    http_internal_FreeHeaders(&inf->ResponseHeaders);
	    xaDeInit(&inf->ResponseHeaders);

	    http_internal_FreeParams(inf);
	    xaDeInit(&inf->Params);

	    /** Free connection data **/
	    if (inf->Server) nmSysFree(inf->Server);
	    inf->Server = NULL;
	    if (inf->Port) nmSysFree(inf->Port);
	    inf->Port = NULL;
	    if (inf->Path) nmSysFree(inf->Path);
	    inf->Path = NULL;
	    if (inf->Method) nmSysFree(inf->Method);
	    inf->Method = NULL;
	    if (inf->Protocol) nmSysFree(inf->Protocol);
	    inf->Protocol = NULL;
	    if (inf->Cipherlist) nmSysFree(inf->Cipherlist);
	    inf->Cipherlist = NULL;
	    if (inf->ProxyServer) nmSysFree(inf->ProxyServer);
	    inf->ProxyServer = NULL;
	    if (inf->ProxyPort) nmSysFree(inf->ProxyPort);
	    inf->ProxyPort = NULL;
	    if (inf->ContentType) nmSysFree(inf->ContentType);
	    inf->ContentType = NULL;

	    /** Close socket, if needed **/
	    http_internal_CloseConnection(inf);

	    if (inf->ObjList)
		expFreeParamList(inf->ObjList);

	    if (inf->ContentCache)
		xsFree(inf->ContentCache);

	    if (inf->Annotation)
		nmSysFree(inf->Annotation);

	    nmFree(inf,sizeof(HttpData));
	    }

    return NULL;
    }


/** taking advantage of the fact that in C, "a" "b" is the same as "ab" **/
/** These definitions come directly from sec3.3.1 of rfc2616 (modified for C identifiers) **/
#define HTTP_date "(" rfc1123_date ")" "|" "(" rfc850_date ")" "|" "(" asctime_date ")"
#define rfc1123_date wkday "," SP date1 SP xtime SP "GMT"
#define rfc850_date weekday "," SP date2 SP xtime SP "GMT"
#define asctime_date wkday SP date3 SP xtime SP DIGIT4
#define date1 DIGIT2 SP month SP DIGIT4
#define date2 DIGIT2 "-" month "-" DIGIT2
#define date3 month SP "(" DIGIT2 "|" "(" SP DIGIT1 ")" ")"
#define xtime DIGIT2 ":" DIGIT2 ":" DIGIT2
#define wkday "(Mon|Tue|Wed|Thu|Fri|Sat|Sun)"
#define weekday "(Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday)"
#define month "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)"
#define DIGIT4 "([0123456789]{4})"
#define DIGIT2 "([0123456789]{2})"
#define DIGIT1 "([0123456789])"
#define SP " "
    
/** b = base string
 ** p = char* to dump data into
 ** m = match #
 **/
#define CPYPMATCH(b,p,m) \
	memset((p),0,pmatch[(m)].rm_eo-pmatch[(m)].rm_so+1);\
	strncpy((p),b+pmatch[(m)].rm_so,pmatch[(m)].rm_eo-pmatch[(m)].rm_so);
/** s = value to assign to
 ** m = match #
 **/
#define PUTMATCHINSTRUCT(s,m) \
	CPYPMATCH(str,temp,m);\
	s=atoi(temp);
/** s = pointer to date object to dump
 **   NOTE: this is all one statement (safe to use w/o curly braces)
 **/
#define DumpDate(s) \
	printf("%02i-%02i-%04i %02i:%02i:%02i\n", \
	    s->Part.Month+1,s->Part.Day+1,s->Part.Year+1900,\
	    s->Part.Hour,s->Part.Minute,s->Part.Second);

/*** http_internal_ParseDate -- an HTTP/1.1 compliant date parser
 ***  -- will detect dates in RFC 1123, RFC 1036, or ANSI C asctime() format
 ***/
int
http_internal_ParseDate(pDateTime dt, const char *str)
    {
    char* obj_short_months[] = {"Jan","Feb","Mar","Apr","May","Jun",
				"Jul","Aug","Sep","Oct","Nov","Dec"};
    char *p;
    char *p2;
    char **ar;
    int off=0;
    int i;
    char temp[256]; // for reporting regcomp errors and temp storage while processing regexec

    regex_t rfc1123date;
    regex_t rfc850date;
    regex_t asctimedate;

    regmatch_t pmatch[11];

	if((i=regcomp(&rfc1123date,rfc1123_date,REG_EXTENDED)))
	    {
	    regerror(i,&rfc1123date,temp,sizeof(temp));
	    mssError(0,"HTTP","Error while building rfc1123date: %s",temp);
	    return -1;
	    }
	if((i=regcomp(&rfc850date,rfc850_date,REG_EXTENDED)))
	    {
	    regerror(i,&rfc850date,temp,sizeof(temp));
	    mssError(0,"HTTP","Error while building rfc850date: %s",temp);
	    return -1;
	    }
	if((i=regcomp(&asctimedate,asctime_date,REG_EXTENDED)))
	    {
	    regerror(i,&asctimedate,temp,sizeof(temp));
	    mssError(0,"HTTP","Error while building asctimedate: %s",temp);
	    return -1;
	    }

	if(HTTP_OS_DEBUG)
	    printf("Date Test: %s\n",str);

	/** RFC1123 Data format? **/
	if(regexec(&rfc1123date,str,11,pmatch,0)==0)
	    {
	    if(HTTP_OS_DEBUG) printf("matched an rfc1123 date\n");
	    if(HTTP_OS_DEBUG)
		for(i=0;i<11;i++)
		    {
		    CPYPMATCH(str,temp,i);
		    printf("%i: '%s'\n",i,temp);
		    }
	    CPYPMATCH(str,temp,2);
	    dt->Part.Day=atoi(temp)-1; /* days are in base 0 -- the 1st is day 0 **/
	    CPYPMATCH(str,temp,3);
	    if(HTTP_OS_DEBUG) printf("temp: %s\n",temp);
	    for(i=0;i<12;i++)
		if(!strcasecmp(obj_short_months[i],temp))
		    break;
	    if(i==12) /* invalid month */
		return -1;
	    dt->Part.Month=i; /* months are in base 0 -- January is month 0 **/
	    CPYPMATCH(str,temp,4);
	    dt->Part.Year=atoi(temp)-1900; /* years are in years since 1900 -- 2002 is 102 **/
	    if(HTTP_OS_DEBUG) printf("temp: %s\n",temp);
	    PUTMATCHINSTRUCT(dt->Part.Hour,5);
	    PUTMATCHINSTRUCT(dt->Part.Minute,6);
	    PUTMATCHINSTRUCT(dt->Part.Second,7);
	    if(HTTP_OS_DEBUG) 
		DumpDate(dt);
	    return 0;
	    }

	/** RFC850 Date format? **/
	if(regexec(&rfc850date,str,11,pmatch,0)==0)
	    {
	    if(HTTP_OS_DEBUG) printf("matched an rfc850 date\n");
	    if(HTTP_OS_DEBUG)
		for(i=0;i<11;i++)
		    {
		    CPYPMATCH(str,temp,i);
		    printf("%i: '%s'\n",i,temp);
		    }
	    CPYPMATCH(str,temp,2);
	    dt->Part.Day=atoi(temp)-1; /* days are in base 0 -- the 1st is day 0 **/
	    CPYPMATCH(str,temp,3);
	    if(HTTP_OS_DEBUG) printf("temp: %s\n",temp);
	    for(i=0;i<12;i++)
		if(!strcmp(obj_short_months[i],temp))
		    break;
	    if(i==12) /* invalid month */
		return -1;
	    dt->Part.Month=i; /* months are in base 0 -- January is month 0 **/
	    CPYPMATCH(str,temp,4);
	    if(atoi(temp)>50) /** warning: 2 digit year -- using 50 as split point **/
		dt->Part.Year=atoi(temp);
	    else
		dt->Part.Year=atoi(temp)+100;
	    if(HTTP_OS_DEBUG) printf("temp: %s\n",temp);
	    PUTMATCHINSTRUCT(dt->Part.Hour,5);
	    PUTMATCHINSTRUCT(dt->Part.Minute,6);
	    PUTMATCHINSTRUCT(dt->Part.Second,7);
	    if(HTTP_OS_DEBUG) 
		DumpDate(dt);
	    return 0;
	    }

	/** asctime() date format? **/
	if(regexec(&asctimedate,str,11,pmatch,0)==0)
	    {
	    if(HTTP_OS_DEBUG) printf("matched an asctime() date\n");
	    if(HTTP_OS_DEBUG)
		for(i=0;i<11;i++)
		    {
		    CPYPMATCH(str,temp,i);
		    printf("%i: '%s'\n",i,temp);
		    }
	    /** 5 is a two-digit day, 6 is a one-digit day **/
	    CPYPMATCH(str,temp,5);
	    if(temp[0]!='\0')
		dt->Part.Day=atoi(temp)-1; /* days are in base 0 -- the 1st is day 0 **/
	    else
		{
		CPYPMATCH(str,temp,6);
		dt->Part.Day=atoi(temp)-1; /* days are in base 0 -- the 1st is day 0 **/
		}
	    CPYPMATCH(str,temp,2);
	    for(i=0;i<12;i++)
		if(!strcmp(obj_short_months[i],temp))
		    break;
	    if(i==12) /* invalid month */
		return -1;
	    dt->Part.Month=i; /* months are in base 0 -- January is month 0 **/
	    CPYPMATCH(str,temp,10);
	    dt->Part.Year=atoi(temp)-1900; /* years are in years since 1900 -- 2002 is 102 **/
	    PUTMATCHINSTRUCT(dt->Part.Hour,7);
	    PUTMATCHINSTRUCT(dt->Part.Minute,8);
	    PUTMATCHINSTRUCT(dt->Part.Second,9);
	    if(HTTP_OS_DEBUG) 
		DumpDate(dt);
	    return 0;
	    }

    return -1;
    }


/*** http_internal_ConnectHttps - make an SSL-encrypted HTTP connection
 ***/
int
http_internal_ConnectHttps(pHttpData inf)
    {
    char msgbuf[256];

	/** Make the network connection **/
	if (inf->Port[0])
	    inf->Socket = netConnectTCP(inf->Server, inf->Port, 0);
	else
	    inf->Socket = netConnectTCP(inf->Server, "443", 0);
	if (!inf->Socket)
	    {
	    mssErrorErrno(1, "HTTP", "Could not connect to server %s:%s", inf->Server, inf->Port[0]?inf->Port:"443");
	    return -1;
	    }

	/** Start up TLS on the connection **/
	inf->SSLpid = cxssStartTLS(HTTP_INF.SSL_ctx, &inf->Socket, &inf->SSLReporting, 0, inf->Server);
	if (inf->SSLpid <= 0)
	    {
	    netCloseTCP(inf->Socket,1,0);
	    inf->Socket = NULL;
	    mssError(0, "HTTP", "Could not start SSL/TLS session");
	    return -1;
	    }

	/** Wait for initial TLS startup message **/
	if (cxssStatTLS(inf->SSLReporting, msgbuf, sizeof(msgbuf)) < 0)
	    {
	    http_internal_CloseConnection(inf);
	    mssError(1, "HTTP", "Could not start SSL/TLS session");
	    return -1;
	    }
	if (msgbuf[0] == '!')
	    {
	    http_internal_CloseConnection(inf);
	    mssError(1, "HTTP", "Could not start SSL/TLS session: %s", msgbuf+1);
	    return -1;
	    }

    return 0;
    }


/*** http_internal_ConnectHttp - make a plain vanilla HTTP connection
 ***/
int
http_internal_ConnectHttp(pHttpData inf)
    {

	if (inf->ProxyServer[0])
	    {
	    /** Proxy **/
	    inf->Socket = netConnectTCP(inf->ProxyServer, (inf->ProxyPort[0])?inf->ProxyPort:"80", 0);
	    if (!inf->Socket)
		{
		mssError(0, "HTTP", "Could not connect to proxy server %s on port %s", inf->ProxyServer, (inf->ProxyPort[0])?inf->ProxyPort:"80");
		return -1;
		}
	    }
	else
	    {
	    /** Standard **/
	    inf->Socket = netConnectTCP(inf->Server, (inf->Port[0])?inf->Port:"80", 0);
	    if (!inf->Socket)
		{
		mssError(0, "HTTP", "Could not connect to server %s on port %s", inf->Server, (inf->Port[0])?inf->Port:"80");
		return -1;
		}
	    }

    return 0;
    }


/*** http_internal_SendRequest - send a HTTP request
 ***/
int
http_internal_SendRequest(pHttpData inf, char* path)
    {
    int rval;
    int i, n_output;
    pHttpHeader hdr;
    char* nonce;
    unsigned char* keydata;
    int cnt;
    unsigned char noncelen;
    int hashpos;
    char hexval[17] = "0123456789abcdef";
    pXString post_params = NULL;
    pXString url_params = NULL;
    pHttpParam one_http_param;
    char reqlen[24];

	/** Compute header nonce.  This is used for functionally nothing, but
	 ** it causes the content and offsets to values in the header to change
	 ** with each request; this can help frustrate certain types of 
	 ** cryptographic attacks.
	 **/
	if (inf->SSLpid && HTTP_INF.NonceData)
	    {
	    keydata = nmSysMalloc(128+8+1);
	    nonce = nmSysMalloc(256+16+1);
	    cxssKeystreamGenerate(HTTP_INF.NonceData, &noncelen, 1);
	    cnt = noncelen;
	    cnt += 16;
	    cxssKeystreamGenerate(HTTP_INF.NonceData, keydata, cnt / 2 + 1);
	    for(i=0;i<cnt;i++)
		{
		if (i & 1)
		    nonce[i] = hexval[(keydata[i/2] & 0xf0) >> 4];
		else
		    nonce[i] = hexval[keydata[i/2] & 0x0f];
		}
	    nonce[cnt] = '\0';
	    http_internal_AddRequestHeader(inf, "X-Nonce", nonce, 1, 1);
	    nmSysFree(keydata);
	    }

	/** Add any parameterized request headers **/
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = xaGetItem(&inf->Params, i);
	    if (one_http_param->Usage == HTTP_PUSAGE_T_HEADER)
		{
		if (!(one_http_param->Parameter->Value->Flags & DATA_TF_NULL))
		    {
		    if (one_http_param->Parameter->Value->DataType == DATA_T_STRING)
			{
			if (one_http_param->Parameter->Value->Data.String)
			    http_internal_AddRequestHeader(inf, one_http_param->Parameter->Name, nmSysStrdup(one_http_param->Parameter->Value->Data.String), 1, 0);
			}
		    else
			{
			mssError(1, "HTTP", "Unsupported data type for '%s' header", one_http_param->Parameter->Name);
			}
		    }
		}
	    }

	/** Build the URL parameters **/
	url_params = xsNew();
	if (!url_params)
	    goto error;
	for(n_output=i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = xaGetItem(&inf->Params, i);
	    if (one_http_param->Usage == HTTP_PUSAGE_T_URL)
		{
		if (!(one_http_param->Parameter->Value->Flags & DATA_TF_NULL))
		    {
		    /** Encode one parameter into the post data **/
		    xsConcatenate(url_params, n_output?"&":"?", 1);
		    xsConcatQPrintf(url_params, "%STR&URL=", one_http_param->Parameter->Name);
		    if (one_http_param->Parameter->Value->DataType == DATA_T_STRING)
			xsConcatQPrintf(url_params, "%STR&URL", one_http_param->Parameter->Value->Data.String);
		    else if (one_http_param->Parameter->Value->DataType == DATA_T_INTEGER)
			xsConcatQPrintf(url_params, "%INT", one_http_param->Parameter->Value->Data.Integer);
		    else
			{
			mssError(1, "HTTP", "Unsupported data type for url parameter %s", one_http_param->Parameter->Name);
			goto error;
			}

		    n_output++;
		    }
		}
	    }

	/** Send command line **/
	if (strpbrk(path, " \t\r\n") || strpbrk(inf->Server, " \t\r\n:/") || strpbrk(inf->Port, " \t\r\n:/"))
	    {
	    mssError(1, "HTTP", "Invalid HTTP request");
	    return -1;
	    }
	rval = fdQPrintf(inf->Socket, "%STR %[http://%STR%]%[:%STR%]%STR%STR HTTP/1.0\r\nHost: %STR%[:%STR%]\r\n",
		inf->Method,
		inf->ProxyServer[0],
		inf->Server,
		inf->ProxyPort[0] && inf->ProxyServer[0],
		inf->Port,
		path,
		xsString(url_params),
		inf->Server,
		inf->Port[0],
		inf->Port);
	if (rval < 0)
	    return rval;

	/** POST parameters? **/
	if (!strcmp(inf->Method, "POST"))
	    {
	    http_internal_AddRequestHeader(inf, "Content-Type", "application/x-www-form-urlencoded", 0, 0);

	    post_params = xsNew();
	    if (!post_params)
		goto error;
	    for(n_output=i=0; i<xaCount(&inf->Params); i++)
		{
		one_http_param = xaGetItem(&inf->Params, i);
		if (one_http_param->Usage == HTTP_PUSAGE_T_POST)
		    {
		    if (!(one_http_param->Parameter->Value->Flags & DATA_TF_NULL))
			{
			/** Encode one parameter into the post data **/
			if (n_output > 0)
			    xsConcatenate(post_params, "&", 1);
			xsConcatQPrintf(post_params, "%STR&URL=", one_http_param->Parameter->Name);
			if (one_http_param->Parameter->Value->DataType == DATA_T_STRING)
			    xsConcatQPrintf(post_params, "%STR&URL", one_http_param->Parameter->Value->Data.String);
			else if (one_http_param->Parameter->Value->DataType == DATA_T_INTEGER)
			    xsConcatQPrintf(post_params, "%INT", one_http_param->Parameter->Value->Data.Integer);
			else
			    {
			    mssError(1, "HTTP", "Unsupported data type for post parameter %s", one_http_param->Parameter->Name);
			    goto error;
			    }

			n_output++;
			}
		    }
		}

	    snprintf(reqlen, sizeof(reqlen), "%d", xsLength(post_params));
	    http_internal_AddRequestHeader(inf, "Content-Length", nmSysStrdup(reqlen), 1, 0);
	    }

	printf("Web client sending request: %s - %s%s - %s\n", inf->Server, path, xsString(url_params), post_params?xsString(post_params):"");

	xsFree(url_params);
	url_params = NULL;

	/** Send Headers **/
	for(i=0; i<inf->RequestHeaders.nItems; i++)
	    {
	    hdr = (pHttpHeader)inf->RequestHeaders.Items[i];
	    if (hdr)
		{
		if (strpbrk(hdr->Name, " \t\r\n:") || strpbrk(hdr->Value, "\r\n"))
		    {
		    mssError(1, "HTTP", "Invalid HTTP header");
		    return -1;
		    }
		rval = fdQPrintf(inf->Socket, "%STR: %STR\r\n", hdr->Name, hdr->Value);
		if (rval < 0)
		    return rval;
		}
	    }

	/** End-of-headers **/
	fdQPrintf(inf->Socket, "\r\n");

	/** Post parameters? **/
	if (post_params)
	    {
	    fdWrite(inf->Socket, xsString(post_params), xsLength(post_params), 0, FD_U_PACKET);
	    xsFree(post_params);
	    }

	return 0;

    error:
	if (post_params)
	    xsFree(post_params);
	if (url_params)
	    xsFree(url_params);
	return -1;
    }


/*** http_internal_GetPageStream - make the connection to the remote server
 *** and issue the HTTP request.
 ***/
int
http_internal_GetPageStream(pHttpData inf)
    {
    pLxSession lex = NULL;
    int toktype;
#define BUF_SIZE 256
    char buf[BUF_SIZE];
    char *fullpath = NULL; // the path to be send to the server
    char *ptr;
    char *ptr2;
    int alloc = 0;
    char *p1;
    short status=0;
    pStructInf attr;
    pHttpHeader resp_hdr;
    int resp_cnt;
    char* hdr_val;
    int i;
    pParam one_param = NULL;
    pHttpParam one_http_param = NULL;
    time_t tval;
    struct tm* thetime;

	/** Reset ContentLength and Offset **/
	inf->ContentLength = inf->NetworkOffset = inf->ReadOffset = 0;

	/** Add or replace the Date: header **/
	tval = time(NULL);
	thetime = gmtime(&tval);
	strftime(buf, sizeof(buf), "%a, %d %b %Y %T GMT", thetime);
	http_internal_AddRequestHeader(inf, "Date", nmSysStrdup(buf), 1, 0);

	/** Re-evaluate hints on parameters, as needed **/
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = (pHttpParam)xaGetItem(&inf->Params, i);
	    one_param = one_http_param->Parameter;

	    /** Apply hints **/
	    if (paramEvalHints(one_param, inf->ObjList, inf->Obj->Session) < 0)
		goto error;
	    }

	/** decide what path we're going for.  Once we've been redirected, 
	    SubCnt is locked, and the server path is altered 
	    and will just follow redirects **/
	if(inf->Redirected || inf->Obj->SubCnt - inf->ParamSubCnt == 1)
	    ptr=NULL;
	else		    
	    ptr=obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr + inf->ParamSubCnt, inf->Obj->SubCnt - inf->ParamSubCnt - 1);
	if(ptr)
	    {
	    if(inf->Path[0])
		{
		fullpath=(char*)nmSysMalloc(strlen(inf->Path)+strlen(ptr)+3);
		sprintf(fullpath,"%s%s/%s", (inf->Path[0] == '/')?"":"/", inf->Path,ptr);
		}
	    else
		{
		fullpath=(char*)nmSysMalloc(strlen(ptr)+2);
		sprintf(fullpath,"/%s",ptr);
		}
	    }
	else
	    {
	    if(inf->Path[0])
		{
		fullpath=(char*)nmSysMalloc(strlen(inf->Path)+2);
		sprintf(fullpath,"%s%s", (inf->Path[0] == '/')?"":"/", inf->Path);
		}
	    else
		{
		fullpath=(char*)nmSysMalloc(2);
		sprintf(fullpath,"/");
		}
	    }

	if(HTTP_OS_DEBUG) printf("requesting: %s\n",fullpath);

	/** Establish the TCP connection **/
	if (!strcmp(inf->Protocol,"http"))
	    {
	    if (http_internal_ConnectHttp(inf) < 0)
		goto error;
	    }
	else if (!strcmp(inf->Protocol,"https") && HTTP_INF.SSL_ctx)
	    {
	    if (http_internal_ConnectHttps(inf) < 0)
		goto error;
	    }
	else
	    {
	    mssError(1,"HTTP","Protocol '%s' unsupported", inf->Protocol);
	    goto error;
	    }

	/** Set some headers **/
	snprintf(buf, sizeof(buf), "Centrallix/%s (objdrv_http)", cx__version);
	http_internal_AddRequestHeader(inf, "User-Agent", nmSysStrdup(buf), 1, 0);
	http_internal_AddRequestHeader(inf, "Accept", "*/*;q=1.0", 0, 0);
	http_internal_AddRequestHeader(inf, "Connection", "close", 0, 0);

	/** Send the request **/
	if (http_internal_SendRequest(inf, fullpath) < 0)
	    goto error;
	nmSysFree(fullpath);
	fullpath = NULL;

	/** Set up connection **/
#if 00
	    if(inf->Port[0])
		{
		snprintf(buf,256,"GET http://%s:%s%s HTTP/1.0\r\n",inf->Server,inf->Port,fullpath);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		sprintf(buf,"Host: %s:%s\r\n",inf->Server,inf->Port);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		}
	    else
		{
		sprintf(buf,"GET http://%s%s HTTP/1.0\r\n",inf->Server,fullpath);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		sprintf(buf,"Host: %s\r\n",inf->Server);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		}
	    if(inf->ProxyAuthLine[0])
		{
		fdWrite(inf->Socket,inf->ProxyAuthLine,strlen(inf->ProxyAuthLine),0,FD_U_PACKET);
		fdWrite(inf->Socket,"\r\n",2,0,FD_U_PACKET);
		}
	    if(inf->AuthLine[0])
		{
		fdWrite(inf->Socket,inf->AuthLine,strlen(inf->AuthLine),0,FD_U_PACKET);
		fdWrite(inf->Socket,"\r\n",2,0,FD_U_PACKET);
		}
	    //fdWrite(inf->Socket,"Connection: close\r\n",19,0,FD_U_PACKET);
	    fdWrite(inf->Socket,"\r\n",2,0,FD_U_PACKET);
	    }
	else
	    {
	    if(HTTP_OS_DEBUG) printf("connected\n");
	    sprintf(buf,"GET %s HTTP/1.0\r\n",fullpath);
	    if(HTTP_OS_DEBUG) printf("%s\n",buf);
	    fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
	    if(inf->Port[0])
		{
		sprintf(buf,"Host: %s:%s\r\n",inf->Server,inf->Port);
		if(HTTP_OS_DEBUG) printf("%s\n",buf);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		}
	    else
		{
		sprintf(buf,"Host: %s\r\n",inf->Server);
		if(HTTP_OS_DEBUG) printf("%s\n",buf);
		fdWrite(inf->Socket,buf,strlen(buf),0,FD_U_PACKET);
		}
	    if(inf->AuthLine[0])
		{
		fdWrite(inf->Socket,inf->AuthLine,strlen(inf->AuthLine),0,FD_U_PACKET);
		fdWrite(inf->Socket,"\r\n",2,0,FD_U_PACKET);
		}
	    //fdWrite(inf->Socket,"Connection: close\r\n",19,0,FD_U_PACKET);
	    fdWrite(inf->Socket,"\r\n",2,0,FD_U_PACKET);
	    }
#endif

	if(HTTP_OS_DEBUG) printf("Opening lexer session\n");
	lex=mlxOpenSession(inf->Socket,MLX_F_LINEONLY | MLX_F_NODISCARD);
	if(!lex)
	    {
	    mssError(0,"HTTP","Could not open lexer session");
	    goto error;
	    }

	/** Read in the response headers **/
	for(resp_cnt = 0; ; resp_cnt++)
	    {
	    if(HTTP_OS_DEBUG) printf("getting token....\n");
	    toktype=mlxNextToken(lex);
	    if(HTTP_OS_DEBUG) printf("got it!\n");
	    if(toktype==MLX_TOK_ERROR)
		{
		mssError(1,"HTTP","Lexer error");
		goto error;
		}

	    /** Get response line **/
	    alloc=0;
	    ptr = mlxStringVal(lex,&alloc);
	    if(HTTP_OS_DEBUG) printf("got token(%02x%02x%02x%02x): %s\n",ptr[0],ptr[1],ptr[2],ptr[3],ptr);

	    /** Remove trailing newlines **/
	    if (strchr(ptr,'\r')) *(strchr(ptr,'\r')) = '\0';
	    if (strchr(ptr,'\n')) *(strchr(ptr,'\n')) = '\0';

	    /** End of response headers? **/
	    if (!*ptr)
		{
		if (alloc) nmSysFree(ptr);
		break;
		}

	    /** First line? **/
	    if (resp_cnt == 0)
		{
		/** Look for HTTP -- first line of header if server is >=HTTP/1.0 **/
		if(regexec(&HTTP_INF.httpheader, ptr, 0, NULL, 0)==0)
		    {
		    if(HTTP_OS_DEBUG) printf("regex match on HTTP header\n");
		    p1=ptr;
		    while(p1[0]!=' ' && p1-ptr<strlen(ptr)) p1++;
		    if(p1-ptr==strlen(ptr)) 
			goto error;
		    p1++;
		    status=atoi(p1);
		   
		    if(status/100==2)
			{
			inf->Annotation=(char*)nmSysMalloc(strlen(inf->Server)+strlen(inf->Port)+strlen(inf->Path)+10);
			if(!inf->Annotation)
			    goto error;
			if(inf->Port[0])
			    sprintf(inf->Annotation,"http://%s:%s/%s",inf->Server,inf->Port,inf->Path);
			else
			    sprintf(inf->Annotation,"http://%s/%s",inf->Server,inf->Path);

			}	
		    if(status/100==4 || status/100==5 || (status/100==3 && !inf->AllowRedirects) )
			{
			if(inf->Redirected || inf->Obj->SubCnt<=1) // nothing more to try, fail
			    {
			    /** don't need to check AllowSubDirs here -- SubCnt==1 if !AllowSubDirs **/
			    mssError(1,"HTTP","Object not accessible on the server (HTTP code %d)", status);
			    goto error;
			    }
			else
			    {
			    inf->Obj->SubCnt--;
			    goto try_up;   // unsuccessful -- try up one more level
			    }
			}
		    }
		else
		    {
		    mssError(1,"HTTP","Could not parse response header line %d", resp_cnt);
		    goto error;
		    }

		continue;
		}

	    /** Find header value **/
	    ptr2 = strchr(ptr, ':');
	    if (ptr2)
		{
		/** Set ptr to the header item, ptr2 to the value **/
		ptr2[0]='\0';
		ptr2++;
		while(ptr2[0]==' ' || ptr2[0] == '\t') ptr2++;

		/** Squirrel away the response header **/
		resp_hdr = (pHttpHeader)nmMalloc(sizeof(HttpHeader));
		if (!resp_hdr)
		    goto error;
		strtcpy(resp_hdr->Name, ptr, sizeof(resp_hdr->Name));
		resp_hdr->Value = nmSysStrdup(ptr2);
		resp_hdr->ValueAlloc = 1;
		xaAddItem(&inf->ResponseHeaders, (void*)resp_hdr);
		}
	    if (alloc) nmSysFree(ptr);
	    }

	/** We're being redirected? **/
	if (status/100 == 3)
	    {
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Location")) != NULL)
		{   // changed to GNU regex matching -- much better
		regmatch_t pmatch[5];
		if (regexec(&HTTP_INF.parsehttp, hdr_val, 5, pmatch, 0) == REG_NOMATCH)
		    {
		    // the Location: line was unparsable
		    goto try_up; // try up one more level
		    }

#define COPY_FROM_PMATCH(p,m) \
		if (p) nmSysFree(p); \
		(p)=(char*)nmSysMalloc(pmatch[(m)].rm_eo-pmatch[(m)].rm_so+1);\
		if(!(p)) goto error;\
		memset((p),0,pmatch[(m)].rm_eo-pmatch[(m)].rm_so+1);\
		strncpy((p),ptr2+pmatch[(m)].rm_so,pmatch[(m)].rm_eo-pmatch[(m)].rm_so);

		/** Server **/
		COPY_FROM_PMATCH(inf->Server,1);
		
		/** Port **/
		if(pmatch[3].rm_so==-1) // port is optional
		    {
		    if (inf->Port) nmSysFree(inf->Port);
		    inf->Port=(char*)nmSysStrdup("");
		    if(!inf->Port)
			goto error;
		    }
		else
		    {
		    COPY_FROM_PMATCH(inf->Port,3);
		    }

		/** Path **/
		COPY_FROM_PMATCH(inf->Path,4);

		/** apparently a server sending a '302 found' doesn't necessarily mean
		 **   it actually has the file -- it would just rather you requested
		 **   it differently....
		 ** Try requesting /news/news.rdf/item from freebsd.org and see....
		 **/
		inf->Redirected=1;
		goto try_up; //unsuccessfull -- try again with updated params
		}

	    mssError(1, "HTTP", "Redirect response without Location header!");
	    goto error;
	    }

	/** Normal connection response **/
	if (status/100 == 2)
	    {
	    inf->ContentCacheTS = mtRealTicks() * 1000 / CxGlobals.ClkTck;
	    objCurrentDate(&inf->LastModified);
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Last-Modified")) != NULL)
		{
		http_internal_ParseDate(&(inf->LastModified),hdr_val);
		/*** objDataToDateTime looked like what I should use, but if I used it,
		 ***   centrallix would hang on access to this object
		 ***/
		    /*
		    objDataToDateTime(DATA_T_STRING,p2,&(inf->LastModified),
					"DDD, dd MMM yyyy HH:mm:ss GMT");
		    */
		}
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Content-Length")) != NULL)
		{
		inf->ContentLength=atoi(hdr_val);
		}
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Cache-Control")) != NULL)
		{
		if (strstr(hdr_val, "max-age=0"))
		    inf->ModDateAlwaysNow = 1;
		}
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Content-Type")) != NULL)
		{
		inf->ContentType = nmSysStrdup(hdr_val);
		if (strchr(inf->ContentType, ';'))
		    *(strchr(inf->ContentType, ';')) = '\0';
		}
	    else
		{
		inf->ContentType = nmSysStrdup("application/octet-stream");
		}
	    }

	mlxCloseSession(lex);
	if(HTTP_OS_DEBUG) printf("stream established\n");

	return 1;

    error:
	/** Error exit **/
	if (alloc) nmSysFree(ptr);
	if (lex) mlxCloseSession(lex);
	if (fullpath) nmSysFree(fullpath);
	return -1;

    try_up:
	/** Try up one level **/
	if (alloc) nmSysFree(ptr);
	if (lex) mlxCloseSession(lex);
	return 2;
    }


/*** http_internal_GetConfigString - get a configuration value
 ***/
char*
http_internal_GetConfigString(pHttpData inf, char* configname, char* default_value)
    {
    char* ptr = NULL;

	//if (stAttrValue(stLookup(inf->Node->Data,(configname)),NULL,&ptr,0) < 0)
	//if (stGetObjAttrValue(inf->Node->Data, configname, DATA_T_STRING, POD(&ptr)) < 0 || !ptr)
	if (stGetAttrValueOSML(stLookup(inf->Node->Data, configname), DATA_T_STRING, POD(&ptr), 0, inf->Obj->Session) < 0 || !ptr)
	    ptr = default_value;
	if (ptr)
	    ptr = nmSysStrdup(ptr);
	if (!ptr)
	    return NULL;
	if (HTTP_OS_DEBUG) printf("%s: %s\n",configname,ptr);

    return ptr;
    }


/*** http_internal_FreeParams - clean up any parameters
 ***/
int
http_internal_FreeParams(pHttpData inf)
    {
    int i;
    pHttpParam one_http_param;
    pParam one_param;

	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = (pHttpParam)xaGetItem(&inf->Params, i);
	    one_param = one_http_param->Parameter;
	    nmFree(one_http_param, sizeof(HttpParam));
	    paramFree(one_param);
	    }
	xaClear(&inf->Params, NULL, NULL);

    return 0;
    }


/*** http_internal_LoadParams - look through the list of "http/parameter"
 *** objects in the node data, and load them into the Params array.
 ***/
int
http_internal_LoadParams(pHttpData inf)
    {
    int i, j;
    pStructInf param_inf, attr_inf;
    pParam one_param = NULL, one_new_param = NULL;
    pHttpParam one_http_param = NULL, one_new_http_param = NULL;
    pStruct one_open_ctl, open_ctl;
    char* val;
    TObjData tod;
    char* endval;

	/** Get parameter list **/
	for(i=0; i<inf->Node->Data->nSubInf; i++)
	    {
	    param_inf = inf->Node->Data->SubInf[i];
	    if (stStructType(param_inf) == ST_T_SUBGROUP && !strcmp(param_inf->UsrType,"http/parameter"))
		{
		/** Found a parameter.  Now set it up **/
		one_new_param = paramCreateFromInf(param_inf);
		if (!one_new_param) goto error;
		one_new_http_param = (pHttpParam)nmMalloc(sizeof(HttpParam));
		if (!one_new_http_param) goto error;
		one_new_http_param->Parameter = one_new_param;
		one_new_http_param->Usage = HTTP_PUSAGE_T_URL;
		one_new_http_param->Source = HTTP_PSOURCE_T_PARAM;
		one_new_http_param->PathPart = 0;

		/** Parameter Usage HTTP_PUSAGE_T_xxx **/
		val = NULL;
		if ((attr_inf = stLookup(param_inf, "usage")))
		    {
		    if (stAttrValue(attr_inf, NULL, &val, 0) == 0 && val)
			{
			if (!strcmp(val, "url"))
			    one_new_http_param->Usage = HTTP_PUSAGE_T_URL;
			else if (!strcmp(val, "post"))
			    one_new_http_param->Usage = HTTP_PUSAGE_T_POST;
			else if (!strcmp(val, "header"))
			    one_new_http_param->Usage = HTTP_PUSAGE_T_HEADER;
			else
			    {
			    mssError(1, "HTTP", "Parameter %s usage must be 'url', 'post', or 'header'", param_inf->Name);
			    goto error;
			    }
			}
		    else
			{
			mssError(1, "HTTP", "Parameter %s usage must be 'url', 'post', or 'header'", param_inf->Name);
			goto error;
			}
		    }

		/** Parameter Source HTTP_PSOURCE_T_xxx **/
		val = NULL;
		if ((attr_inf = stLookup(param_inf, "source")))
		    {
		    if (stAttrValue(attr_inf, NULL, &val, 0) == 0 && val)
			{
			if (!strcmp(val, "path"))
			    one_new_http_param->Source = HTTP_PSOURCE_T_PATH;
			else if (!strcmp(val, "param"))
			    one_new_http_param->Source = HTTP_PSOURCE_T_PARAM;
			else if (!strcmp(val, "none"))
			    one_new_http_param->Source = HTTP_PSOURCE_T_NONE;
			else
			    {
			    mssError(1, "HTTP", "Parameter %s source must be 'path', 'param', or 'none'", param_inf->Name);
			    goto error;
			    }
			}
		    else
			{
			mssError(1, "HTTP", "Parameter %s source must be 'path', 'param', or 'none'", param_inf->Name);
			goto error;
			}
		    }

		/** Parameter path part for source=path **/
		if ((attr_inf = stLookup(param_inf, "pathpart")))
		    {
		    if (one_new_http_param->Source != HTTP_PSOURCE_T_PATH)
			{
			mssError(1, "HTTP", "Parameter %s has 'pathpart' specified but is not source=\"path\"", param_inf->Name);
			goto error;
			}
		    if (stAttrValue(attr_inf, &one_new_http_param->PathPart, NULL, 0) < 0)
			{
			mssError(1, "HTTP", "Parameter %s path part must be an integer 1 or greater", param_inf->Name);
			goto error;
			}
		    }

		/** Add to our list **/
		xaAddItem(&inf->Params, one_new_http_param);
		one_new_param = NULL;
		one_new_http_param = NULL;
		}
	    }

	/** Go through the parameters and set any path params **/
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = (pHttpParam)xaGetItem(&inf->Params, i);
	    one_param = one_http_param->Parameter;

	    if (one_http_param->Source == HTTP_PSOURCE_T_PATH)
		{
		/** Did the user supply the path item? **/
		if (one_http_param->PathPart + inf->Obj->SubPtr <= inf->Obj->Pathname->nElements)
		    {
		    /** Keep track of how many elements we're using for parameters **/
		    if (inf->ParamSubCnt < one_http_param->PathPart)
			{
			inf->ParamSubCnt = one_http_param->PathPart;
			}

		    /** Get the path element **/
		    val = obj_internal_PathPart(inf->Obj->Pathname, inf->Obj->SubPtr + one_http_param->PathPart - 1, 1);
		    if (!val)
			goto error;
		    if (one_param->Value->DataType == DATA_T_INTEGER)
			{
			tod.DataType = DATA_T_INTEGER;
			tod.Flags = 0;
			tod.Data.Integer = strtol(val, &endval, 10);
			if (!val[0] || endval[0])
			    {
			    mssError(1, "HTTP", "Invalid integer parameter %s from path", one_param->Name);
			    goto error;
			    }
			}
		    else if (one_param->Value->DataType == DATA_T_STRING)
			{
			tod.DataType = DATA_T_STRING;
			tod.Flags = 0;
			tod.Data.String = val;
			}
		    else
			{
			mssError(1, "HTTP", "Parameter %s must be string or integer", one_param->Name);
			goto error;
			}
		    paramSetValue(one_param, &tod);
		    }
		}
	    }

	/** Go through the parameters and set any url params **/
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = (pHttpParam)xaGetItem(&inf->Params, i);
	    one_param = one_http_param->Parameter;

	    /** Set the value supplied by the user, if needed. **/
	    if (one_http_param->Source == HTTP_PSOURCE_T_PARAM)
		{
		open_ctl = inf->Obj->Pathname->OpenCtl[inf->Obj->SubPtr + inf->ParamSubCnt - 1];
		if (open_ctl)
		    {
		    for(j=0; j<open_ctl->nSubInf; j++)
			{
			one_open_ctl = open_ctl->SubInf[j];
			if (!strcmp(one_open_ctl->Name, one_param->Name))
			    {
			    paramSetValueFromInfNe(one_param, one_open_ctl);
			    break;
			    }
			}
		    }
		}
	    }

	/** Apply presentation hints **/
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = (pHttpParam)xaGetItem(&inf->Params, i);
	    one_param = one_http_param->Parameter;
	    if (paramEvalHints(one_param, inf->ObjList, inf->Obj->Session) < 0)
		goto error;
	    }

	return 0;

    error:
	if (one_new_http_param)
	    nmFree(one_new_http_param, sizeof(HttpParam));
	if (one_new_param)
	    paramFree(one_new_param);
	http_internal_FreeParams(inf);
	return -1;
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
    char* user;
    char* pw;
    char* authline;
    int len;
    int redir_cnt;

	/** Allocate the structure **/
	inf = (pHttpData)nmMalloc(sizeof(HttpData));
	if (!inf) 
	    goto error;
	memset(inf,0,sizeof(HttpData));
	inf->Obj = obj;
	inf->Mask = mask;
	objCurrentDate(&inf->LastModified);
	xaInit(&inf->RequestHeaders, 16);
	xaInit(&inf->ResponseHeaders, 16);
	xaInit(&inf->Params, 16);
	http_internal_AddRequestHeader(inf, "X-Nonce", "", 0, 0);
	inf->ContentCache = NULL;
	inf->Flags = 0;
	inf->NetworkOffset = 0;
	inf->ReadOffset = 0;

	//printf("objdrv_http.c was offered: (%i,%i,%i) %s\n",obj->SubPtr,
	//	obj->SubCnt,obj->Pathname->nElements,obj_internal_PathPart(obj->Pathname,0,0));
	obj->SubCnt = obj->Pathname->nElements - obj->SubPtr + 1; // Grab everything...

	/** Otherwise, try to open it first. **/
	if (!node)
	    {
	    node = snReadNode(obj->Prev);
	    }

	/** If _still_ no node, quit out. **/
	if (!node)
	    {
	    mssError(0,"HTTP","Could not open structure file");
	    goto error;
	    }

	/** Set object params. **/
	inf->Node = node;
	strcpy(inf->Pathname, obj_internal_PathPart(obj->Pathname,0,0));
	inf->Node->OpenCnt++;

	/** Create the expression eval param list **/
	inf->ObjList = expCreateParamList();
	inf->ObjList->Session = obj->Session;
	expAddParamToList(inf->ObjList, "headers", (void*)inf, EXPR_O_CURRENT);
	expSetParamFunctions(inf->ObjList, "headers", http_internal_GetHdrType, http_internal_GetHdrValue, http_internal_SetHdrValue);
	expAddParamToList(inf->ObjList, "parameters", (void*)inf, 0);
	expSetParamFunctions(inf->ObjList, "parameters", http_internal_GetParamType, http_internal_GetParamValue, http_internal_SetParamValue);

	/** Configuration **/
	inf->ProxyServer = http_internal_GetConfigString(inf, "proxyserver", "");
	inf->ProxyPort = http_internal_GetConfigString(inf, "proxyport", "80");
	inf->Server = http_internal_GetConfigString(inf, "server", "");
	inf->Port = http_internal_GetConfigString(inf, "port", "");
	inf->Path = http_internal_GetConfigString(inf, "path", "/");
	inf->Method = http_internal_GetConfigString(inf, "method", "GET");
	inf->Protocol = http_internal_GetConfigString(inf, "protocol", "http");
	inf->Cipherlist = http_internal_GetConfigString(inf, "ssl_cipherlist", "");

	/** Valid method? **/
	if (strcmp(inf->Method, "GET") && strcmp(inf->Method, "POST"))
	    {
	    mssError(1, "HTTP", "Invalid method '%s'", inf->Method);
	    goto error;
	    }

	/** Authentication headers **/
	if ((ptr = http_internal_GetConfigString(inf, "proxyauthline", NULL)))
	    {
	    http_internal_AddRequestHeader(inf, "Proxy-Authorization", ptr, 1, 0);
	    }
	if ((ptr = http_internal_GetConfigString(inf, "authline", NULL)))
	    {
	    http_internal_AddRequestHeader(inf, "Authorization", ptr, 1, 0);
	    }
	else if ((user = http_internal_GetConfigString(inf, "username", NULL)))
	    {
	    pw = http_internal_GetConfigString(inf, "password", "");
	    ptr = nmSysMalloc(strlen(user) + 1 + strlen(pw) + 1);
	    if (!ptr)
		goto error;
	    sprintf(ptr, "%s:%s", user, pw);
	    len = (strlen(user) + 1 + strlen(pw)) * 2 + 3 + 6;
	    authline = nmSysMalloc(len);
	    if (!authline)
		{
		nmSysFree(ptr);
		goto error;
		}
	    qpfPrintf(NULL, authline, len, "Basic %STR&B64", ptr);
	    nmSysFree(ptr);
	    http_internal_AddRequestHeader(inf, "Authorization", authline, 1, 0);
	    }

	/** Control flags **/
	if (stAttrValue(stLookup(node->Data, "allowbadcert"), &inf->AllowBadCert, NULL, 0) < 0)
	    inf->AllowBadCert=0;
	if (stAttrValue(stLookup(node->Data, "allowsubdirs"), &inf->AllowSubDirs, NULL, 0) < 0)
	    inf->AllowSubDirs=1;
	if (stAttrValue(stLookup(node->Data, "allowredirects"), &inf->AllowRedirects, NULL, 0) < 0)
	    inf->AllowRedirects=1;
	if (!inf->AllowSubDirs)
	    inf->Obj->SubCnt=1;

	/** Cache control **/
	if (stAttrValue(stLookup(node->Data, "cache_min_ttl"), &inf->CacheMinTime, NULL, 0) < 0)
	    inf->CacheMinTime = 0;
	if (inf->CacheMinTime < 0)
	    inf->CacheMinTime = 0;
	if (stAttrValue(stLookup(node->Data, "cache_max_ttl"), &inf->CacheMaxTime, NULL, 0) < 0)
	    inf->CacheMaxTime = 0;
	if (inf->CacheMaxTime < 0)
	    inf->CacheMaxTime = 0;
	if (stAttrValue(stLookup(node->Data, "cache_max_length"), &inf->ContentCacheMaxLen, NULL, 0) < 0)
	    inf->ContentCacheMaxLen = HTTP_DEFAULT_MEM_CACHE;
	if (inf->ContentCacheMaxLen < 0)
	    inf->ContentCacheMaxLen = 0;

	/** Ensure all required config items are given **/
	if (!inf->Server[0])
	    {
	    mssError(1, "HTTP", "Server name must be provided");
	    goto error;
	    }

	/** Load parameters **/
	if (http_internal_LoadParams(inf) < 0)
	    goto error;

	/** Adjust SubCnt based on parameter usage **/
	if (!inf->AllowSubDirs)
	    inf->Obj->SubCnt = 1 + inf->ParamSubCnt;

	/** Connect to server and retrieve the response headers **/
	redir_cnt = 0;
	while((rval = http_internal_GetPageStream(inf)) == 2)
	    {
	    if(HTTP_OS_DEBUG) printf("trying again...(redirected: %i)\n",inf->Redirected);
	    if (inf->Redirected) redir_cnt++;
	    if (redir_cnt > HTTP_REDIR_MAX)
		{
		mssError(1,"HTTP","HTTP server redirection loop");
		goto error;
		}
	    }
	if(rval != 1)
	    goto error;

	return (void*)inf;

    error:
	if (inf) http_internal_Cleanup(inf);
	return NULL;
    }


/*** httpClose - close an open object.
 ***/
int
httpClose(void* inf_v, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);

	/** Release the memory **/
	inf->Node->OpenCnt --;

	/** Release all memory allocations **/
	http_internal_Cleanup(inf);

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
    int readcnt=maxcnt; /* if we get no other info, use the size requested */
    int redir_cnt;
    int maxread;
    int rval;
    int cur_msec = mtRealTicks() * 1000 / CxGlobals.ClkTck;
    int len;

	if (flags & OBJ_U_SEEK)
	    inf->ReadOffset = offset;

	if (maxcnt <= 0)
	    return 0;

	/** Cached and data available? **/
	if (!inf->ContentCache)
	    inf->ContentCache = xsNew();
	if (!inf->ContentCache)
	    return -1;
	len = xsLength(inf->ContentCache);
	if ((inf->Flags & HTTP_F_CONTENTCOMPLETE || inf->ReadOffset < len) && cur_msec < inf->ContentCacheTS + inf->CacheMinTime)
	    {
	    if (inf->ReadOffset <= len)
		{
		if (maxcnt > len - inf->ReadOffset)
		    maxcnt = len - inf->ReadOffset;
		if (maxcnt)
		    memcpy(buffer, xsString(inf->ContentCache) + inf->ReadOffset, maxcnt);
		inf->ReadOffset += maxcnt;
		return maxcnt;
		}
	    else
		return 0;
	    }

	/** Restart connection? **/
	if(!inf->Socket || inf->ReadOffset < inf->NetworkOffset)
	    {
	    /** if there's no connection or we're told to seek to 0, reinit the connection **/
	    int rval;
	    redir_cnt = 0;
	    while((rval=http_internal_GetPageStream(inf))==2)
		{
		if(HTTP_OS_DEBUG) printf("trying again...(redirected: %i)\n",inf->Redirected);
		if (inf->Redirected) redir_cnt++;
		if (redir_cnt > HTTP_REDIR_MAX)
		    {
		    mssError(1,"HTTP","HTTP server redirection loop");
		    return -1;
		    }
		}
	    if(rval==0)
		return -1;
	    inf->NetworkOffset = 0;
	    inf->Flags &= ~HTTP_F_CONTENTCOMPLETE;
	    }
	if(inf->ContentLength)
	    {
	    /** if the server provided a Content-Length header, use it... **/
	    readcnt = inf->ContentLength - inf->NetworkOffset; /* the maximum length we're allowed to request */
	    readcnt = readcnt>maxcnt?maxcnt:readcnt; /* drop down to what the requesting object wants */
	    }
	if(!inf->Socket) return -1;
	if(HTTP_OS_DEBUG) printf("HTTP -- starting fdRead -- asking for: %i bytes\n", readcnt);

	/** Skip to offset - the hard way (optimize - honor HTTP ranges) **/
	while (inf->ReadOffset > inf->NetworkOffset)
	    {
	    maxread = maxcnt;
	    if (maxread > (inf->ReadOffset - inf->NetworkOffset))
		maxread = inf->ReadOffset - inf->NetworkOffset;
	    rval = fdRead(inf->Socket, buffer, maxread, inf->NetworkOffset, flags & ~FD_U_SEEK);
	    if (rval <= 0)  
		{
		if (rval == 0 && maxread > 0 && inf->NetworkOffset <= inf->ContentCacheMaxLen)
		    inf->Flags |= HTTP_F_CONTENTCOMPLETE;
		if (rval < 0 || maxread > 0)
		    http_internal_CloseConnection(inf);
		return rval;
		}
	    if (inf->NetworkOffset + rval <= inf->ContentCacheMaxLen)
		xsWrite(inf->ContentCache, buffer, rval, inf->NetworkOffset, XS_U_SEEK);
	    inf->NetworkOffset += rval;
	    }

	/** Do the "real read" **/
	rval = fdRead(inf->Socket, buffer, readcnt, offset, flags & ~FD_U_SEEK);
	if(HTTP_OS_DEBUG) printf("HTTP -- done with fdRead -- got: %i bytes\n", rval);

	/** update inf->Offset with the new distance we are into the stream **/
	if (rval > 0)
	    {
	    if (inf->NetworkOffset + rval <= inf->ContentCacheMaxLen)
		xsWrite(inf->ContentCache, buffer, rval, inf->NetworkOffset, XS_U_SEEK);
	    inf->NetworkOffset += rval;
	    inf->ReadOffset += rval;
	    }
	else
	    {
	    if (rval == 0 && readcnt > 0 && inf->NetworkOffset <= inf->ContentCacheMaxLen)
		inf->Flags |= HTTP_F_CONTENTCOMPLETE;

	    /** end of file - close up **/
	    if (rval < 0 || readcnt > 0)
		http_internal_CloseConnection(inf);
	    }

    return rval;
    }


/*** httpWrite - Can't write to a file over HTTP
 ***/
int
httpWrite(void* inf_v, char* buffer, int cnt, int offset, int flags, pObjTrxTree* oxt)
    {
    return -1;
    }


/*** httpOpenQuery - Can't get directory information over HTTP
 ***/
void*
httpOpenQuery(void* inf_v, pObjQuery query, pObjTrxTree* oxt)
    {
    return NULL;
    }


/*** httpQueryFetch - no entries
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
	if (!strcmp(attrname,"Content-Length") || !strcmp(attrname,"size")) return DATA_T_INTEGER;

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

    return DATA_T_STRING;
    }


/*** httpGetAttrValue - get the value of an attribute by name.  The 'val'
 *** pointer must point to an appropriate data type.
 ***/
int
httpGetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree* oxt)
    {
    pHttpData inf = HTTP(inf_v);
    pStructInf find_inf;
    char* ptr;
    int i;
    int cur_msec = mtRealTicks() * 1000 / CxGlobals.ClkTck;

	/** Choose the attr name **/
	if (!strcmp(attrname,"name"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->String = inf->Obj->Pathname->Elements[inf->Obj->Pathname->nElements-1];
	    return 0;
	    }

	if (!strcmp(attrname,"outer_type"))
	    {	// shouldn't we return something like application/http, not the inner type?
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    /*if (httpGetAttrValue(inf_v,"Content-Type", datatype, val, oxt)==0)
		{
		}
	    else
		{*/
		val->String = "application/http";
		/*}*/
	    return 0;
	    }

	/** If content-type, return as appropriate **/
	if (!strcmp(attrname,"content_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->String = inf->ContentType;
	    return 0;
	    }

	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    if(inf->Annotation)
		{
		val->String=inf->Annotation;
		return 0;
		}
	    else
		return -1;
	    }

	if(!strcmp(attrname,"last_modification"))
	    {
	    if (datatype != DATA_T_DATETIME)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=datetime]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    if(inf->LastModified.Value)
		{
		if (inf->ModDateAlwaysNow && cur_msec > inf->ContentCacheTS + inf->CacheMinTime)
		    objCurrentDate(&inf->LastModified);
		val->DateTime=&(inf->LastModified);
		return 0;
		}
	    else
		return 1; /* NULL */
	    }
	if(!strcmp(attrname,"Content-Length") || !strcmp(attrname, "size"))
	    {
	    if (datatype != DATA_T_INTEGER)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=integer]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->Integer=inf->ContentLength;
	    return 0;
	    }

	if ((ptr = http_internal_GetHeader(&inf->ResponseHeaders, attrname)) != NULL)
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->String = ptr;
	    return 0;
	    }

	if(!strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->String = inf->ContentType;
	    return 0;
	    }

	/** If annotation, and not found, return "" **/
	if (!strcmp(attrname,"annotation"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}
	    val->String = "";
	    return 0;
	    }

    return 1; /* null */
    }


/*** httpGetNextAttr - get the next attribute name for this object.
 ***/
char*
httpGetNextAttr(void* inf_v, pObjTrxTree oxt)
    {
    pHttpData inf = HTTP(inf_v);
    pHttpHeader hdr;

    if(inf->NextAttr<0)
	{
	switch(inf->NextAttr++)
	    {
#if 0
	    case -7: return "name";
	    case -6: return "content_type";
	    case -5: return "annotation";
	    case -4: return "inner_type";
	    case -3: return "outer_type";
	    case -2:
		     if(inf->LastModified.Value)
			 return "last_modification";
		     inf->NextAttr++;
#endif
	    case -1:
		     return "Content-Length";

	    }
	}

    if(inf->NextAttr<inf->ResponseHeaders.nItems)
	{ 
	/* skip Content-Length as an attribute -- it was handled above */
	hdr = (pHttpHeader)inf->ResponseHeaders.Items[inf->NextAttr++];
	if (!strcasecmp(hdr->Name,"Content-Length"))
	    {
	    if(inf->NextAttr >= inf->ResponseHeaders.nItems)
		return NULL;
	    hdr = (pHttpHeader)inf->ResponseHeaders.Items[inf->NextAttr++];
	    }
	return hdr->Name;
	}

    return NULL;
    }


/*** httpGetFirstAttr - get the first attribute name for this object.
 ***/
char*
httpGetFirstAttr(void* inf_v, pObjTrxTree oxt)
    {
    pHttpData inf = HTTP(inf_v);
    char* ptr;

	/** Set the first attribute to be returned. **/
	inf->NextAttr = -1;

	/** Return the next one. **/
	ptr = httpGetNextAttr(inf_v, oxt);

    return ptr;
    }


/*** httpSetAttrValue - sets the value of an attribute.  'val' must
 *** point to an appropriate data type.
 ***/
int
httpSetAttrValue(void* inf_v, char* attrname, int datatype, pObjData val, pObjTrxTree oxt)
    {
    return -1;
    }

/*** httpAddAttr - add an attribute to an object.  This doesn't always work
 *** for all object types, and certainly makes no sense for some (like unix
 *** files).
 ***/
int
httpAddAttr(void* inf_v, char* attrname, int type, pObjData val, pObjTrxTree oxt)
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
httpExecuteMethod(void* inf_v, char* methodname, pObjData param, pObjTrxTree oxt)
    {
    return -1;
    }


/*** httpInfo - Return the capabilities of the object
 ***/
int
httpInfo(void* inf_v, pObjectInfo info)
    {
    pHttpData inf = HTTP(inf_v);

	info->Flags |= ( OBJ_INFO_F_CANT_ADD_ATTR | OBJ_INFO_F_CAN_SEEK_REWIND | OBJ_INFO_F_CAN_HAVE_CONTENT | 
	    OBJ_INFO_F_HAS_CONTENT );
	if (inf->AllowSubDirs) info->Flags |= OBJ_INFO_F_CAN_HAVE_SUBOBJ;
	else info->Flags |= OBJ_INFO_F_CANT_HAVE_SUBOBJ;

    return 0;
    }


/*** httpInitialize - initialize this driver, which also causes it to 
 *** register itself with the objectsystem.
 ***/
int
httpInitialize()
    {
    pObjDriver drv;
    int retval;
    char temp[256];
    char* ptr;

	/** Allocate the driver **/
	drv = (pObjDriver)nmMalloc(sizeof(ObjDriver));
	if (!drv) return -1;
	memset(drv, 0, sizeof(ObjDriver));

	/** Initialize globals **/
	memset(&HTTP_INF,0,sizeof(HTTP_INF));
	retval=regcomp(&HTTP_INF.parsehttp,"http://([^:/]+)(:(.+))?/(.*)",REG_EXTENDED | REG_ICASE);
	if(retval)
	    {
	    regerror(retval,&HTTP_INF.parsehttp,temp,sizeof(temp));
	    mssError(0,"HTTP","Error while building regex: %s",temp);
	    }
	retval=regcomp(&HTTP_INF.httpheader,"^HTTP",REG_EXTENDED | REG_ICASE | REG_NOSUB);
	if(retval)
	    {
	    regerror(retval,&HTTP_INF.httpheader,temp,sizeof(temp));
	    mssError(0,"HTTP","Error while building regex: %s",temp);
	    }

	/** Set up header nonce **/
	HTTP_INF.NonceData = cxssKeystreamNew(NULL, 0);
	if (!HTTP_INF.NonceData)
	    mssError(1, "HTTP", "Warning: X-Nonce headers will not be emitted");

	/** Set up OpenSSL **/
	HTTP_INF.SSL_ctx = SSL_CTX_new(SSLv23_client_method());
	if (HTTP_INF.SSL_ctx) 
	    {
	    SSL_CTX_set_options(HTTP_INF.SSL_ctx, SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE | SSL_OP_ALL);
	    if (stAttrValue(stLookup(CxGlobals.ParsedConfig,"ssl_cipherlist"),NULL,&ptr,0) < 0) ptr="DEFAULT";
	    SSL_CTX_set_cipher_list(HTTP_INF.SSL_ctx, ptr);
	    SSL_CTX_set_verify(HTTP_INF.SSL_ctx, SSL_VERIFY_PEER, NULL);
	    SSL_CTX_set_verify_depth(HTTP_INF.SSL_ctx, 10);
	    SSL_CTX_set_default_verify_paths(HTTP_INF.SSL_ctx);
	    }

	/** Setup the structure **/
	strcpy(drv->Name,"HTTP - HTTP/HTTPS Protocol for objectsystem");
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
	drv->Info = httpInfo;

	nmRegister(sizeof(HttpData),"HttpData");
	nmRegister(sizeof(HttpQuery),"HttpQuery");

	/** Register the driver **/
	if (objRegisterDriver(drv) < 0) return -1;

    return 0;
    }


MODULE_INIT(httpInitialize);
MODULE_PREFIX("http");
MODULE_DESC("HTTP/HTTPS ObjectSystem Driver");
MODULE_VERSION(0,1,0);
MODULE_IFACE(CX_CURRENT_IFACE);

