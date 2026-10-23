#ifndef _EXPRESSION_PRIVATE_H
#define _EXPRESSION_PRIVATE_H

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
/* Module:	expression_private.h, exp_*.c				*/
/* Author:	Israel Fuller						*/
/* Date:	September 2nd, 2026					*/
/* 									*/
/* Description:	Internal declarations shared between the expression	*/
/* 		sources.  These are not a part of the public		*/
/* 		expression interface.					*/
/************************************************************************/


#include "expression.h"


/*** Internal functions ***/
pExpression exp_internal_CompileExpression_r(pLxSession lxs, int level, pParamObjects objlist, int cmpflags);
int exp_internal_CopyNode(pExpression src, pExpression dst);
pExpression exp_internal_CopyTree(pExpression orig_exp);
int exp_internal_EvalTree(pExpression tree, pParamObjects objlist);
int exp_internal_EvalAggregates(pExpression tree, pParamObjects objlist);
int exp_internal_ResetAggregates(pExpression tree, int reset_id, int level);
int exp_internal_DefineFunctions();
int exp_internal_DefineNodeEvals();
int exp_internal_SetupControl(pExpression exp);
pExpControl exp_internal_LinkControl(pExpControl ctl);
int exp_internal_UnlinkControl(pExpControl ctl);

#endif /* not defined _EXPRESSION_PRIVATE_H */
