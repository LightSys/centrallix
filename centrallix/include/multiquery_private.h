#ifndef _MULTIQUERY_PRIVATE_H
#define _MULTIQUERY_PRIVATE_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2026 LightSys Technology Services, Inc.		*/
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
/* Module:	multiquery_private.h, multiq_*.c			*/
/* Author:	Israel Fuller						*/
/* Date:	September 2nd, 2026					*/
/* 									*/
/* Description:	Internal declarations shared between the multiquery	*/
/* 		sources.  These are not a part of the public		*/
/* 		multiquery interface.					*/
/************************************************************************/


#include "multiquery.h"


/*** Internal functions ***/
char* mq_internal_QEGetNextAttr(pMultiQuery mq, pQueryElement qe, pParamObjects objlist, int* attrid, int* astobjid);
int mq_internal_FreeQS(pQueryStructure qstree);
pQueryStructure mq_internal_AllocQS(int type);
pQueryStructure mq_internal_FindItem(pQueryStructure tree, int type, pQueryStructure next);
pQueryElement mq_internal_AllocQE();
int mq_internal_FreeQE(pQueryElement qe);
int mq_internal_AddOrderBy(pQueryElement qe, pExpression exp);
int mq_internal_ClearOrderBy(pQueryElement qe);
int mq_internal_nOrderBy(pQueryElement qe);
pPseudoObject mq_internal_CreatePseudoObject(pMultiQuery qy, pObject hl_obj);
int mq_internal_FreePseudoObject(pPseudoObject p);
int mq_internal_EvalHavingClause(pQueryStatement stmt, pPseudoObject p);
handle_t mq_internal_FindCollection(pMultiQuery mq, char* collection);
void mq_internal_CheckYield(pMultiQuery mq);

#endif  /* not defined _MULTIQUERY_PRIVATE_H */
