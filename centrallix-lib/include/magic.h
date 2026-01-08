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



#ifdef 	DBMAGIC
#include <stdlib.h>

#define ASSERTMAGIC(data, expect) \
    ({ \
    pMagicHdr _data = (pMagicHdr)(data); \
    Magic_t _expect = (expect); \
    Magic_t _actual = (_data == NULL) ? 0xEE1EE100 : _data->Magic; \
    if (_data != NULL && _actual != _expect) \
	{ \
	printf( \
	    "%s:%d: Magic assertion failed, unexpected %u != %d for %8.8lX.\n", \
	    __FILE__, __LINE__, _actual, _expect, (long)_data \
	); \
	abort(); \
	} \
    0; \
    })

#define ASSERTNOTMAGIC(data, expect) \
    ({ \
    pMagicHdr _data = (pMagicHdr)(data); \
    Magic_t _expect = (expect); \
    Magic_t _actual = (_data == NULL) ? 0xEE1EE100 : _data->Magic; \
    if (_data != NULL && _actual == _expect) \
	{ \
	printf( \
	    "%s:%d: Magic assertion failed, unexpected %d.\n", \
	    __FILE__, __LINE__, _expect \
	); \
	abort(); \
	} \
    0; \
    })

#else	/* defined DBMAGIC */

#define ASSERTMAGIC(x,y) ((void)(y))
#define ASSERTNOTMAGIC(x,y) ((void)(y))

#endif	/* defined DBMAGIC */

#define ISMAGIC(data, expect)   (((pMagicHdr)(data))->Magic == (expect))
#define ISNTMAGIC(data, expect) (((pMagicHdr)(data))->Magic != (expect))
#define SETMAGIC(data, expect)  (((pMagicHdr)(data))->Magic = (expect))

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
#define MGK_STNODE	0x12340d21	/* st_node.h::SnNode */

#define MGK_MEMSTART	0x12340809	/* newmalloc.c::MemStruct */
#define MGK_MEMEND	0x12340908	/* newmalloc.c::MemStruct */

#define MGK_XSTRING	0x12340Ab8	/* xstring.h::XString */
#define MGK_NNFS	0x12340b01	/* net_nfs.c */

#define MGK_WGTR	0x12340c32	/* wgtr.c::WgtrNode */

#define MGK_SMREGION	0x1200345c	/* smmalloc.h::SmRegion */
#define MGK_SMBLOCK	0x1200349a	/* smmalloc_private.h::SmBlock */

#endif /* not defined _MAGIC_H */
