#ifndef _OBFUSCATE_H
#define _OBFUSCATE_H

#include "obj.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2013 LightSys Technology Services, Inc.		*/
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
/* Module: 	obfuscate.c, obfuscate.h				*/
/* Author:	Greg Beeley (GRB)  					*/
/* Creation:	26-Nov-2013						*/
/* Description:	This is the data obfuscation module, which provides	*/
/*		repeatable and non-repeatable obfuscation of data for	*/
/*		testing and demos.					*/
/************************************************************************/


/*** Structure for a wordlist entry ***/
typedef struct _OBFWORD
    {
    struct _OBFWORD*	Next;
    char*		Category;
    char*		Word;
    float		Probability;
    }
    ObfWord, *pObfWord;


/*** Structure for a wordlist category ***/
typedef struct _OBFCAT
    {
    struct _OBFCAT*	Next;
    char*		Category;
    pObfWord		CatStart;
    int			WordCnt;
    float		TotalProb;
    }
    ObfWordCat, *pObfWordCat;


/*** Structure for a rule ***/
typedef struct _OBFRULE
    {
    struct _OBFRULE*	Next;
    char*		Attrname;
    char*		Datatype;
    char*		Typename;
    char*		Objname;
    char*		ValueString;
    unsigned int	HashKey:1;
    unsigned int	HashAttr:1;
    unsigned int	HashName:1;
    unsigned int	HashType:1;
    unsigned int	HashValue:1;
    unsigned int	NoObfuscate:1;
    unsigned int	EquivAttr:1;
    char*		Param;
    pObfWord		Wordlist;
    pObfWordCat		Catlist;
    }
    ObfRule, *pObfRule;


/*** Structure for a session ***/
typedef struct
    {
    char		RuleFilePath[OBJSYS_MAX_PATH];
    pObfRule		RuleList;
    char*		Key;
    }
    ObfSession, *pObfSession;


/*** For obfuscation when no session is active ***/
int obfObfuscateData(pObjData srcval, pObjData dstval, int data_type, char* attrname, char* objname, char* type_name, char* key, char* which, char* param, pObfWord wordlist, pObfWordCat catlist);

/*** Session-oriented functions ***/
pObfSession obfOpenSession(pObjSession objsess, char* rule_file_osml_path, char* key);
int obfCloseSession(pObfSession sess);
int obfObfuscateDataSess(pObfSession sess, pObjData srcval, pObjData dstval, int data_type, char* attrname, char* objname, char* type_name);

#endif /* not defined _OBFUSCATE_H */
