#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "mtask.h"
#include "mtlexer.h"
#include "exception.h"
#include "stparse.h"
#include "mtsession.h"
#include "xstring.h"
#include "newmalloc.h"

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
/* Module: 	stparse.c,stparse.h  					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	Parser to handle structured data.  This module parses	*/
/*		AND generates "structure files".  It does NOT remember	*/
/*		comments when generating a file.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: stparse.c,v 1.3 2002/06/20 15:57:05 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/src/stparse.c,v $

    $Log: stparse.c,v $
    Revision 1.3  2002/06/20 15:57:05  gbeeley
    Fixed some compiler warnings.  Repaired improper buffer handling in
    mtlexer's mlxReadLine() function.

    Revision 1.2  2002/04/25 17:54:39  gbeeley
    Small update to stparse to fix some string length issues.

    Revision 1.1.1.1  2001/08/13 18:04:22  gbeeley
    Centrallix Library initial import

    Revision 1.2  2001/07/04 02:57:58  gbeeley
    Fixed a few error checking and array-overflow problems.

    Revision 1.1.1.1  2001/07/03 01:02:59  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/*** Some function headers ***/
int st_internal_GenerateAttr(pStructInf info, char** buf, int* buflen, int* datalen, int level);
int st_internal_FLStruct(pLxSession s, pStructInf info);

/*** stAllocInf - allocate a new StructInf structure.
 ***/
pStructInf
stAllocInf()
    {
    pStructInf this;

	/** Allocate the structure **/
	this = (pStructInf)nmMalloc(sizeof(StructInf));
	memset(this,0, sizeof(StructInf));

    return this;
    }


/*** stFreeInf - release an existing StructInf, and any sub infs
 ***/
int
stFreeInf(pStructInf this)
    {
    int i,j;

	/** Free any subinfs first **/
	for(i=0;i<64;i++) if (this->SubInf[i]) stFreeInf(this->SubInf[i]);

	/** String need to be deallocated? **/
	for(i=0;i<this->nVal;i++) if (this->StrVal[i] && this->StrAlloc[i]) 
	    nmSysFree(this->StrVal[i]);

	/** Disconnect from parent if there is one. **/
	if (this->Parent)
	    {
	    for(i=0;i<this->Parent->nSubInf;i++)
	        {
		if (this == this->Parent->SubInf[i])
		    {
		    this->Parent->nSubInf--;
		    for(j=i;j<this->Parent->nSubInf;j++)
		        {
			this->Parent->SubInf[j] = this->Parent->SubInf[j+1];
			}
		    this->Parent->SubInf[this->Parent->nSubInf] = NULL;
		    }
		}
	    }

	/** Free the current one. **/
	nmFree(this,sizeof(StructInf));

    return 0;
    }


/*** stAddInf - add a subinf to the main inf structure
 ***/
int
stAddInf(pStructInf main_inf, pStructInf sub_inf)
    {
    int i;
    int found=0;

	/** Find a slot for the sub inf in the main one **/
	for(i=0;i<64;i++) if (!(main_inf->SubInf[i]))
	    {
	    main_inf->SubInf[i] = sub_inf;
	    main_inf->nSubInf++;
	    found=1;
	    break;
	    }
	if (!found) return -1;
	sub_inf->Parent = main_inf;

    return 0;
    }


/*** stAddAttr - adds an attribute to the existing inf.
 ***/
pStructInf
stAddAttr(pStructInf inf, char* name)
    {
    pStructInf newinf;

	newinf = stAllocInf();
	if (!newinf) return NULL;
	memccpy(newinf->Name, name, 0, 63);
	newinf->Name[63] = 0;
	newinf->Type = ST_T_ATTRIB;
	if (stAddInf(inf, newinf) < 0)
	    {
	    stFreeInf(newinf);
	    return NULL;
	    }

    return newinf;
    }


/*** stAddGroup - adds a subgroup to an existing inf.
 ***/
pStructInf
stAddGroup(pStructInf inf, char* name, char* type)
    {
    pStructInf newinf;

	newinf = stAllocInf();
	if (!newinf) return NULL;
	memccpy(newinf->Name, name, 0, 63);
	newinf->Name[63] = 0;
	memccpy(newinf->UsrType, type, 0, 63);
	newinf->UsrType[63] = 0;
	newinf->Type = ST_T_SUBGROUP;
	if (stAddInf(inf, newinf) < 0)
	    {
	    stFreeInf(newinf);
	    return NULL;
	    }

    return newinf;
    }


/*** stAddValue - adds a value to the attribute inf.
 ***/
int
stAddValue(pStructInf inf, char* strval, int intval)
    {

	if (inf->nVal == 64) return 0;
	if (strval)
	    {
	    inf->StrVal[inf->nVal] = strval;
	    inf->IntVal[inf->nVal++] = 0;
	    }
	else
	    {
	    inf->StrVal[inf->nVal] = NULL;
	    inf->IntVal[inf->nVal++] = intval;
	    }

    return inf->nVal;
    }


/*** stCreateStruct - creates a new command inf.
 ***/
pStructInf
stCreateStruct(char* name, char* type)
    {
    pStructInf newinf;

	newinf = stAllocInf();
	if (!newinf) return NULL;
	newinf->Type = ST_T_STRUCT;
	if (name) 
	    {
	    memccpy(newinf->Name, name, 0, 63);
	    newinf->Name[63] = 0;
	    }
	if (type) 
	    {
	    memccpy(newinf->UsrType, type, 0, 63);
	    newinf->UsrType[63] = 0;
	    }

    return newinf;
    }


/*** stLookup - find an attribute or subgroup in the protoinf and 
 *** return the attribute inf.
 ***/
pStructInf
stLookup(pStructInf this, char* name)
    {
    pStructInf inf = NULL;
    int i;

	/** Search for a subinf with the right name **/
	if (!this) return NULL;
	for(i=0;i<this->nSubInf;i++)
	    {
	    if (!strcmp(this->SubInf[i]->Name, name)) 
		{
		inf = this->SubInf[i];
		break;
		}
	    }

    return inf;
    }


/*** stAttrValue - return the value of an attribute inf.
 ***/
int
stAttrValue(pStructInf this, int* intval, char** strval, int nval)
    {

	/** Do some error-cascade checking. **/
	if (!this) return -1;
	if (this->Type != ST_T_ATTRIB) return -1;
	if (nval >= this->nVal) return -1;

	/** String or int val? **/
	if (intval) *intval = this->IntVal[nval];
	if (strval) *strval = this->StrVal[nval];

    return 0;
    }


/*** st_internal_ParseAttr - parse an attribute.  This routine
 *** should be called with the current token set at the equals
 *** sign.
 ***/
int
st_internal_ParseAttr(pLxSession s, pStructInf inf)
    {
    int toktype;

	inf->nVal = 0;

	/** Repeat in case of comma separated listing **/
	while(1)
	    {
	    /** Get the next token. **/
	    toktype = mlxNextToken(s);

	    /** Wrong type? **/
	    if (toktype != MLX_TOK_INTEGER && toktype != MLX_TOK_STRING &&
	        toktype != MLX_TOK_KEYWORD)
	        {
		mssError(1,"ST","Attribute value must be integer, string, or keyword/symbol");
		mlxNotePosition(s);
	        return -1;
	        }

	    /** Oops? **/
	    if (inf->nVal == 64)
	        {
		mssError(1,"ST","Exceeded internal representation for attribute values");
		return -1;
		}

	    /** Get the string value of the token, if string/keywd **/
    	    if (toktype == MLX_TOK_STRING || toktype == MLX_TOK_KEYWORD)
    	        {
    	        inf->IntVal[inf->nVal] = 0;
    	        inf->StrAlloc[inf->nVal] = 1;
    	        inf->StrVal[inf->nVal] = mlxStringVal(s,&(inf->StrAlloc[inf->nVal]));
		inf->nVal++;
    	        }
    	    else
    	        {
    	        inf->StrVal[inf->nVal] = NULL;
    	        inf->IntVal[inf->nVal++] = mlxIntVal(s);
    	        }
    
    	    /** See if we need to fetch a comma separated list. **/
    	    toktype = mlxNextToken(s);
    	    if (toktype != MLX_TOK_COMMA)
    	        {
    	        mlxHoldToken(s);
    	        break;
    	        }
	    }
    
    return 0;
    }
    
    
    
/*** st_internal_ParseGroup - parse a subgroup within a command
 *** or another subgroup.  Should be called with the current
 *** token set to the open brace.
 ***/
int
st_internal_ParseGroup(pLxSession s, pStructInf inf)
    {
    int toktype;
    pStructInf subinf;
    char* str;

	while(1)
	    {
	    toktype = mlxNextToken(s);
	    if (toktype == MLX_TOK_CLOSEBRACE) break;
	    else if (toktype == MLX_TOK_KEYWORD)
		{
		/** Check the string **/
		str = mlxStringVal(s,NULL);
		if (!str) 
		    {
		    mssError(0,"ST","Could not obtain name of structure file group");
		    mlxNotePosition(s);
		    return -1;
		    }

		/** Create a structure to use **/
		subinf = stAllocInf();
		if (stAddInf(inf, subinf) < 0)
		    {
		    stFreeInf(subinf);
		    mssError(1,"ST","Internal representation exceeded");
		    return -1;
		    }
		memccpy(subinf->Name, str, 0, 31);
		subinf->Name[31] = 0;

		/** If a subgroup, will have a type.  Check for it. **/
		toktype = mlxNextToken(s);
		if (toktype == MLX_TOK_STRING)
		    {
		    mlxCopyToken(s,subinf->UsrType,64);
		    toktype = mlxNextToken(s);
		    }

		/** Check next token.  eq means attrib, brace means subgrp **/
		if (toktype == MLX_TOK_EQUALS)
		    {
		    if (subinf->UsrType[0] != 0) 
		        {
			mssError(1,"ST","Attributes do not have explicit object types");
		        mlxNotePosition(s);
			return -1;
			}
		    subinf->Type = ST_T_ATTRIB;
		    if (st_internal_ParseAttr(s,subinf) < 0) return -1;
		    }
		else if (toktype == MLX_TOK_OPENBRACE)
		    {
		    if (subinf->UsrType[0] == 0)
		        {
			mssError(1,"ST","Subgroups must have an object type");
		        mlxNotePosition(s);
			return -1;
			}
		    subinf->Type = ST_T_SUBGROUP;
		    if (st_internal_ParseGroup(s,subinf) < 0) return -1; 
		    }
		else
		    {
		    mssError(1,"ST","Expected 'type' { or = after subgroup/attr name");
		    mlxNotePosition(s);
		    return -1;
		    }
		}
	    else
		{
		mssError(1,"ST","Structure file group/attr name is missing");
		mlxNotePosition(s);
		return -1;
		}
	    }

    return 0;
    }


/*** st_internal_IsDblOpen - check a string to see if it is a double-open-
 *** brace on a line by itself with only surrounding whitespace.
 ***/
int
st_internal_IsDblOpen(char* str)
    {
    	
	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** Double open? **/
	if (str[0] != '{' || str[1] != '{') return 0;

	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** End of line? **/
	if (*str != '\0') return 0;

    return 1;
    }


/*** st_internal_IsDblClose - check a string to see if it is a double-close-
 *** brace on a line by itself.
 ***/
int
st_internal_IsDblClose(char* str)
    {
    	
	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** Double open? **/
	if (str[0] != '}' || str[1] != '}') return 0;

	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** End of line? **/
	if (*str != '\0') return 0;

    return 1;
    }


/*** st_internal_ParseScript - parse an embedded JavaScript segment within
 *** a structure file.  This is called once a {{ has been encountered 
 *** during group processing, and this routine will continue until a
 *** matching }} is encountered.
 ***/
int
st_internal_ParseScript(pLxSession s, pStructInf info)
    {
    int t;
    char* str;
    int alloc;
    XString xs;

    	/** Throw the lexer into linemode. **/
	mlxSetOptions(s, MLX_F_LINEONLY);
	xsInit(&xs);

	/** Start reading the lines and copy 'em to the script buffer **/
	while(1)
	    {
	    /** Token must be a string -- one script line. **/
	    t = mlxNextToken(s);
	    if (t != MLX_TOK_STRING) 
	        {
		mssError(0,"ST","Unexpected error parsing script '%s'", info->Name);
		return -1;
		}

	    /** Get the value of the thing. **/
	    alloc = 0;
	    str = mlxStringVal(s,&alloc);

	    /** Is this the end of the script? **/
	    if (st_internal_IsDblClose(str))
	        {
		if (alloc) nmSysFree(str);
		break;
		}

	    /** Breaking out of script and into structure file again? **/
	    if (st_internal_IsDblOpen(str))
	        {
		/*if (*/
		}
	    }

	/** Revert back to token mode **/
	xsDeInit(&xs);
	mlxUnsetOptions(s, MLX_F_LINEONLY);

    return 0;
    }


/*** st_internal_ParseStruct - parse a command structure from the 
 *** input stream.
 ***/
int
st_internal_ParseStruct(pLxSession s, pStructInf *info)
    {
    int toktype;
    Exception parse_err;

	/** Allocate the structure **/
	*info = stAllocInf();
	if (!*info) return -1;
	(*info)->Type = ST_T_STRUCT;

	/** In case we get a parse error: **/
	Catch(parse_err)
	    {
	    stFreeInf(*info);
	    *info = NULL;
	    mssError(1,"ST","Parse error while reading structure file top-level group");
	    mlxNotePosition(s);
	    return -1;
	    }

	/** Get a token to see what command we have. **/
	if (mlxNextToken(s) != MLX_TOK_KEYWORD) Throw(parse_err);
	mlxCopyToken(s,(*info)->Name,32);

	/** GRB 2/2000 - Is this a FormLayout style file? **/
	if (!strcmp((*info)->Name, "BEGIN"))
	    {
	    return st_internal_FLStruct(s, *info);
	    }

	/** If a subgroup, will have a type.  Check for it. **/
	toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_STRING) Throw(parse_err);
	mlxCopyToken(s,(*info)->UsrType,64);

	/** Check for the open brace **/
	/** Double open brace {{ with "system/script" means Script group, not Struct group **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_DBLOPENBRACE && !strcmp((*info)->UsrType,"system/script"))
	    {
	    if (st_internal_ParseScript(s,*info) < 0) Throw(parse_err);
	    }
	else if (toktype == MLX_TOK_OPENBRACE)
	    {
	    /** Ok, got the command header.  Now do the attribs/subs **/
	    if (st_internal_ParseGroup(s,*info) < 0) Throw(parse_err);
	    }
	else
	    {
	    Throw(parse_err);
	    }


    return 0;
    }


/*** stParseMsg - parse an incoming message from a file or network 
 *** connection.
 ***/
pStructInf
stParseMsg(pFile inp_fd, int flags)
    {
    pStructInf info;
    pLxSession s;

	/** Open a session with the lexical analyzer **/
	s = mlxOpenSession(inp_fd, MLX_F_CPPCOMM | MLX_F_DBLBRACE);
	if (!s) 
	    {
	    mssError(0,"ST","Could not begin analysis of structure file");
	    return NULL;
	    }

	/** Parse a command **/
	st_internal_ParseStruct(s, &info);

	/** Close the lexer session **/
	mlxCloseSession(s);

    return info;
    }


/*** stParseMsgGeneric - parse an incoming message from a generic
 *** descriptor via a read-function.
 ***/
pStructInf
stParseMsgGeneric(void* src, int (*read_fn)(), int flags)
    {
    pStructInf info;
    pLxSession s;

	/** Open a session with the lexical analyzer **/
	s = mlxGenericSession(src,read_fn, MLX_F_CPPCOMM | MLX_F_DBLBRACE);
	if (!s) 
	    {
	    mssError(0,"ST","Could not begin analysis of structure file");
	    return NULL;
	    }

	/** Parse a command **/
	st_internal_ParseStruct(s, &info);

	/** Close the lexer session **/
	mlxCloseSession(s);

    return info;
    }


/*** st_internal_CkAddBuf - check to see if we need to realloc on the buffer
 *** to add n characters.
 ***/
int
st_internal_CkAddBuf(char** buf, int* buflen, int* datalen, int n)
    {

	while (*datalen + n >= *buflen)
	    {
	    *buflen += 1024;
	    *buf = (char*)nmSysRealloc(*buf, *buflen);
	    if (!*buf) 
	        {
		mssError(1,"ST","Insufficient memory to load structure file data");
		return -1;
		}
	    }

    return 0;
    }


/*** st_internal_GenerateGroup - output a group listing from an info
 *** structure.
 ***/
int
st_internal_GenerateGroup(pStructInf info, char** buf, int* buflen, int* datalen, int level)
    {
    int i;

	/** Print the header line. **/
	if (st_internal_CkAddBuf(buf,buflen,datalen,strlen(info->Name)+7+strlen(info->UsrType)+level*4+3) <0) return -1;
	sprintf((*buf)+(*datalen),"%*.*s%s \"%s\"\r\n%*.*s{\r\n",level*4,level*4,"",info->Name,info->UsrType,level*4+4,level*4+4,"");
	(*datalen) += strlen((*buf)+(*datalen));

	/** Print any sub-info parts and attributes **/
	for(i=0;i<64;i++) if (info->SubInf[i])
	    {
	    switch(info->SubInf[i]->Type)
		{
		case ST_T_ATTRIB:
		    st_internal_GenerateAttr(info->SubInf[i], buf, buflen, datalen, level+1);
		    break;
		case ST_T_SUBGROUP:
		    st_internal_GenerateGroup(info->SubInf[i], buf, buflen, datalen, level+1);
		    break;
		}
	    }

	/** Print trailing line **/
	if (st_internal_CkAddBuf(buf,buflen,datalen,level*4+3) <0) return -1;
	sprintf((*buf)+(*datalen),"%*.*s}\r\n",level*4+4,level*4+4,"");
	(*datalen) += strlen((*buf)+(*datalen));

    return 0;
    }


/*** st_internal_GenerateAttr - output a single attribute from an
 *** info structure.
 ***/
int
st_internal_GenerateAttr(pStructInf info, char** buf, int* buflen, int* datalen, int level)
    {
    int i;

	/** Add the attribute name and the = sign... **/
	if (st_internal_CkAddBuf(buf,buflen,datalen,strlen(info->Name)+3+level*4) <0) return -1;
	sprintf((*buf)+(*datalen), "%*.*s%s = ", level*4, level*4, "", info->Name);
	*datalen += (strlen(info->Name) + 3 + level*4);

	/** Now print the attribute as the appropriate data type. **/
	for(i=0;i<info->nVal;i++)
	    {
	    if (info->StrVal[i] == NULL)
	        {
	        if (st_internal_CkAddBuf(buf,buflen,datalen,16) <0) return -1;
	        sprintf((*buf)+(*datalen), "%d", info->IntVal[i]);
		(*datalen) += strlen((*buf)+(*datalen));
		}
	    else
		{
	        if (st_internal_CkAddBuf(buf,buflen,datalen,strlen(info->StrVal[i])+2) <0) return -1;
		sprintf((*buf)+(*datalen), "\"%s\"", info->StrVal[i]);
		(*datalen) += strlen(info->StrVal[i])+2;
		}
	
	    /** comma if more values coming **/
	    if (st_internal_CkAddBuf(buf,buflen,datalen,2) <0) return -1;
	    if (i+1<info->nVal) 
		{
		sprintf((*buf)+(*datalen),", ");
		(*datalen) += 2;
		}
	    else
		{
		sprintf((*buf)+(*datalen),"\r\n");
		(*datalen) += 2;
		}
	    }
    
    return 0;
    }


/*** stGenerateMsg - generate a message to output to a file or network
 *** connection.
 ***/
int
stGenerateMsgGeneric(void* dst, int (*write_fn)(), pStructInf info, int flags)
    {
    char* buf;
    int buflen;
    int datalen;
    int i;

	/** Allocate our initial buffer **/
	buf = (char*)nmSysMalloc(1024);
	buflen = 1024;
	datalen = 0;

	/** Start the structure header. **/
	if (st_internal_CkAddBuf(&buf,&buflen,&datalen,strlen(info->Name)+12+strlen(info->UsrType)) <0) return -1;
	sprintf((buf)+(datalen),"%s \"%s\"\r\n    {\r\n",info->Name,info->UsrType);
	datalen = strlen(buf);

	/** Print any sub-info parts and attributes **/
	for(i=0;i<64;i++) if (info->SubInf[i])
	    {
	    switch(info->SubInf[i]->Type)
		{
		case ST_T_ATTRIB:
		    st_internal_GenerateAttr(info->SubInf[i], &buf, &buflen, &datalen, 1);
		    break;
		case ST_T_SUBGROUP:
		    st_internal_GenerateGroup(info->SubInf[i], &buf, &buflen, &datalen, 1);
		    break;
		}
	    }

	/** Put the trailing brace on and write the thing. **/
	if (datalen+7 >= buflen) 
	    {
	    buf = (char*)nmSysRealloc(buf,buflen+1024);
	    buflen += 1024;
	    }
        sprintf(buf+datalen, "    }\r\n");
        datalen += 7;
	write_fn(dst, buf, datalen, 0,0);

    return 0;
    }

int
stGenerateMsg(pFile fd, pStructInf info, int flags)
    {
    return stGenerateMsgGeneric((void*)fd, fdRead, info, flags);
    }


/*** st_internal_FLStruct - parse a FormLayout structure file.
 ***/
int
st_internal_FLStruct(pLxSession s, pStructInf info)
    {
    Exception parse_err, mem_err;
    int toktype;
    char* ptr;
    pStructInf new_info = NULL;

	/** In case we get a parse error: **/
	Catch(parse_err)
	    {
	    if (new_info) stFreeInf(new_info);
	    new_info = NULL;
	    mssError(1,"ST","Parse error while parsing a FormLayout group");
	    mlxNotePosition(s);
	    return -1;
	    }
	Catch(mem_err)
	    {
	    if (new_info) stFreeInf(new_info);
	    new_info = NULL;
	    mssError(1,"ST","Internal representation exceeded while parsing a FormLayout group");
	    mlxNotePosition(s);
	    return -1;
	    }

	/** Info already has "BEGIN"... check for type and name **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_PERIOD) toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_KEYWORD) Throw(parse_err);
	sprintf(info->UsrType,"widget/%.50s",mlxStringVal(s,NULL));
	toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_KEYWORD && toktype != MLX_TOK_STRING) Throw(parse_err);
	sprintf(info->Name,"%.63s",mlxStringVal(s,NULL));

	/** Ok, now we have a list of attributes and subgroups **/
	while(1)
	    {
	    toktype = mlxNextToken(s);
	    if (toktype == MLX_TOK_COMMA)
	        {
		mlxNextToken(s);
		continue;
		}
	    if (toktype != MLX_TOK_KEYWORD) Throw(parse_err);
	    ptr = mlxStringVal(s,NULL);
	    if (!strcmp(ptr,"END")) break;
	    if (!strcmp(ptr,"EndProperty")) continue;
	    if (!strcmp(ptr,"ENDGRIDCOLUMN")) break;
	    if (!strcmp(ptr,"BeginProperty"))
	        {
		mlxNextToken(s);
		continue;
		}
	    if (!strcmp(ptr,"BEGIN"))
	        {
		new_info = stAllocInf();
		new_info->Type = ST_T_SUBGROUP;
		if (stAddInf(info,new_info) < 0) Throw(mem_err);
		st_internal_FLStruct(s,new_info);
		continue;
		}
	    new_info = stAllocInf();
	    new_info->Type = ST_T_ATTRIB;
	    if (stAddInf(info,new_info) < 0) Throw(mem_err);
	    sprintf(new_info->Name,"%.63s",mlxStringVal(s,NULL));
	    toktype = mlxNextToken(s);
	    if (toktype != MLX_TOK_EQUALS) Throw(parse_err);
	    toktype = mlxNextToken(s);
	    if (toktype == MLX_TOK_STRING || toktype == MLX_TOK_KEYWORD)
	        {
		new_info->StrVal[0] = nmSysStrdup(mlxStringVal(s,NULL));
		new_info->nVal = 1;
		new_info->StrAlloc[0] = 1;
		}
	    else
	        {
		new_info->IntVal[0] = mlxIntVal(s);
		new_info->nVal = 1;
		}
	    }

    return 0;
    }
