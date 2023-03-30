#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "centrallix.h"

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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_params.c: handle object reference file params	*/
/************************************************************************/



/*** objParamsRead - load in parameters from the given file.  The results
 *** go into an XArray of ObjParam structures.
 ***/
pXArray 
objParamsRead(pFile fd)
    {
    pXArray xa;
    pLxSession s;
    pObjParam op;
    int t;
    int mlxFlags;

	/** Allocate the array **/
	xa = (pXArray)nmMalloc(sizeof(XArray));
	xaInit(xa,16);

	/** Open the lexer session **/
	mlxFlags = MLX_F_EOF | MLX_F_POUNDCOMM;
	if(CxGlobals.CharacterMode == CharModeUTF8) mlxFlags |= MLX_F_ENFORCEUTF8;
	s = mlxOpenSession(fd, MLX_F_EOF | MLX_F_POUNDCOMM);

	/** Read through the file, gathering the params **/
	while((t = mlxNextToken(s)) != MLX_TOK_EOF)
	    {
	    if (t != MLX_TOK_KEYWORD) 
		{
		mlxCloseSession(s);
		objParamsFree(xa);
		return NULL;
		}
	    op = (pObjParam)nmMalloc(sizeof(ObjParam));
	    if (!op)
		{
		mlxCloseSession(s);
		objParamsFree(xa);
		return NULL;
		}
	    mlxCopyToken(s, op->Name, 32);
	    if (mlxNextToken(s) != MLX_TOK_EQUALS) 
		{
		mlxCloseSession(s);
		objParamsFree(xa);
		return NULL;
		}
	    t = mlxNextToken(s);
	    if (t == MLX_TOK_INTEGER)
		{
		op->IntParam = mlxIntVal(s);
		op->Type = DATA_T_INTEGER;
		}
	    else if (t == MLX_TOK_STRING)
		{
		mlxCopyToken(s, op->StringParam, 80);
		op->Type = DATA_T_STRING;
		}
	    else
		{
		mlxCloseSession(s);
		objParamsFree(xa);
		return NULL;
		}
	    xaAddItem(xa, (void*)op);
	    }

	/** Close the session. **/
	mlxCloseSession(s);

    return xa;
    }


/*** objParamsWrite - writes out the parameter structure to a param file
 *** specified by the fd.
 ***/
int 
objParamsWrite(pFile fd, pXArray params)
    {
    int i;
    pObjParam op;
    char sbuf[128];

	/** Loop through the params **/
	for(i=0;i<params->nItems;i++)
	    {
	    op = (pObjParam)(params->Items[i]);
	    if (op->Type == DATA_T_INTEGER)
		{
		sprintf(sbuf,"%s = %d\n",op->Name,op->IntParam);
		fdWrite(fd,sbuf,strlen(sbuf),0,0);
		}
	    else if (op->Type == DATA_T_STRING)
		{
		sprintf(sbuf,"%s = \"%s\"\n",op->Name,op->StringParam);
		fdWrite(fd,sbuf,strlen(sbuf),0,0);
		}
	    }

    return 0;
    }


/*** objParamsLookupInt - get an integer value from the params list.
 ***/
int 
objParamsLookupInt(pXArray params, char* name)
    {
    int i;
    pObjParam op;

	/** Loop through looking for 'name'. **/
	for(i=0;i<params->nItems;i++)
	    {
	    op=(pObjParam)(params->Items[i]);
	    if (!strcmp(name,op->Name)) return op->IntParam;
	    }

    return -1;
    }



/*** objParamsLookupString - get a string value from the params.
 ***/
char* 
objParamsLookupString(pXArray params, char* name)
    {
    int i;
    pObjParam op;

	/** Loop through looking for 'name'. **/
	for(i=0;i<params->nItems;i++)
	    {
	    op=(pObjParam)(params->Items[i]);
	    if (!strcmp(name,op->Name)) return op->StringParam;
	    }

    return NULL;
    }


/*** objParamsSet - sets and/or adds a new parameter/value.  If stringval
 *** is NULL, then this sets an integer value.
 ***/
int 
objParamsSet(pXArray params, char* name, char* stringval, int intval)
    {
    int i;
    pObjParam op,find_op=NULL;

	/** Loop through looking for 'name'. **/
	for(i=0;i<params->nItems;i++)
	    {
	    op=(pObjParam)(params->Items[i]);
	    if (!strcmp(name,op->Name))
		{
		find_op = op;
		}
	    }

	/** Need to allocate a new one? **/
	if (!find_op)
	    {
	    find_op = (pObjParam)nmMalloc(sizeof(ObjParam));
	    if (!find_op) return -1;
	    xaAddItem(params, (void*)find_op);
	    }

	/** Set the op. **/
	if (stringval) 
	    {
	    strncpy(find_op->StringParam,stringval,79);
	    find_op->StringParam[79]=0;
	    find_op->Type = DATA_T_STRING;
	    }
	else
	    {
	    find_op->IntParam = intval;
	    find_op->Type = DATA_T_INTEGER;
	    }

    return 0;
    }


/*** objParamsFree - deallocate the xarray as well as the various
 *** param structures within it.
 ***/
int 
objParamsFree(pXArray params)
    {
    int i;

	/** Scan through the array, deallocating the structures **/
	for(i=0;i<params->nItems;i++)
	    {
	    nmFree(params->Items[i],sizeof(ObjParam));
	    }

	/** De init the xarray. **/
	xaDeInit(params);

	/** Now free the xarray. **/
	nmFree(params,sizeof(XArray));

    return 0;
    }

