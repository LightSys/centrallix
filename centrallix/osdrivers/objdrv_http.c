#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
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
#include "cxlib/xhash.h"
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
#define HTTP_PUSAGE_T_NONE	4	/* parameter is not sent, but is available for use via :parameters:name */

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
    int		AllowCookies;
    int		AllowInsecureRedirects;
    int		ForceSecureRedirects;
    int		SuccessCodes[8];
    int		NextAttr;
    pFile	Socket;
    pFile	SSLReporting;
    int		SSLpid;
    SSL_CTX*	SSL_ctx;
    char	Redirected;
    int		Status;
    DateTime	LastModified;
    char	*Annotation;
    int		ContentLength;
    int		NetworkOffset;
    int		ReadOffset;
    char*	ContentType;		/* actual content type provided by remote http server */
    char*	RequestContentType;	/* content type of request sent to the server */
    char*	RestrictContentType;	/* restriction on content type allowed in the response */
    char *	ContentCharset;		/* the charset provided by the remote http server */
    char *	ExpectedContentCharset; /* provide the ability to force an expected charset */
    int		OverrideContentCharset;	/* when true, the actual charset withh be overwritten with the specifiied*/
    int		RetryCount;		/* actual number of retries */
    int		RetryLimit;		/* configured retry limit */
    double	RetryDelay;
    double	RetryBackoffRatio;
    int		RetryFor;		/* HTTP_RETRY_F_xxx */
    int		RedirectLimit;
    int		SentAuth;		/* whether authorization was sent on this request */
    int		SendAuth;		/* whether to send Authorization on initial request */
    int		HadAuth;		/* whether an Authorization header was available. */
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

#define HTTP_RETRY_F_TCPRESET	1
#define	HTTP_RETRY_F_RUDECLOSE	2
#define HTTP_RETRY_F_HTTP401	4
#define HTTP_RETRY_F_HTTP4XX	8
#define HTTP_RETRY_F_HTTP5XX	16
#define HTTP_RETRY_F_TLSERROR	32

#define HTTP(x) ((pHttpData)(x))
#define HTTP_OS_DEBUG		0
#define HTTP_REDIR_MAX		4	/* default limit on redirects, can be overridden in node */

/*** Status return values from GetPageStream() ***/
#define HTTP_CONN_S_FAIL	(-1)
#define HTTP_CONN_S_SUCCESS	0
#define HTTP_CONN_S_HTTP401	1
#define HTTP_CONN_S_HTTP404	2
#define HTTP_CONN_S_HTTP4XX	3
#define HTTP_CONN_S_HTTP5XX	4
#define HTTP_CONN_S_TCPRESET	5
#define HTTP_CONN_S_RUDECLOSE	6
#define HTTP_CONN_S_TLSERROR	7
#define HTTP_CONN_S_REDIRECT	8
#define HTTP_CONN_S_TRYUP	9

const char* http_conn_s_desc[] = { "success", "http401", "http404", "http4xx", "http5xx", "tcpreset", "rudeclose", "tlserror", "redirect", "tryup" };


/*** Structure used by queries for this driver. ***/
typedef struct
    {
    pHttpData	Data;
    char	NameBuf[OBJSYS_MAX_PATH];
    int		ItemCnt;
    }
    HttpQuery, *pHttpQuery;


/*** Structure for one cookie ***/
typedef struct
    {
    char*	Name;
    char*	Value;
    }
    HttpCookie, *pHttpCookie;


/*** Structure used to store data about a server. ***/
typedef struct
    {
    char	URI[256];	/* name of user and website, form http(s)://username@host:port */
    XArray	Cookies;	/* of pHttpCookie */
    }
    HttpServerData, *pHttpServerData;


/*** GLOBALS ***/
struct
    {
    regex_t	parsehttp;
    regex_t	httpheader;
    pCxssKeystreamState NonceData;
    XHashTable	ServerData;
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
	    if (inf->RequestContentType) nmSysFree(inf->RequestContentType);
	    inf->RequestContentType = NULL;
	    if (inf->RestrictContentType) nmSysFree(inf->RestrictContentType);
	    inf->RestrictContentType = NULL;
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
	    if (inf->ContentCharset) nmSysFree(inf->ContentCharset);
	    inf->ContentCharset = NULL;
	    if (inf->ExpectedContentCharset) nmSysFree(inf->ExpectedContentCharset);
	    inf->ExpectedContentCharset = NULL;

	    /** Close socket, if needed **/
	    http_internal_CloseConnection(inf);

	    if (inf->ObjList)
		expFreeParamList(inf->ObjList);

	    if (inf->ContentCache)
		xsFree(inf->ContentCache);

	    if (inf->Annotation)
		nmSysFree(inf->Annotation);

	    /** free SSL context, if needed**/
	    if(inf->SSL_ctx)
		SSL_CTX_free(inf->SSL_ctx);
	    inf->SSL_ctx = NULL;

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
	if (inf->AllowBadCert)
	    SSL_CTX_set_verify(inf->SSL_ctx, SSL_VERIFY_NONE, NULL);
	else
	    SSL_CTX_set_verify(inf->SSL_ctx, SSL_VERIFY_PEER, NULL);
	if (*inf->Cipherlist)
	    SSL_CTX_set_cipher_list(inf->SSL_ctx, inf->Cipherlist);
	inf->SSLpid = cxssStartTLS(inf->SSL_ctx, &inf->Socket, &inf->SSLReporting, 0, inf->Server);
	if (inf->SSLpid <= 0)
	    {
	    mssErrorErrno(0, "HTTP", "Could not start SSL/TLS session");
	    netCloseTCP(inf->Socket,1,0);
	    inf->Socket = NULL;
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


/*** http_internal_StrCmp - string compare for qsort()
 ***/
int
http_internal_StrCmp(const void *v1, const void *v2)
    {
    pHttpParam *p1 = (pHttpParam*)v1;
    pHttpParam *p2 = (pHttpParam*)v2;
    return strcmp((*p1)->Parameter->Name, (*p2)->Parameter->Name);
    }


/*** http_i_PostBodyUrlencode - create a POST body with form urlencoding
 *** formatting.
 ***/
pXString
http_i_PostBodyUrlencode(pHttpData inf)
    {
    pXString post_params = NULL;
    pHttpParam one_http_param;
    int n_output, i;

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

	return post_params;

    error:
	if (post_params)
	    xsFree(post_params);
	return NULL;
    }


/*** http_i_PostBodyXML - create a POST body with XML formatting.
 ***/
pXString
http_i_PostBodyXML(pHttpData inf)
    {
    pXString post_params = NULL;
    pHttpParam one_http_param;
    int i,j;
    char* cur_xml[16];
    char* last_xml[16];
    char* param_part;
    int param_part_cnt;
    char* save_ptr;
    char cur_param[256];

	post_params = xsNew();
	if (!post_params)
	    goto error;

	/** Need to sort them to get xml data organized right **/
	qsort(inf->Params.Items, inf->Params.nItems, sizeof(char*), http_internal_StrCmp);

	/** Encode them **/
	for(j=0; j<sizeof(cur_xml) / sizeof(char*); j++)
	    {
	    cur_xml[j] = NULL;
	    last_xml[j] = NULL;
	    }
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = xaGetItem(&inf->Params, i);
	    if (one_http_param->Usage == HTTP_PUSAGE_T_POST)
		{
		if (!(one_http_param->Parameter->Value->Flags & DATA_TF_NULL))
		    {
		    strtcpy(cur_param, one_http_param->Parameter->Name, sizeof(cur_param));

		    /** Go through the . separated parts of the param name **/
		    param_part = strtok_r(cur_param, ".", &save_ptr);
		    param_part_cnt = 0;
		    while(param_part)
			{
			cur_xml[param_part_cnt] = param_part;
			param_part = strtok_r(NULL, ".", &save_ptr);
			param_part_cnt++;
			if (param_part_cnt >= sizeof(cur_xml) / sizeof(char*) - 1)
			    {
			    mssError(1, "HTTP", "Nesting too great for XML parameter %s", one_http_param->Parameter->Name);
			    goto error;
			    }
			}
		    for(j=param_part_cnt; j < sizeof(cur_xml) / sizeof(char*); j++)
			{
			cur_xml[j] = NULL;
			}

		    /** Emit closing tags (reverse order)? **/
		    for(j=sizeof(cur_xml) / sizeof(char*) - 1; j>=0; j--)
			{
			if (last_xml[j] && (!cur_xml[j] || strcmp(last_xml[j], cur_xml[j]) != 0))
			    xsConcatQPrintf(post_params, "</%STR&HTE>", last_xml[j]);
			}

		    /** Emit opening tags? **/
		    for(j=0; j < sizeof(cur_xml) / sizeof(char*); j++)
			{
			if (cur_xml[j] && (!last_xml[j] || strcmp(last_xml[j], cur_xml[j]) != 0))
			    xsConcatQPrintf(post_params, "<%STR&HTE>", cur_xml[j]);
			}

		    /** Emit value **/
		    if (one_http_param->Parameter->Value->DataType == DATA_T_STRING)
			xsConcatQPrintf(post_params, "%STR&HTE", one_http_param->Parameter->Value->Data.String);
		    else if (one_http_param->Parameter->Value->DataType == DATA_T_INTEGER)
			xsConcatQPrintf(post_params, "%INT", one_http_param->Parameter->Value->Data.Integer);
		    else
			{
			mssError(1, "HTTP", "Unsupported data type for post parameter %s", one_http_param->Parameter->Name);
			goto error;
			}

		    /** Save param parts for next param comparison if not done **/
		    for(j=0; j < sizeof(cur_xml) / sizeof(char*); j++)
			{
			if (last_xml[j])
			    {
			    nmSysFree(last_xml[j]);
			    last_xml[j] = NULL;
			    }
			if (cur_xml[j])
			    {
			    last_xml[j] = nmSysStrdup(cur_xml[j]);
			    }
			}
		    }
		}
	    }

	/** Close any open tags (in reverse order), and free memory **/
	for(j=sizeof(cur_xml) / sizeof(char*) - 1; j>=0; j--)
	    {
	    if (cur_xml[j])
		xsConcatQPrintf(post_params, "</%STR&HTE>", cur_xml[j]);
	    if (last_xml[j])
		{
		nmSysFree(last_xml[j]);
		last_xml[j] = NULL;
		}
	    }

	return post_params;

    error:
	if (post_params)
	    xsFree(post_params);
	for(j=0; j < sizeof(cur_xml) / sizeof(char*); j++)
	    {
	    if (last_xml[j])
		nmSysFree(last_xml[j]);
	    }
	return NULL;
    }


/*** http_i_PostBodyJSON - create a POST body with JSON formatting.
 ***/
pXString
http_i_PostBodyJSON(pHttpData inf)
    {
    pXString post_params = NULL;
    pHttpParam one_http_param;
    int i, j, comma = 0, last;
    char* cur_json[16];
    char* last_json[16];
    char* param_part;
    int param_part_cnt;
    char* save_ptr;
    char cur_param[256];
    int is_array[16];	/* 0 = object, 1 = array */
    int last_is_array[16];
    int changed[16];

	post_params = xsNew();
	if (!post_params)
	    goto error;
	xsConcatenate(post_params, "{ ", 2);

	/** Need to sort them to get JSON data organized right **/
	qsort(inf->Params.Items, inf->Params.nItems, sizeof(char*), http_internal_StrCmp);

	/** Encode them **/
	for(j=0; j<sizeof(cur_json) / sizeof(char*); j++)
	    {
	    cur_json[j] = NULL;
	    last_json[j] = NULL;
	    is_array[j] = 0;
	    last_is_array[j] = 0;
	    }
	for(i=0; i<xaCount(&inf->Params); i++)
	    {
	    one_http_param = xaGetItem(&inf->Params, i);
	    if (one_http_param->Usage == HTTP_PUSAGE_T_POST)
		{
		if (!(one_http_param->Parameter->Value->Flags & DATA_TF_NULL))
		    {
		    strtcpy(cur_param, one_http_param->Parameter->Name, sizeof(cur_param));

		    /** Go through the . separated parts of the param name **/
		    param_part = strtok_r(cur_param, ".", &save_ptr);
		    param_part_cnt = 0;
		    while(param_part)
			{
			cur_json[param_part_cnt] = param_part;
			is_array[param_part_cnt] = (cur_json[param_part_cnt][0] >= '0' && cur_json[param_part_cnt][0] <= '9');
			param_part = strtok_r(NULL, ".", &save_ptr);
			param_part_cnt++;
			if (param_part_cnt >= sizeof(cur_json) / sizeof(char*) - 1)
			    {
			    mssError(1, "HTTP", "Nesting too great for JSON parameter %s", one_http_param->Parameter->Name);
			    goto error;
			    }
			}
		    for(j=param_part_cnt; j < sizeof(cur_json) / sizeof(char*); j++)
			{
			cur_json[j] = NULL;
			}

		    /** What has changed? **/
		    for(j=0; j<sizeof(cur_json) / sizeof(char*); j++)
			{
			if (j > 0 && changed[j-1])
			    changed[j] = 1;
			else
			    changed[j] = (last_json[j] && !cur_json[j]) || (!last_json[j] && cur_json[j]) || (last_json[j] && cur_json[j] && strcmp(last_json[j], cur_json[j]) != 0);
			}

		    /** Emit closing braces? **/
		    last = 1;
		    for(j=sizeof(cur_json) / sizeof(char*) - 1; j>=0; j--)
			{
			if (last_json[j] && changed[j])
			    {
			    if (!last)
				{
				if (last_is_array[j+1])
				    xsConcatenate(post_params, " ] ", 3);
				else
				    xsConcatenate(post_params, " } ", 3);
				}
			    comma = 1;
			    last = 0;
			    }
			}

		    /** New JSON object? **/
		    for(j=0; j < sizeof(cur_json) / sizeof(char*); j++)
			{
			last = (j == param_part_cnt - 1);
			if (cur_json[j] && changed[j])
			    {
			    xsConcatQPrintf(post_params, 
				    " %[, %]%[\"%STR&JSONSTR\":%] %[%STR %]", 
				    comma, 
				    !is_array[j],
				    cur_json[j], 
				    !last, 
				    (!last && is_array[j+1])?"[":"{"
				    );
			    comma = 0;
			    }
			}

		    /** Emit value **/
		    if (one_http_param->Parameter->Value->DataType == DATA_T_STRING)
			xsConcatQPrintf(post_params, "\"%STR&JSONSTR\"", one_http_param->Parameter->Value->Data.String);
		    else if (one_http_param->Parameter->Value->DataType == DATA_T_INTEGER)
			xsConcatQPrintf(post_params, "%INT", one_http_param->Parameter->Value->Data.Integer);
		    else
			{
			mssError(1, "HTTP", "Unsupported data type for post parameter %s", one_http_param->Parameter->Name);
			goto error;
			}

		    /** Save param parts for next param comparison if not done **/
		    for(j=0; j < sizeof(cur_json) / sizeof(char*); j++)
			{
			if (last_json[j])
			    {
			    nmSysFree(last_json[j]);
			    last_json[j] = NULL;
			    }
			if (cur_json[j])
			    {
			    last_json[j] = nmSysStrdup(cur_json[j]);
			    last_is_array[j] = is_array[j];
			    }
			}
		    }
		}
	    }

	/** Close any open tags (in reverse order), and free memory **/
	last = 1;
	for(j=sizeof(cur_json) / sizeof(char*) - 1; j>=0; j--)
	    {
	    if (cur_json[j])
		{
		if (!last)
		    {
		    if (last_is_array[j+1])
			xsConcatenate(post_params, " ] ", 3);
		    else
			xsConcatenate(post_params, " } ", 3);
		    }
		last = 0;
		}
	    if (last_json[j])
		{
		nmSysFree(last_json[j]);
		last_json[j] = NULL;
		}
	    }

	xsConcatenate(post_params, " }", 2);

	return post_params;

    error:
	if (post_params)
	    xsFree(post_params);
	for(j=0; j < sizeof(cur_json) / sizeof(char*); j++)
	    {
	    if (last_json[j])
		nmSysFree(last_json[j]);
	    }
	return NULL;
    }


/*** http_internal_SendRequest - send a HTTP request
 ***/
int
http_internal_SendRequest(pHttpData inf, char* path)
    {
    int rval;
    int i, j, n_output;
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
			errno = 0;
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
	    errno = 0;
	    goto error;
	    }
	errno = 0;
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
	if (rval == 0)
	    errno = ENOTCONN;
	if (rval <= 0)
	    goto error;

	/** POST parameters? **/
	if (!strcmp(inf->Method, "POST"))
	    {
	    errno = 0;
	    if (!strcmp(inf->RequestContentType, "application/x-www-form-urlencoded"))
		{
		/** Form URL Encoded request body **/
		http_internal_AddRequestHeader(inf, "Content-Type", inf->RequestContentType, 0, 0);
		post_params = http_i_PostBodyUrlencode(inf);
		}
	    else if (!strcmp(inf->RequestContentType, "text/xml") || !strcmp(inf->RequestContentType, "application/xml"))
		{
		/** XML request body **/
		http_internal_AddRequestHeader(inf, "Content-Type", inf->RequestContentType, 0, 0);
		post_params = http_i_PostBodyXML(inf);
		}
	    else if (!strcmp(inf->RequestContentType, "application/json"))
		{
		/** JSON request body **/
		http_internal_AddRequestHeader(inf, "Content-Type", inf->RequestContentType, 0, 0);
		post_params = http_i_PostBodyJSON(inf);
		}
	    else
		{
		mssError(1, "HTTP", "Invalid request content type '%s'", inf->RequestContentType);
		errno = 0;
		goto error;
		}

	    if (!post_params)
		goto error;

	    snprintf(reqlen, sizeof(reqlen), "%d", xsLength(post_params));
	    http_internal_AddRequestHeader(inf, "Content-Length", nmSysStrdup(reqlen), 1, 0);
	    }

	printf("Web client sending request, server:%s path:%s%s post:%s headers:", inf->Server, path, xsString(url_params), post_params?xsString(post_params):"");

	xsFree(url_params);
	url_params = NULL;

	/** Send Headers **/
	inf->SentAuth = 0;
	inf->HadAuth = 0;
	for(i=0; i<inf->RequestHeaders.nItems; i++)
	    {
	    hdr = (pHttpHeader)inf->RequestHeaders.Items[i];
	    if (hdr)
		{
		errno = 0;
		if (!strcasecmp(hdr->Name, "Authorization"))
		    {
		    inf->HadAuth = 1;
		    if (!inf->SendAuth)
			{
			/** Skip authorization header this time **/
			continue;
			}
		    inf->SentAuth = 1;
		    }
		if (strpbrk(hdr->Name, " \t\r\n:") || strpbrk(hdr->Value, "\r\n"))
		    {
		    mssError(1, "HTTP", "Invalid HTTP header");
		    goto error;
		    }
		rval = fdQPrintf(inf->Socket, "%STR: %STR\r\n", hdr->Name, hdr->Value);
		if (rval == 0)
		    errno = ENOTCONN;
		if (rval <= 0)
		    goto error;
		printf("%s%s=%s", (i==0)?"":",", hdr->Name, hdr->Value);
		}
	    }
	printf("\n");

	/** End-of-headers **/
	errno = 0;
	rval = fdQPrintf(inf->Socket, "\r\n");
	if (rval == 0)
	    errno = ENOTCONN;
	if (rval <= 0)
	    goto error;

	/** Post parameters? **/
	if (post_params)
	    {
	    fdWrite(inf->Socket, xsString(post_params), xsLength(post_params), 0, FD_U_PACKET);
	    xsFree(post_params);
	    post_params = NULL;
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
    pStructInf attr;
    pHttpHeader resp_hdr;
    int resp_cnt;
    char* hdr_val;
    int i;
    pParam one_param = NULL;
    pHttpParam one_http_param = NULL;
    time_t tval;
    struct tm* thetime;
    pHttpCookie cookie = NULL;
    pHttpCookie old_cookie;
    pHttpServerData sd = NULL;
    char uri[256];
    int rval;
    int return_status;

	inf->Status = 0;
	inf->Annotation=(char*)nmSysMalloc(strlen(inf->Server)+strlen(inf->Port)+strlen(inf->Path)+strlen(inf->Protocol)+6);
	if(!inf->Annotation)
	    goto error;
	if(inf->Port[0])
	    sprintf(inf->Annotation,"%s://%s:%s/%s",inf->Protocol,inf->Server,inf->Port,inf->Path);
	else
	    sprintf(inf->Annotation,"%s://%s/%s",inf->Protocol,inf->Server,inf->Path);

	/** Data about this server? **/
	snprintf(uri, sizeof(uri), "%s://%s@%s:%s", inf->Protocol, mssUserName(), inf->Server, inf->Port);
	sd = (pHttpServerData)xhLookup(&HTTP_INF.ServerData, uri);
	if (!sd)
	    {
	    sd = (pHttpServerData)nmMalloc(sizeof(HttpServerData));
	    if (!sd)
		goto error;
	    memset(sd, 0, sizeof(HttpServerData));
	    strtcpy(sd->URI, uri, sizeof(sd->URI));
	    xaInit(&sd->Cookies, 16);
	    xhAdd(&HTTP_INF.ServerData, sd->URI, (void*)sd);
	    }

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
		{
		if (errno == ECONNREFUSED)
		    {
		    return_status = HTTP_CONN_S_TCPRESET;
		    goto retry;
		    }
		goto error;
		}
	    }
	else if (!strcmp(inf->Protocol,"https") && inf->SSL_ctx)
	    {
	    if (http_internal_ConnectHttps(inf) < 0)
		{
		if (errno == EMFILE || errno == ENFILE || errno == EAGAIN || errno == ENOMEM)
		    {
		    goto error;
		    }
		else if (errno == ECONNREFUSED)
		    {
		    return_status = HTTP_CONN_S_TCPRESET;
		    goto retry;
		    }
		else
		    {
		    return_status = HTTP_CONN_S_TLSERROR;
		    goto retry;
		    }
		}
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

	/** Add cookies **/
	if (inf->AllowCookies)
	    {
	    for(i=0; i<sd->Cookies.nItems; i++)
		{
		old_cookie = (pHttpCookie)sd->Cookies.Items[i];
		ptr = nmSysMalloc(strlen(old_cookie->Name) + 1 + strlen(old_cookie->Value) + 1);
		if (!ptr)
		    goto error;
		sprintf(ptr, "%s=%s", old_cookie->Name, old_cookie->Value);
		http_internal_AddRequestHeader(inf, "Cookie", ptr, 1, 0);
		}
	    }

	/** Send the request **/
	if (http_internal_SendRequest(inf, fullpath) < 0)
	    {
	    if (errno == ECONNRESET || errno == ECONNABORTED)
		{
		return_status = HTTP_CONN_S_TCPRESET;
		goto retry;
		}
	    if (errno == ENOTCONN)
		{
		return_status = HTTP_CONN_S_RUDECLOSE;
		goto retry;
		}
	    goto error;
	    }
	nmSysFree(fullpath);
	fullpath = NULL;

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
		if (resp_cnt > 0)
		    {
		    mssError(1,"HTTP","HTTP response header parse error");
		    goto error;
		    }
		else
		    {
		    return_status = HTTP_CONN_S_RUDECLOSE;
		    goto retry;
		    }
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
		if (resp_cnt <= 1)
		    {
		    /** Short response - nothing or status line only - treat as rude close **/
		    return_status = HTTP_CONN_S_RUDECLOSE;
		    goto retry;
		    }
		if (alloc) nmSysFree(ptr);
		alloc = 0;
		break;
		}

	    /** First line? **/
	    if (resp_cnt == 0)
		{
		/** Look for HTTP -- first line of header if server is >=HTTP/1.0 **/
		if (regexec(&HTTP_INF.httpheader, ptr, 0, NULL, 0) == 0)
		    {
		    if(HTTP_OS_DEBUG) printf("regex match on HTTP header\n");
		    p1=ptr;
		    while(p1[0]!=' ' && p1-ptr<strlen(ptr)) p1++;
		    if(p1-ptr==strlen(ptr)) 
			goto error;
		    p1++;
		    inf->Status=atoi(p1);

		    /** Other non-2XX HTTP response codes treated as success? **/
		    for(i=0; i<8; i++)
			{
			if (inf->SuccessCodes[i] != 0 && inf->SuccessCodes[i] == inf->Status)
			    {
			    inf->Status = 200;
			    break;
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
	    alloc = 0;
	    }

	/** Look for cookies coming from the server.  Right now we only
	 ** allow one cookie per Set-Cookie header.  Only allow cookies from
	 ** a 2xx response or a 401 response.
	 **/
	if ((inf->Status/100 == 2 || inf->Status == 401) && inf->AllowCookies && (hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Set-Cookie")) != NULL)
	    {
	    ptr = strchr(hdr_val, '=');
	    if (ptr)
		{
		/** Found a valid cookie of the form Name=Value **/
		*ptr = '\0';
		cookie = (pHttpCookie)nmMalloc(sizeof(HttpCookie));
		if (!cookie)
		    goto error;
		memset(cookie, 0, sizeof(HttpCookie));
		cookie->Name = nmSysStrdup(hdr_val);
		cookie->Value = nmSysStrdup(ptr + 1);
		ptr2 = strchr(cookie->Value, ';');
		if (ptr2)
		    *ptr2 = '\0';
		if (!cookie->Name || !cookie->Value)
		    goto error;
		*ptr = '=';

		/** Remove any duplicates **/
		for(i=0; i<sd->Cookies.nItems; i++)
		    {
		    old_cookie = (pHttpCookie)sd->Cookies.Items[i];
		    if (!strcmp(old_cookie->Name, cookie->Name))
			{
			xaRemoveItem(&sd->Cookies, i);
			nmSysFree(old_cookie->Name);
			nmSysFree(old_cookie->Value);
			nmFree(old_cookie, sizeof(HttpCookie));
			break;
			}
		    }

		/** Add the new cookie **/
		if (HTTP_OS_DEBUG)
		    printf("Received cookie: %s=%s\n", cookie->Name, cookie->Value);
		xaAddItem(&sd->Cookies, cookie);
		cookie = NULL;
		}
	    }

	/** Failed request? **/
	if(inf->Status/100==4 || inf->Status/100==5 || (inf->Status/100==3 && !inf->AllowRedirects) )
	    {
	    /** Scanning for sub-objects on remote server? **/
	    if (inf->Status == 404 && inf->AllowSubDirs && inf->Obj->SubCnt > 1)
		{
		inf->Obj->SubCnt--;
		return_status = HTTP_CONN_S_TRYUP;
		goto retry;
		}

	    /** Check the status to see how to proceed **/
	    mssError(1,"HTTP","Object not accessible on the server (HTTP code %d)", inf->Status);
	    if (inf->Status == 404)
		{
		return_status = HTTP_CONN_S_HTTP404;
		goto retry;
		}
	    else if (inf->Status == 401)
		{
		return_status = HTTP_CONN_S_HTTP401;
		goto retry;
		}
	    else if (inf->Status/100 == 4)
		{
		return_status = HTTP_CONN_S_HTTP4XX;
		goto retry;
		}
	    else if (inf->Status/100 == 5)
		{
		return_status = HTTP_CONN_S_HTTP5XX;
		goto retry;
		}
	    else
		{
		goto error;
		}
	    }

	/** We're being redirected? **/
	if (inf->Status/100 == 3)
	    {
	    if ((hdr_val = http_internal_GetHeader(&inf->ResponseHeaders, "Location")) != NULL)
		{
		regmatch_t pmatch[6];
		if (regexec(&HTTP_INF.parsehttp, hdr_val, 6, pmatch, 0) == REG_NOMATCH)
		    {
		    mssError(1, "HTTP", "Server '%s' redirected to unparsable URL '%s'", inf->Server, hdr_val);
		    goto error;
		    }

#define COPY_FROM_PMATCH(p,m) \
		if (p) nmSysFree(p); \
		(p)=(char*)nmSysMalloc(pmatch[(m)].rm_eo-pmatch[(m)].rm_so+1);\
		if(!(p)) goto error;\
		memset((p),0,pmatch[(m)].rm_eo-pmatch[(m)].rm_so+1);\
		strncpy((p),ptr2+pmatch[(m)].rm_so,pmatch[(m)].rm_eo-pmatch[(m)].rm_so);

		/** Protocol **/
		COPY_FROM_PMATCH(inf->Protocol, 1);

		/** Insecure redirect disallowed? **/
		if (!strcmp(inf->Protocol, "http") && inf->SSL_ctx)
		    {
		    if (inf->ForceSecureRedirects)
			{
			mssError(1, "HTTP", "Warning: Server '%s' tried to redirect to insecure URL '%s', using HTTPS instead", inf->Server, hdr_val);
			nmSysFree(inf->Protocol);
			inf->Protocol = nmSysStrdup("https");
			if (!inf->Protocol)
			    goto error;
			}
		    if (!inf->AllowInsecureRedirects)
			{
			mssError(1, "HTTP", "Server '%s' attempted to redirect to insecure URL '%s'", inf->Server, hdr_val);
			goto error;
			}
		    }

		/** Server **/
		COPY_FROM_PMATCH(inf->Server, 2);
		
		/** Port **/
		if(pmatch[4].rm_so==-1) // port is optional
		    {
		    if (inf->Port) nmSysFree(inf->Port);
		    inf->Port=(char*)nmSysStrdup("");
		    if(!inf->Port)
			goto error;
		    }
		else
		    {
		    COPY_FROM_PMATCH(inf->Port, 4);
		    }

		/** Path **/
		COPY_FROM_PMATCH(inf->Path, 5);

		/** apparently a server sending a '302 found' doesn't necessarily mean
		 **   it actually has the file -- it would just rather you requested
		 **   it differently....
		 ** Try requesting /news/news.rdf/item from freebsd.org and see....
		 **/
		inf->Redirected=1;
		return_status = HTTP_CONN_S_REDIRECT;
		goto retry;
		}

	    mssError(1, "HTTP", "Redirect response without Location header!");
	    goto error;
	    }

	/** Normal connection response **/
	if (inf->Status/100 == 2)
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
		/* add charset if present */
		char * charsetStart = NULL;
		if((charsetStart = strstr(hdr_val, "charset=")) != NULL && !inf->OverrideContentCharset)
		    {
		    inf->ContentCharset = nmSysStrdup(charsetStart+8);
		    if (strchr(inf->ContentCharset, ';'))
			*(strchr(inf->ContentCharset, ';')) = '\0';
		    }
		else if(inf->ExpectedContentCharset)
		    inf->ContentCharset = nmSysStrdup(inf->ExpectedContentCharset);
		}
	    else
		{
		inf->ContentType = nmSysStrdup(inf->RestrictContentType);
		if(inf->ExpectedContentCharset)
			inf->ContentCharset = nmSysStrdup(inf->ExpectedContentCharset);
		}

	    /** Make sure the content type is allowed **/
	    rval = objIsRelatedType(inf->ContentType, inf->RestrictContentType);
	    if (rval < 0 || rval == OBJSYS_NOT_RELATED)
		{
		mssError(1, "HTTP", "Content type '%s' returned by server is not allowed (only '%s' and descendent types allowed)", inf->ContentType, inf->RestrictContentType);
		goto error;
		}
	    }

	mlxCloseSession(lex);
	if(HTTP_OS_DEBUG) printf("stream established\n");

	return HTTP_CONN_S_SUCCESS;

    error:
	/** Error exit **/
	return_status = HTTP_CONN_S_FAIL;

    retry:
	http_internal_CloseConnection(inf);
	if (alloc) nmSysFree(ptr);
	if (lex) mlxCloseSession(lex);
	if (fullpath) nmSysFree(fullpath);
	if (cookie)
	    {
	    if (cookie->Name) nmSysFree(cookie->Name);
	    if (cookie->Value) nmSysFree(cookie->Value);
	    nmFree(cookie, sizeof(HttpCookie));
	    }
	return return_status;
    }


/*** http_internal_StartConnection - send the request to the server and start
 *** retrieval of the response, retrying if we get redirected or if we have a
 *** retryable error.
 ***/
int
http_internal_StartConnection(pHttpData inf)
    {
    int	redir_cnt = 0;
    int rval;
    char* ptr;

	while(1)
	    {
	    /** Set up OpenSSL if requested **/
	    if (!inf->SSL_ctx && !strcmp(inf->Protocol, "https"))
		{
		inf->SSL_ctx = SSL_CTX_new(SSLv23_client_method());
		if (inf->SSL_ctx) 
		    {
		    SSL_CTX_set_options(inf->SSL_ctx, SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE | SSL_OP_ALL);
		    if (stAttrValue(stLookup(CxGlobals.ParsedConfig,"ssl_cipherlist"),NULL,&ptr,0) < 0) ptr="DEFAULT";
		    SSL_CTX_set_cipher_list(inf->SSL_ctx, ptr);
		    SSL_CTX_set_verify(inf->SSL_ctx, SSL_VERIFY_PEER, NULL);
		    SSL_CTX_set_verify_depth(inf->SSL_ctx, 10);
		    SSL_CTX_set_default_verify_paths(inf->SSL_ctx);
		    }
		}

	    /** Try the connection **/
	    rval = http_internal_GetPageStream(inf);

	    /** Basic result types **/
	    if (rval == HTTP_CONN_S_FAIL)
		goto error;
	    if (HTTP_OS_DEBUG)
		printf("http: connection status = %s\n", http_conn_s_desc[rval]);

	    if (rval == HTTP_CONN_S_REDIRECT)
		{
		if (HTTP_OS_DEBUG)
		    printf("trying again...(redirected: %i)\n", inf->Redirected);
		redir_cnt++;
		if (redir_cnt > inf->RedirectLimit)
		    {
		    mssError(1,"HTTP","HTTP server redirection loop");
		    goto error;
		    }
		inf->SendAuth = http_internal_GetConfigBoolean(inf, "send_auth_initial", 1, 0);
		}
	    else if (rval == HTTP_CONN_S_TRYUP)
		{
		continue;
		}
	    else if (rval == HTTP_CONN_S_SUCCESS)
		{
		break;
		}
	    else if (rval == HTTP_CONN_S_HTTP401 && !inf->SentAuth && inf->HadAuth)
		{
		inf->SendAuth++;
		continue;
		}
	    else
		{
		/** Analyze the cause vs. our retry settings **/
		if (rval == HTTP_CONN_S_HTTP404 && !(inf->RetryFor & HTTP_RETRY_F_HTTP4XX))
		    goto error;
		if (rval == HTTP_CONN_S_HTTP4XX && !(inf->RetryFor & HTTP_RETRY_F_HTTP4XX))
		    goto error;
		if (rval == HTTP_CONN_S_HTTP5XX && !(inf->RetryFor & HTTP_RETRY_F_HTTP5XX))
		    goto error;
		if (rval == HTTP_CONN_S_TCPRESET && !(inf->RetryFor & HTTP_RETRY_F_TCPRESET))
		    goto error;
		if (rval == HTTP_CONN_S_RUDECLOSE && !(inf->RetryFor & HTTP_RETRY_F_RUDECLOSE))
		    goto error;
		if (rval == HTTP_CONN_S_TLSERROR && !(inf->RetryFor & HTTP_RETRY_F_TLSERROR))
		    goto error;

		/** Retry? **/
		if (inf->RetryLimit == 0)
		    {
		    mssError(0, "HTTP", "Connection failed to '%s'.", inf->Annotation);
		    goto error;
		    }
		if (inf->RetryCount >= inf->RetryLimit)
		    {
		    mssError(0, "HTTP", "Connection retry limit exceeded to '%s'.", inf->Annotation);
		    goto error;
		    }

		/** Wait a delay and try again **/
		if (HTTP_OS_DEBUG)
		    printf("http: retry #%d, waiting %dms\n", inf->RetryCount+1, (int)(inf->RetryDelay * 1000));
		thSleep((int)(inf->RetryDelay * 1000 + 0.5));
		inf->RetryDelay *= inf->RetryBackoffRatio;
		inf->RetryCount++;
		}
	    }

	return rval;

    error:
	return -1;
    }


/*** http_internal_GetConfig() - get a configuration value
 ***/
int
http_internal_GetConfig(pHttpData inf, char* configname, pObjData pod, int datatype, int index)
    {
    pExpression exp;

	/** Normal string or expression? **/
	if (stGetAttrType(stLookup(inf->Node->Data, configname), index) == DATA_T_CODE)
	    {
	    /** Expression **/
	    if ((exp = stGetExpression(stLookup(inf->Node->Data, configname), index)) != NULL)
		{
		expBindExpression(exp, inf->ObjList, EXPR_F_RUNSERVER);
		if (expEvalTree(exp, inf->ObjList) == 0)
		    {
		    if (exp->DataType == datatype && !(exp->Flags & EXPR_F_NULL))
			{
			return expExpressionToPod(exp, datatype, pod);
			}
		    }
		}
	    return -1;
	    }
	else if (stGetAttrValueOSML(stLookup(inf->Node->Data, configname), datatype, pod, index, inf->Obj->Session, NULL) < 0)
	    {
	    /** Failed to get value **/
	    return -1;
	    }

    return 0;
    }


/*** http_internal_GetConfigString - get a configuration value as a string
 ***/
char*
http_internal_GetConfigString(pHttpData inf, char* configname, char* default_value, int index)
    {
    char* ptr = NULL;
    int n = 0;
    char intbuf[16];

	/** Try string **/
	if (http_internal_GetConfig(inf, configname, POD(&ptr), DATA_T_STRING, index) < 0)
	    ptr = NULL;

	/** Try integer **/
	if (http_internal_GetConfig(inf, configname, POD(&n), DATA_T_INTEGER, index) == 0)
	    {
	    snprintf(intbuf, sizeof(intbuf), "%d", n);
	    ptr = intbuf;
	    }

	/** Apply default? **/
	if (!ptr)
	    ptr = default_value;

	/** Valid? **/
	if (!ptr)
	    return NULL;

	/** Copy it **/
	ptr = nmSysStrdup(ptr);

	if (HTTP_OS_DEBUG) printf("%s: %s\n",configname,ptr);

    return ptr;
    }


/*** http_internal_GetConfigBoolean - get a configuration value as a boolean
 ***/
int
http_internal_GetConfigBoolean(pHttpData inf, char* configname, int default_value, int index)
    {
    char* ptr = NULL;
    int n = 0;

	/** Try string **/
	if (http_internal_GetConfig(inf, configname, POD(&ptr), DATA_T_STRING, index) < 0)
	    ptr = NULL;
	if (ptr)
	    {
	    if (!strcasecmp(ptr, "true") || !strcasecmp(ptr, "yes") || !strcasecmp(ptr, "on") || !strcmp(ptr, "1"))
		return 1;
	    else if (!strcasecmp(ptr, "false") || !strcasecmp(ptr, "no") || !strcasecmp(ptr, "off") || !strcmp(ptr, "0"))
		return 0;
	    }

	/** Try integer **/
	if (http_internal_GetConfig(inf, configname, POD(&n), DATA_T_INTEGER, index) == 0)
	    {
	    return n?1:0;
	    }

    return default_value;
    }


/*** http_internal_GetConfigInteger - get a configuration value as an integer
 ***/
int
http_internal_GetConfigInteger(pHttpData inf, char* configname, int default_value, int index)
    {
    int n = 0;

	/** Get integer **/
	if (http_internal_GetConfig(inf, configname, POD(&n), DATA_T_INTEGER, index) == 0)
	    {
	    return n;
	    }

    return default_value;
    }


/*** http_internal_GetConfigDouble - get a configuration value as a double
 ***/
double
http_internal_GetConfigDouble(pHttpData inf, char* configname, double default_value, int index)
    {
    double d = 0.0;
    int n = 0;

	/** Try double **/
	if (http_internal_GetConfig(inf, configname, POD(&d), DATA_T_DOUBLE, index) == 0)
	    {
	    return d;
	    }

	/** Try integer **/
	if (http_internal_GetConfig(inf, configname, POD(&n), DATA_T_INTEGER, index) == 0)
	    {
	    return (double)n;
	    }

    return default_value;
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
			else if (!strcmp(val, "none"))
			    one_new_http_param->Usage = HTTP_PUSAGE_T_NONE;
			else
			    {
			    mssError(1, "HTTP", "Parameter %s usage must be 'url', 'post', 'header', or 'none'", param_inf->Name);
			    goto error;
			    }
			}
		    else
			{
			mssError(1, "HTTP", "Parameter %s usage must be 'url', 'post', 'header', or 'none'", param_inf->Name);
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
		    paramSetValue(one_param, &tod, 1, inf->ObjList, inf->Obj->Session);
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
			    paramSetValueFromInfNe(one_param, one_open_ctl, 1, inf->ObjList, inf->Obj->Session);
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
    int i;

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
	inf->SSL_ctx = NULL;

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

	/** Load parameters **/
	if (http_internal_LoadParams(inf) < 0)
	    goto error;

	/** Configuration **/
	inf->ProxyServer = http_internal_GetConfigString(inf, "proxyserver", "", 0);
	inf->ProxyPort = http_internal_GetConfigString(inf, "proxyport", "80", 0);
	inf->Server = http_internal_GetConfigString(inf, "server", "", 0);
	inf->Port = http_internal_GetConfigString(inf, "port", "", 0);
	inf->Path = http_internal_GetConfigString(inf, "path", "/", 0);
	inf->Method = http_internal_GetConfigString(inf, "method", "GET", 0);
	inf->RequestContentType = http_internal_GetConfigString(inf, "request_content_type", "application/x-www-form-urlencoded", 0);
	inf->RestrictContentType = http_internal_GetConfigString(inf, "restrict_content_type", "application/octet-stream", 0);
	inf->ExpectedContentCharset = http_internal_GetConfigString(inf, "expected_content_charset", NULL, 0);
	inf->OverrideContentCharset = http_internal_GetConfigBoolean(inf, "override_content_charset", 0, 0);
	inf->AllowInsecureRedirects = http_internal_GetConfigBoolean(inf, "allow_insecure_redirects", 0, 0);
	inf->ForceSecureRedirects = http_internal_GetConfigBoolean(inf, "force_secure_redirects", 0, 0);
	inf->Protocol = http_internal_GetConfigString(inf, "protocol", "http", 0);
	inf->Cipherlist = http_internal_GetConfigString(inf, "ssl_cipherlist", "", 0);

	/** Valid method? **/
	if (strcmp(inf->Method, "GET") && strcmp(inf->Method, "POST") && strcmp(inf->Method, "DELETE"))
	    {
	    mssError(1, "HTTP", "Invalid method '%s'", inf->Method);
	    goto error;
	    }

	/** Which result codes (other than 2XX) are treated as successful? **/
	for(i=0; i<8; i++)
	    {
	    inf->SuccessCodes[i] = http_internal_GetConfigInteger(inf, "success_codes", 0, i);
	    }

	/** Retry settings **/
	inf->RetryCount = 0;
	inf->RetryLimit = http_internal_GetConfigInteger(inf, "retry_limit", 0, 0);
	inf->RedirectLimit = http_internal_GetConfigInteger(inf, "redirect_limit", HTTP_REDIR_MAX, 0);
	inf->RetryDelay = http_internal_GetConfigDouble(inf, "retry_delay", 1.0, 0);
	inf->RetryBackoffRatio = http_internal_GetConfigDouble(inf, "retry_backoff_ratio", 2.0, 0);
	inf->RetryFor = 0;
	for(i=0; i<5; i++)
	    {
	    ptr = http_internal_GetConfigString(inf, "retry_for", NULL, i);
	    if (ptr)
		{
		if (!strcmp(ptr, "tcp_reset"))
		    inf->RetryFor |= HTTP_RETRY_F_TCPRESET;
		else if (!strcmp(ptr, "rude_close"))
		    inf->RetryFor |= HTTP_RETRY_F_RUDECLOSE;
		else if (!strcmp(ptr, "http_401"))
		    inf->RetryFor |= HTTP_RETRY_F_HTTP401;
		else if (!strcmp(ptr, "http_4xx"))
		    inf->RetryFor |= HTTP_RETRY_F_HTTP4XX;
		else if (!strcmp(ptr, "http_5xx"))
		    inf->RetryFor |= HTTP_RETRY_F_HTTP5XX;
		else if (!strcmp(ptr, "tls_error"))
		    inf->RetryFor |= HTTP_RETRY_F_TLSERROR;
		else if (!strcmp(ptr, "any"))
		    inf->RetryFor = HTTP_RETRY_F_TCPRESET | HTTP_RETRY_F_RUDECLOSE | HTTP_RETRY_F_HTTP401 | HTTP_RETRY_F_HTTP4XX | HTTP_RETRY_F_HTTP5XX | HTTP_RETRY_F_TLSERROR;
		else if (!strcmp(ptr, "none") && i == 0)
		    {
		    inf->RetryFor = 0;
		    break;
		    }
		else
		    {
		    mssError(1, "HTTP", "Invalid retry_for option '%s'", ptr);
		    goto error;
		    }
		}
	    }

	/** Authentication headers **/
	inf->SendAuth = http_internal_GetConfigBoolean(inf, "send_auth_initial", 1, 0);
	if ((ptr = http_internal_GetConfigString(inf, "proxyauthline", NULL, 0)))
	    {
	    http_internal_AddRequestHeader(inf, "Proxy-Authorization", ptr, 1, 0);
	    }
	if ((ptr = http_internal_GetConfigString(inf, "authline", NULL, 0)))
	    {
	    http_internal_AddRequestHeader(inf, "Authorization", ptr, 1, 0);
	    }
	else if ((user = http_internal_GetConfigString(inf, "username", NULL, 0)))
	    {
	    pw = http_internal_GetConfigString(inf, "password", "", 0);
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
	inf->AllowBadCert = http_internal_GetConfigBoolean(inf, "allowbadcert", 0, 0);
	inf->AllowSubDirs = http_internal_GetConfigBoolean(inf, "allowsubdirs", 0, 0);
	inf->AllowRedirects = http_internal_GetConfigBoolean(inf, "allowredirects", 1, 0);
	inf->AllowCookies = http_internal_GetConfigBoolean(inf, "allowcookies", 0, 0);
	if (!inf->AllowSubDirs)
	    inf->Obj->SubCnt=1;

	/** Cache control **/
	inf->CacheMinTime = http_internal_GetConfigInteger(inf, "cache_min_ttl", 0, 0);
	if (inf->CacheMinTime < 0)
	    inf->CacheMinTime = 0;
	inf->CacheMaxTime = http_internal_GetConfigInteger(inf, "cache_max_ttl", 0, 0);
	if (inf->CacheMaxTime < 0)
	    inf->CacheMaxTime = 0;
	inf->ContentCacheMaxLen = http_internal_GetConfigInteger(inf, "cache_max_length", HTTP_DEFAULT_MEM_CACHE, 0);
	if (inf->ContentCacheMaxLen < 0)
	    inf->ContentCacheMaxLen = 0;

	/** Warn if http_4xx retry type is provided while allowing subdirs **/
	if (inf->RetryLimit > 0 && inf->AllowSubDirs && (inf->RetryFor & HTTP_RETRY_F_HTTP4XX))
	    {
	    mssError(1, "HTTP", "Warning: both retry_for http_4xx and allowsubdirs are enabled; retry on 404 Not Found is disabled.");
	    }

	/** Ensure all required config items are given **/
	if (!inf->Server[0])
	    {
	    mssError(1, "HTTP", "Server name must be provided");
	    goto error;
	    }

	/** Adjust SubCnt based on parameter usage **/
	if (!inf->AllowSubDirs)
	    inf->Obj->SubCnt = 1 + inf->ParamSubCnt;

	/** Connect to server and retrieve the response headers **/
	rval = http_internal_StartConnection(inf);
	if (rval != 0)
	    goto error;

	return (void*)inf;

    error:
	if (inf)
	    http_internal_Cleanup(inf);
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
    int maxread;
    int rval;
    int cur_msec = mtRealTicks() * 1000 / CxGlobals.ClkTck;
    int len;

	if (flags & OBJ_U_SEEK)
	    inf->ReadOffset = offset;

	if (maxcnt <= 0)
	    return 0;

	/** Complete, and we're still at end of result?  Give an EOF if so. **/
	if ((inf->Flags & HTTP_F_CONTENTCOMPLETE) && inf->ContentLength && inf->ReadOffset >= inf->ContentLength)
	    {
	    return 0;
	    }

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
	    http_internal_CloseConnection(inf);
	    rval = http_internal_StartConnection(inf);
	    if (rval != 0)
		return -1;
	    inf->NetworkOffset = 0;
	    inf->Flags &= ~HTTP_F_CONTENTCOMPLETE;
	    }

	/** if the server provided a Content-Length header, use it... **/
	if(inf->ContentLength)
	    {
	    readcnt = inf->ContentLength - inf->NetworkOffset; /* the maximum length we're allowed to request */
	    readcnt = readcnt>maxcnt?maxcnt:readcnt; /* drop down to what the requesting object wants */
	    }

	/** Still no connection? **/
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
		    {
		    inf->Flags |= HTTP_F_CONTENTCOMPLETE;
		    inf->ContentLength = inf->NetworkOffset;
		    http_internal_CloseConnection(inf);
		    }
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

	    if (inf->ContentLength && inf->NetworkOffset >= inf->ContentLength)
		{
		inf->Flags |= HTTP_F_CONTENTCOMPLETE;
		http_internal_CloseConnection(inf);
		}
	    }
	else
	    {
	    if (rval == 0 && readcnt > 0 && inf->NetworkOffset <= inf->ContentCacheMaxLen)
		inf->Flags |= HTTP_F_CONTENTCOMPLETE;

	    /** end of file - close up **/
	    if (rval < 0 || readcnt > 0 || (inf->ContentLength && inf->NetworkOffset >= inf->ContentLength))
		{
		inf->Flags |= HTTP_F_CONTENTCOMPLETE;
		inf->ContentLength = inf->NetworkOffset;
		http_internal_CloseConnection(inf);
		}
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

	/** If content-type / inner type, return as appropriate **/
	if (!strcmp(attrname,"content_type") || !strcmp(attrname,"inner_type"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}

	    if (inf->ContentType)
		val->String = inf->ContentType;
	    else
		val->String = "application/octet-stream";

	    return 0;
	    }
	if (!strcmp(attrname,"content_charset"))
	    {
	    if (datatype != DATA_T_STRING)
		{
		mssError(1,"HTTP","Type mismatch getting attribute '%s' [requested=%s, actual=string]",
			attrname, obj_type_names[datatype]);
		return -1;
		}

	    if (inf->ContentCharset)
		val->String = inf->ContentCharset;
	    else
		/* there is no standard expected charset for now */
		val->String = NULL;

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
	retval=regcomp(&HTTP_INF.parsehttp,"(http|https)://([^:/]+)(:(.+))?/(.*)",REG_EXTENDED | REG_ICASE);
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
	xhInit(&HTTP_INF.ServerData, 255, 0);

	/** Set up header nonce **/
	HTTP_INF.NonceData = cxssKeystreamNew(NULL, 0);
	if (!HTTP_INF.NonceData)
	    mssError(1, "HTTP", "Warning: X-Nonce headers will not be emitted");

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

