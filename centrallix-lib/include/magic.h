#ifndef _MAGIC_H
#define _MAGIC_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	magic.h			 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	November 26, 1999					*/
/* Description:	A list of structure magic numbers for specific use in	*/
/*		debug build releases.  The magic numbers can also be	*/
/*		used in non-debug build releases to identify the	*/
/*		structure in question.					*/
/*		The assertion routines, if DBMAGIC is set, are designed	*/
/*		to cause an immediate segfault in the calling thread	*/
/*		on assertion failure.  This will cmd-line the debugger	*/
/*		or transfer control to MTASK's segfault handler.	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: magic.h,v 1.1 2001/08/13 18:04:19 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/magic.h,v $

    $Log: magic.h,v $
    Revision 1.1  2001/08/13 18:04:19  gbeeley
    Initial revision

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#ifdef 	DBMAGIC

#define ASSERTMAGIC(x,y) ((!(x) || (((pMagicHdr)(x))->Magic == (y)))?0:(printf("LS-PANIC: Magic number assertion failed, unexpected %X != %X\n",(x)?(((pMagicHdr)(x))->Magic):(0xEE1EE100),(y)),(*((int*)(0)) = *((int*)(0)))));
#define ASSERTNOTMAGIC(x,y) ((!(x) || (((pMagicHdr)(x))->Magic != (y)))?0:(printf("LS-PANIC: Magic number assertion failed, unexpected %X\n",(y)),(*((int*)(0)) = *((int*)(0)))));
#define ISMAGIC(x,y) (((pMagicHdr)(x))->Magic == (y))
#define ISNTMAGIC(x,y) (((pMagicHdr)(x))->Magic != (y))
#define SETMAGIC(x,y) (((pMagicHdr)(x))->Magic = (y))

#else	/* defined DBMAGIC */

#define ASSERTMAGIC(x,y)
#define ASSERTNOTMAGIC(x,y)
#define ISMAGIC(x,y) (1)
#define ISNTMAGIC(x,y) (1)
#define SETMAGIC(x,y) (y)

#endif	/* defined DBMAGIC */

typedef struct
    {
    int	Magic;
    }
    MagicHdr, *pMagicHdr;

#define	MGK_FILE	0x12340001	/* mtask.h::File */
#define MGK_OBJECT	0x12340102	/* obj.h::Object */
#define MGK_LXSESSION	0x12340203	/* mtlexer.h::LxSession */
#define MGK_FREEMEM	0x12340304	/* newmalloc.c::Overlay (in)*/
#define MGK_ALLOCMEM	0x1234031f	/* newmalloc.c::Overlay (out,uninitialized)*/
#define MGK_REGISBLK	0x1234032e	/* newmalloc.c::RegisteredBlockType */
#define MGK_EXPRESSION	0x12340405	/* expression.h::Expression */
#define MGK_PRTOBJSTRM	0x12340502	/* prtmgmt_new.h::PrtObjStream */
#define MGK_PRTOBJSSN	0x1234058e	/* prtmgmt_new.h::PrtSession */


#endif /* not defined _MAGIC_H */
