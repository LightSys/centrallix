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

    $Id: magic.h,v 1.11 2004/08/27 01:28:33 jorupp Exp $
    $Source: /srv/bld/centrallix-repo/centrallix-lib/include/magic.h,v $

    $Log: magic.h,v $
    Revision 1.11  2004/08/27 01:28:33  jorupp
     * cleaning up some compile warnings

    Revision 1.10  2004/07/22 00:20:52  mmcgill
    Added a magic number define for WgtrNode, and added xaInsertBefore and
    xaInsertAfter functions to the XArray module.

    Revision 1.9  2003/05/30 17:40:42  gbeeley
    - make magic number assertion tell us the bad pointer address

    Revision 1.8  2003/04/16 03:26:38  nehresma
    oops.  forgot to commit the update to magic.h to go along with the nfs CXSEC stuff

    Revision 1.7  2003/04/03 04:32:39  gbeeley
    Added new cxsec module which implements some optional-use security
    hardening measures designed to protect data structures and stack
    return addresses.  Updated build process to have hardening and
    optimization options.  Fixed some build-related dependency checking
    problems.  Updated mtask to put some variables in registers even
    when not optimizing with -O.  Added some security hardening features
    to xstring as an example.

    Revision 1.6  2003/03/04 06:28:22  jorupp
     * added buffer overflow checking to newmalloc
    	-- define BUFFER_OVERFLOW_CHECKING in newmalloc.c to enable

    Revision 1.5  2002/04/25 17:56:54  gbeeley
    Added Handle support (xhandle module, XHN).  This is used to provide a
    more flexible abstraction between API return values (handles vs. ptrs)
    and the underlying structures they actually reference.  Handles are
    64bit on glibc2 ia32 platforms (unsigned long long int).

    Revision 1.4  2002/03/23 06:25:09  gbeeley
    Updated MSS to have a larger error string buffer, as a lot of errors
    were getting chopped off.  Added BDQS protocol files with some very
    simple initial implementation.

    Revision 1.3  2001/12/28 21:53:25  gbeeley
    Added some support for the new printmanagement system.

    Revision 1.2  2001/09/28 20:00:18  gbeeley
    Modified magic number system syntax slightly to eliminate semicolon
    from within the macro expansions of the ASSERT macros.

    Revision 1.1.1.1  2001/08/13 18:04:19  gbeeley
    Centrallix Library initial import

    Revision 1.1.1.1  2001/07/03 01:03:01  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


#ifdef 	DBMAGIC

#define ASSERTMAGIC(x,y) ((!(x) || (((pMagicHdr)(x))->Magic == (y)))?0:(printf("LS-PANIC: Magic number assertion failed, unexpected %X != %X for %8.8X\n",(x)?(((pMagicHdr)(x))->Magic):(0xEE1EE100),(y),(int)(x)),(*((int*)(8)) = *((int*)(0)))))
#define ASSERTNOTMAGIC(x,y) ((!(x) || (((pMagicHdr)(x))->Magic != (y)))?0:(printf("LS-PANIC: Magic number assertion failed, unexpected %X\n",(y)),(*((int*)(8)) = *((int*)(0)))))

#else	/* defined DBMAGIC */

#define ASSERTMAGIC(x,y) ((void)(y))
#define ASSERTNOTMAGIC(x,y) ((void)(y))

#endif	/* defined DBMAGIC */

#define ISMAGIC(x,y) (((pMagicHdr)(x))->Magic == (y))
#define ISNTMAGIC(x,y) (((pMagicHdr)(x))->Magic != (y))
#define SETMAGIC(x,y) (((pMagicHdr)(x))->Magic = (y))

typedef int Magic_t;

typedef struct
    {
    Magic_t	Magic;
    }
    MagicHdr, *pMagicHdr;

#define	MGK_FILE	0x12340001	/* mtask.h::File */
#define MGK_OBJECT	0x12340102	/* obj.h::Object */
#define MGK_OBJQUERY	0x1234015a	/* obj.h::ObjQuery */
#define MGK_OBJSESSION	0x1234019b	/* obj.h::ObjSession */
#define MGK_LXSESSION	0x12340203	/* mtlexer.h::LxSession */
#define MGK_FREEMEM	0x12340304	/* newmalloc.c::Overlay (in)*/
#define MGK_ALLOCMEM	0x1234031f	/* newmalloc.c::Overlay (out,uninitialized)*/
#define MGK_REGISBLK	0x1234032e	/* newmalloc.c::RegisteredBlockType */
#define MGK_EXPRESSION	0x12340405	/* expression.h::Expression */
#define MGK_PRTOBJSTRM	0x12340502	/* prtmgmt_v3.h::PrtObjStream */
#define MGK_PRTOBJSSN	0x1234058e	/* prtmgmt_v3.h::PrtSession */
#define MGK_PRTOBJTYPE	0x123405cc	/* prtmgmt_v3.h::PrtObjType */
#define MGK_PRTLM     	0x1234055a	/* prtmgmt_v3.h::PrtLayoutMgr */
#define MGK_PRTHANDLE 	0x12340531	/* prtmgmt_v3.h::PrtHandle */
#define MGK_PRTFMTR 	0x12340593	/* prtmgmt_v3.h::PrtHandle */
#define MGK_PRTOUTDRV 	0x123405a4	/* prtmgmt_v3.h::PrtHandle */
#define MGK_STRUCTINF	0x123406fd	/* stparse_new.h::StructInf */
#define MGK_HANDLE	0x12340707	/* xhandle.h::HandleData */

#define MGK_MEMSTART	0x12340809	/* newmalloc.c::MemStruct */
#define MGK_MEMEND	0x12340908	/* newmalloc.c::MemStruct */

#define MGK_XSTRING	0x12340Ab8	/* xstring.h::XString */
#define MGK_NNFS	0x12340b01	/* net_nfs.c */

#define MGK_WGTR	0x12340c32	/* wgtr.c::WgtrNode */
#endif /* not defined _MAGIC_H */
