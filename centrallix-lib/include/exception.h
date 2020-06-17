#ifndef _EXCEPTION_H
#define _EXCEPTION_H

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
/* Module: 	exception.h          					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	A set of defines that help make exception handling 	*/
/*		inside a function a little bit easier.  Uses setjmp	*/
/*		and longjmp.  Syntax below...				*/
/*									*/
/*		Exception ex;						*/
/*		....							*/
/*									*/
/*		// Enable exception handling of this exception		*/
/*		Catch(ex)						*/
/*		    {							*/
/*		    printf("We got an error!!!\n");			*/
/*		    return -1;						*/
/*		    }							*/
/*		....							*/
/*									*/
/*		if (error_condition) Throw(ex);				*/
/************************************************************************/



#include "setjmp.h"

typedef jmp_buf	Exception;

#define Catch(ex)  if (setjmp(ex))
#define Throw(ex)  longjmp((ex),1)

#endif /* _EXCEPTION_H */
