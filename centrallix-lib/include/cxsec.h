#ifndef _CXSEC_H
#define _CXSEC_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright (C) 2003 LightSys Technology Services, Inc.		*/
/* 									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/* 									*/
/* Module: 	cxsec.h                 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	March 31, 2003    					*/
/* Description:	This module implements some security hardening features	*/
/*		that are optional for application functioning.		*/
/************************************************************************/

extern unsigned long cxsec_key;

void cxsecCheckCanary(unsigned long* ck, unsigned long c, char* filename, int linenum);
void cxsecInitDS(unsigned long* start, unsigned long* end);
void cxsecVerifyDS(unsigned long* start, unsigned long* end, char* filename, int linenum);
void cxsecUpdateDS(unsigned long* start, unsigned long* end, char* filename, int linenum);
void cxsecInitialize();

int cxsecVerifySymbol(char* sym);

#ifndef __GNUC__
#define __attribute__(a) /* hide function attributes from non-GCC compilers */
#endif

#ifdef CXLIB_SECH

/*** function entry/exit dynamic stack canary checking.  We put the 
 *** tweetie in an array to try to keep the compiler from optimizing
 *** it out or optimizing it into a register
 ***/
#define	CXSEC_ENTRY(x)	auto unsigned long cx__canary[2] = {(x),(x)}
#define CXSEC_EXIT(x)	cxsecCheckCanary(cx__canary, (unsigned long)(x), __FILE__, __LINE__)

/*** data structure in-memory verification
 ***/
#define CXSEC_DS_BEGIN	unsigned long cx__dsrandom
#define CXSEC_DS_END	unsigned long cx__dsverify[2]
#define CXSEC_INIT(x)	cxsecInitDS(&((x).cx__dsrandom), &((x).cx__dsverify[1]))
#define CXSEC_VERIFY(x)	cxsecVerifyDS(&((x).cx__dsrandom), &((x).cx__dsverify[1]), __FILE__, __LINE__)
#define CXSEC_UPDATE(x)	cxsecUpdateDS(&((x).cx__dsrandom), &((x).cx__dsverify[1]), __FILE__, __LINE__)

#else /* defined CXLIB_SECH */

#define CXSEC_ENTRY(x)	unsigned long cx__canary[2] __attribute__ ((unused))
#define CXSEC_EXIT(x)	
#define CXSEC_DS_BEGIN	unsigned long cx__dsrandom
#define CXSEC_DS_END	unsigned long cx__dsverify[2]
#define CXSEC_INIT(x)	
#define CXSEC_VERIFY(x)	
#define CXSEC_UPDATE(x)	

#endif /* defined CXLIB_SECH */

#endif /* not defined _CXSEC_H */
