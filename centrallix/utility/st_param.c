#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "xstring.h"
#include "stparse.h"
#include "st_node.h"
#include "expression.h"
#include "st_param.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 2000-2001 LightSys Technology Services, Inc.		*/
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
/* Module:      st_param.c						*/
/* Author:      Tim Young (TCY)                                         */
/* Creation:    February 18, 2000                                       */
/* Description: Functions to parse a parametarized structure file.      */
/*              These functions will allow variable substitution via    */
/*              parameters within the structure file, parameters in     */
/*              the path, and setattr calls.  It will return an         */
/*              overridden value or the default, in that order.         */
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: st_param.c,v 1.1 2001/08/13 18:01:18 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/st_param.c,v $

    $Log: st_param.c,v $
    Revision 1.1  2001/08/13 18:01:18  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/08/07 02:31:19  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

/*** 
 *** stpAllocInf(Node)
 ***	Take a Node, create override inf, parse node items into override
 *** stpLookup
 *** 	Search override struct first, then Node.
 ***	If needs evaluation, it will be in override
 ***		userdata points to expression tree.
 *** stpAddAttr
 *** 	Add an attribute to the override structure
 *** stpAddValue?
 *** stpAttrValue
 *** 	Look at the current atribute, return value
 ***		Evaluate if needed
 *** stpAttrType
 *** 	Look at the current atribute, return type
 ***		Type may change after evaluating
 *** stpUpdateNode?
 *** 	Update the node data with the override data?  Set as dirty
 *** stpSubstParam
 ***	Take a string and do the variable substitution
 *** stpParsePathOverrides
 *** 	Parse PathPart and do overrides
 *** stpMakeWhereTree ??
 ***	Make a whereclause tree from override struct, pathpart and pObj Struct
 ***	SHould this be done on individual basis, in this lib, or elsewhere? 
 *** stpFreeInf
 ***	DeAlloc the structure
 ***/

/*** Future modifications:
 *** Better handling of evaluated constants.  Right now only non-referential
 *** 	constants are evaluated and stored in the override structure.
 ***	A reference to an unchanging item could be evaluated to a constant
 ***	after it's first reference.
 ***
 *** Finish the code for stpUpdateNode.
 ***	This function should make changes to the root node based on the
 *** 	override structure.
 ***/

int
stp_internal_evaluate_parameter(pStpInf inf, pStructInf attr, pObject obj)
    {
    pExpression expression;
    pParamObjects ExpParamObjects = NULL;
    char* tmpstring;
    char tstr[255];
    int exprsuccess;
    pXString tmpxstring;
    pStructInf tmpinf;
	/* We only have to parse strings */
   	if (attr->StrVal[0] != 0)
	    {

	    /* Parametarize */
	    tmpxstring = stpSubstParam(inf, attr->StrVal[0]);
	    memccpy(tstr, tmpxstring->String, '\0', 254);
	    tstr[254] = '\0'; 

	    /* Evaluate */
	    exprsuccess= -1;
	    ExpParamObjects = expCreateParamList();
	    expression = expCompileExpression(tstr, ExpParamObjects,
					     MLX_F_ICASE, 0);
	    if (expression && (expression->ObjCoverageMask == 0))
		{
		/* Just constants.  Evaluate now */
	        expAddParamToList(ExpParamObjects, "me", obj, EXPR_O_CURRENT);
		exprsuccess = expEvalTree(expression, ExpParamObjects);
		}
	    else
		{
		attr->UserData=(void*)expression;
		}

	    /* Create new overrideinf */
	    if (expression && (exprsuccess != -1))
		{
		/* the expression worked.  Use new value in new inf */
		if (expression->DataType != DATA_T_STRING)
		    {
		    sprintf(tstr,"%d",expression->Integer);
	            tmpstring=(char*)malloc(sizeof(char*) *
 			strlen(tstr) +1);
	            strcpy(tmpstring, tstr);
		    }
		else
		    {
	            tmpstring=(char*)malloc(sizeof(char*) *
 			strlen(expression->String) +1);
	            strcpy(tmpstring, expression->String);
		    }
	    	tmpinf=stLookup(inf->OverrideInf, attr->Name);
	    	if (!tmpinf)
		    {
		    tmpinf = stpAddAttr(inf, attr->Name);
	    	    stAddValue(tmpinf, tmpstring, 0);
		    }
	    	else
		    {
		    free(tmpinf->StrVal[0]);
		    tmpinf->StrVal[0]=tmpstring;
    		    /* stAddValue(tmpinf, tmpstring, 0); */
		    }
		expFreeExpression(expression);
		}
	    else
		{
		/* the expression failed.  use tstr if needed */
		if (strcmp(tstr, attr->StrVal[0]))
		    {
	            tmpstring=(char*)malloc(sizeof(char*) * strlen(tstr) +1);
	            strcpy(tmpstring, tstr);
	    	    tmpinf=stLookup(inf->OverrideInf, attr->Name);
	    	    if (!tmpinf)
			{
			tmpinf = stpAddAttr(inf, attr->Name);
	    		stAddValue(tmpinf, tmpstring, 0);
			}
	    	    else
			{
			free(tmpinf->StrVal[0]);
	    		stAddValue(tmpinf, tmpstring, 0);
			}
		    }
		}
	    if (ExpParamObjects) expFreeParamList(ExpParamObjects);
	    }
    }

pStpInf
stpAllocInf(pSnNode node, pStructInf openctl, pObject obj, int DefaultVersion)
    {
    pStpInf inf;
    pStructInf tmpinf;
    int i;
	/** allocate structure **/
	if (!node)
	    {
	    mssError(1,"STP","No ST Node passed into STP!");
	    return NULL;
	    }
	inf = (pStpInf)nmMalloc(sizeof(StpInf));
	if (!inf) return NULL;
	inf->Node = node;
	inf->Node->OpenCnt++;
	inf->CurrAttr = -1;
	inf->Obj = obj;
	inf->nSubInf = 0;
	inf->OverrideInf = stAllocInf();
        inf->OverrideInf->Type = ST_T_STRUCT;

	/** If version exists, set it to value returned.  Else =1 **/
	if (stAttrValue(stLookup(inf->Node->Data,"version"),&(inf->Version),NULL,0) < 0)
            inf->Version = DefaultVersion;

	stp_internal_register_node_attrib(inf);
	
	/* get override from pathname parameter list*/
	if (openctl)
	    {
            for(i=0;i<openctl->nSubInf;i++)
                {
                if (openctl->SubInf[i]->Type == ST_T_ATTRIB)
                    {
		    tmpinf=stLookup(inf->OverrideInf, openctl->SubInf[i]->Name);
		    if (!tmpinf)
		        {
		        tmpinf = stpAddAttr(inf, openctl->SubInf[i]->Name);
		        }
		    /** tmpinf is either a new inf, or the original **/
		    tmpinf->StrVal[0]=openctl->SubInf[i]->StrVal[0];
		    tmpinf->nVal = 1;
		    }
	        }
	    }

	/** Parse all attributes from Node which need to be parsed **/
	/** Only strings need to be parsed **/
            for(i=0;i<inf->Node->Data->nSubInf;i++)
                {
                if (inf->Node->Data->SubInf[i]->Type == ST_T_ATTRIB)
                    {
                    /** parametarize and evaluate **/
		    if (inf->Node->Data->SubInf[i]->StrVal[0] &&
			strcmp(inf->Node->Data->SubInf[i]->Name,"primarykey") &&
		        strcmp(inf->Node->Data->SubInf[i]->Name,"sql") &&
		        strcmp(inf->Node->Data->SubInf[i]->Name,"version"))
		        stp_internal_evaluate_parameter( inf, 
			    inf->Node->Data->SubInf[i], obj);

                    }
                }
	/** Parse all attributes from override, just in case **/
	/** Only strings need to be parsed **/
            for(i=0;i<inf->OverrideInf->nSubInf;i++)
                {
                if (inf->OverrideInf->SubInf[i]->Type == ST_T_ATTRIB)
                    {
		    stp_internal_evaluate_parameter( inf, 
			inf->OverrideInf->SubInf[i], obj);
		    }
		}

    return inf;
    }

stp_internal_register_attrib(pStpInf inf, pStructInf subinf)
    {
    int i, flag=0;
 	for (i=0; i<inf->nSubInf; i++)
	    {
		if (!strcmp(inf->AttrList[i], subinf->Name))
		    {
		    	flag = 1;
		    }
	    }
	/** If not already in list **/
	if (!flag) inf->AttrList[inf->nSubInf++] = subinf->Name; 
    }

stp_internal_register_node_attrib(pStpInf inf)
    {
    int i;
    pStructInf subinf;
        for(i=0;i<inf->Node->Data->nSubInf;i++)
            {   
            subinf = (pStructInf)(inf->Node->Data->SubInf[i]);
                    
            /** Version 1: top-level attribute inf **/
            if (inf->Version == 1)
                {   
                if (subinf->Type == ST_T_ATTRIB)
                    {
                    inf->AttrList[inf->nSubInf++] = subinf->Name; 
                    }
                }   
            else    
                {   
                /** Version 2: top-level subgroup with default attr **/
                if (subinf->Type == ST_T_SUBGROUP && !strcmp(subinf->UsrType,"system/parameter"))
                    {
                    inf->AttrList[inf->nSubInf++] = subinf->Name; 
                    } 
                }
            }
            inf->AttrList[inf->nSubInf] = NULL; 
    }


/** stpGetNextAttr  **/
char*
stpGetNextAttr(pStpInf inf)
    {
        if (inf->CurrAttr >= inf->nSubInf)
	    return NULL;
	if (inf->CurrAttr == -1)
	    {
	    if (inf->nSubInf > 0)
		{
		inf->CurrAttr = 0;
		return inf->AttrList[inf->CurrAttr];
		}
	    /* No items in list */
	    return NULL; 
	    }
	inf->CurrAttr++;
	return inf->AttrList[inf->CurrAttr];
    }

char*
stpGetFirstAttr(pStpInf inf)
    {
        inf->CurrAttr = -1;
	return stpGetNextAttr(inf);
    }

/*** stpLookup - retrieves a parameter value from either the
 *** structure file or from the parameter override structure.
 ***/
pStructInf
stpLookup(pStpInf inf, char* paramname)
    {
    pStructInf find_inf;

        /** Look for it in the param override structure first **/
        find_inf = stLookup(inf->OverrideInf, paramname);

        /** If not found, look in the structure file **/
        if (!find_inf)
            {
            if (inf->Version == 1)
                {
                /** Version 1: top-level attr **/
                find_inf = stLookup(inf->Node->Data, paramname);
                }
            else
                {
                /** Version 2: system/parameter, default attr **/
                find_inf = stLookup(inf->Node->Data, paramname);
                if (!find_inf) return NULL;
                find_inf = stLookup(find_inf,"default");
                }
            }
    return find_inf;
    }

/*** stpAddInf - add a subinf to the main inf structure
 ***/
int
stpAddInf(pStpInf main_inf, pStructInf sub_inf)
    {
        stp_internal_register_attrib(main_inf, sub_inf);
    return stAddInf(main_inf->OverrideInf, sub_inf);
    }


/*** stpAddAttr - adds an attribute to the existing inf.
 ***/
pStructInf
stpAddAttr(pStpInf inf, char* name)
    {
    pStructInf newinf, tmp_inf;
	/** Things in the override structure are only Verision 1 **/
        tmp_inf = stAddAttr(inf->OverrideInf, name);
        stp_internal_register_attrib(inf, tmp_inf);
    return tmp_inf;
    }


/*** stpAttrValue **/
int
stpAttrValue(pStructInf attr, pObject obj, int* intval, char** strval, int nval)
    {
    int flag;
    pExpression expression;
    pParamObjects ExpParamObjects = NULL;
    	if (attr->UserData)
	    {
		expression = (pExpression)(attr->UserData);
	        ExpParamObjects = expCreateParamList();
	        expAddParamToList(ExpParamObjects, "me", obj, EXPR_O_CURRENT);
		flag = expEvalTree(expression,ExpParamObjects);
		if (flag != -1)
		    {
		    if (expression->Flags == EXPR_F_NULL) flag = -1;
		    else
			{
            		if (expression->DataType == DATA_T_INTEGER)
			    *intval = expression->Integer;
            		if (expression->DataType == DATA_T_STRING)
			    *strval = expression->String;
			}
		    }
	        if (ExpParamObjects) expFreeParamList(ExpParamObjects);
		return flag;
	    }
	else
	    {
            if (!attr) return -1;
            if (attr->Type != ST_T_ATTRIB) return -1;
            if (nval >= attr->nVal) return -1;

            /** String or int val? **/
            if (intval) *intval = attr->IntVal[nval];
            if (strval) *strval = attr->StrVal[nval];

            return 0;

	    }
    return -1;
    }



int 
stpSetAttrValue(pStpInf inf, char* attrname, void* val)
    {
    pStructInf find_inf;
    int type;

        /** Choose the attr name **/
        if (!strcmp(attrname,"name"))
            {
            mssError(1,"STP","Illegal attempt to modify name");
            return -1;
            }
        /** Content-type?  can't set that **/
        if (!strcmp(attrname,"content_type")) 
            {
            mssError(1,"STP","Illegal attempt to modify content type");
            return -1;
            }

        /** Otherwise, try to set top-level attrib inf value in the override struct **/
        /** First, see if the thing exists in the original inf struct. **/
        type = stpAttrType(inf, attrname);
        if (type < 0) return -1;

        /** Now, look for it in the override struct. **/
        find_inf = stpLookup(inf, attrname);

        /** If not found, add a new one **/
        if (!find_inf)
            {
            find_inf = stpAddAttr(inf, attrname);
            find_inf->Type = ST_T_ATTRIB;
            find_inf->StrVal[0] = NULL;
            find_inf->StrAlloc[0] = 0;
            find_inf->IntVal[0] = 0;
            find_inf->nVal = 1;
            }

        /** Set the value. **/
        if (find_inf)
            {
            switch(type)
                {
                case DATA_T_INTEGER:
                    find_inf->IntVal[0] = *(int*)val;
                    break;

                case DATA_T_STRING:
                    if (find_inf->StrAlloc[0]) free(find_inf->StrVal[0]);
                    find_inf->StrAlloc[0]=1;
                    find_inf->StrVal[0] = (char*)malloc(strlen(*(char**)val));
                    strcpy(find_inf->StrVal[0], *(char**)val);
                    break;

                case DATA_T_INTVEC:
                    break;

                case DATA_T_STRINGVEC:
                    break;
                }
            return 0;
            }

    return -1;

    }

/*** stpAttrType **/
/** An evaluated expression might be a different type **/
int stpAttrType(pStpInf inf, char* attrname)
    {
    pStructInf findinf;
	findinf = stpLookup(inf,attrname);
	if (!findinf) return -1;
        if (findinf->nVal == 1 && findinf->StrVal[0] != NULL) return DATA_T_STRING;
        else if (findinf->nVal == 1) return DATA_T_INTEGER;

        /** If more than one value, return string or int vec **/
        if (findinf->nVal > 1 && findinf->StrVal[0] != NULL) return DATA_T_STRINGVEC;
        else if (findinf->nVal > 1) return DATA_T_INTVEC;

	return -1;
    }

/*** stpUpdateNode? **/
/** Update the original node with the override information **/
int stpUpdateNode(pStpInf inf)
	{
	/** Right now this does nothing **/
	return -1;
	}

/*** stpParsePathOverrides **/
int stpParsePathOverride(pStpInf inf, char* pathpart)
	{
	/** Right now this does nothing **/
	return -1;
	}


/*** stpMakeWhereTree ?? **/
int stpMakeWhereTree()
	{
	/** Right now this does nothing **/
	return -1;
	}

/*** stpFreeInf **/
int stpFreeInf(pStpInf inf)
    {
	inf->Node->OpenCnt++;
	inf->Node = NULL;
	/*** unallocate expression trees here! **/

	/** free the override inf **/
	stFreeInf(inf->OverrideInf);
	/** free the inf **/
	nmFree(inf, sizeof(StpInf));
    }


/*** stpSubstParam - performs parameterized substitution on the
 *** given string, returning an allocated string containing the substitution.
 *** If the parameter is an expression, that expression will be placed in
 *** the string.  Expressions evaluating into constants have already been
 *** evaluated and those constants will be inserted into the string, not
 *** the expression.
 ***/
pXString
stpSubstParam(pStpInf inf, char* src)
    {
    pXString dest;
    char* ptr;
    char* ptr2;
    char paramname[64];
    char nbuf[16];
    int n;
    pStructInf param_inf;

        /** Allocate our string. **/
        dest = (pXString)nmMalloc(sizeof(XString));
        if (!dest) return NULL;
        xsInit(dest);

        /** Look for the param subst & sign... **/
        while(ptr = strchr(src,'&'))
            {
            /** Copy all of string up to the & but not including it **/
            xsConcatenate(dest, src, ptr-src);

            /** Now look for the name of the parameter. **/
            ptr2 = ptr+1;
            while((*ptr2 >= 'A' && *ptr2 <= 'Z') || (*ptr2 >= 'a' && *ptr2 <= 'z') || *ptr2 == '_')
                {
                ptr2++;
                }
            n = (ptr2 - ptr) - 1;
            if (n <= 63 && n > 0)
                {
                memcpy(paramname, ptr+1, n);
                paramname[n] = 0;
                }
            else
                {
                xsConcatenate(dest, "&", 1);
                src = ptr+1;
                continue;
                }

	    /** No need to do evaluation here.  Constants already evaluated **/
	    /** Expressions will be placed into the string, if the parameter **/
	    /** evaluates to an expression **/

            /** Ok, got the paramname.  Now look it up. **/
            param_inf = stpLookup(inf, paramname);
            if (!param_inf)
                {
                xsConcatenate(dest, "&", 1);
                src = ptr+1;
                continue;
                }

            /** If string, copy it.  If number, sprintf it then copy it. **/
            if (param_inf->StrVal[0] != NULL)
                {
                xsConcatenate(dest, param_inf->StrVal[0], -1);
                }
            else
                {
                sprintf(nbuf, "%d", param_inf->IntVal[0]);
                xsConcatenate(dest, nbuf, -1);
                }

            /** Ok, skip over the paramname so we don't copy it... **/
            src = ptr2;
            }

        /** Copy the tail of the string **/
        xsConcatenate(dest, src, -1);

    return dest;
    }

