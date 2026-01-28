#ifndef DOUBLE_METAPHONE_H
#define	DOUBLE_METAPHONE_H

/************************************************************************/
/* Text-DoubleMetaphone							*/
/* Centrallix Base Library						*/
/* 									*/
/* Copyright 2000, Maurice Aubrey <maurice@hevanet.com>.		*/
/* All rights reserved.							*/
/* 									*/
/* This code is copied for redistribution with modification, from the	*/
/* gitpan/Text-DoubleMetaphone implementation on GitHub (1), which is	*/
/* under the following license.						*/
/* 									*/
/*    This code is based heavily on the C++ implementation by Lawrence	*/
/*    Philips and incorporates several bug fixes courtesy of Kevin	*/
/*    Atkinson <kevina@users.sourceforge.net>.				*/
/* 									*/
/*    This module is free software; you may redistribute it and/or	*/
/*    modify it under the same terms as Perl itself.			*/
/* 									*/
/* A summary of the relevant content from https://dev.perl.org/licenses	*/
/* has been included below for the convenience of the reader. This	*/
/* information was collected and saved on September 5th, 2025 and may	*/
/* differ from current information. For the most up to date copy of	*/
/* this information, please use  the link provided above.		*/
/* 									*/
/*    Perl5 is Copyright © 1993 and later, by Larry Wall and others.	*/
/* 									*/
/*    It is free software; you can redistribute it and/or modify it	*/
/*    under the terms of either:					*/
/* 									*/
/*    a) the GNU General Public License (2) as published by the Free	*/
/*	 Software Foundation (3); either version 1 (2), or (at your	*/
/*	 option) any later version (4), or				*/
/* 									*/
/*    b) the "Artistic License" (5).					*/
/* 									*/
/* Citations:								*/
/*    1: https://github.com/gitpan/Text-meta_double_metaphone		*/
/*    2: https://dev.perl.org/licenses/gpl1.html			*/
/*    3: http://www.fsf.org						*/
/*    4: http://www.fsf.org/licenses/licenses.html#GNUGPL		*/
/*    5: https://dev.perl.org/licenses/artistic.html			*/
/* 									*/
/* Centrallix is published under the GNU General Public License,	*/
/* satisfying the above requirement. A summary of this is included	*/
/* below for the convenience of the reader.				*/
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
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA		*/
/* 02111-1307  USA							*/
/* 									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	double_metaphone.c, double_metaphone.h			*/
/* Author:	Maurice Aubrey and Israel Fuller			*/
/* Description:	This module implements a "sounds like" algorithm by	*/
/* 		Lawrence Philips which he published in the June, 2000	*/
/* 		issue of C/C++ Users Journal. Double Metaphone is an	*/
/* 		improved version of the original Metaphone algorithm	*/
/* 		written by Philips'. This implementaton was written by	*/
/* 		Maurice Aubrey for C/C++ with bug fixes provided by	*/
/* 		Kevin Atkinson. It was revised by Israel Fuller to	*/
/* 		better align with the Centrallix coding style and	*/
/* 		standards so that it could be included here.		*/
/************************************************************************/

void meta_double_metaphone(const char* str, char** primary_code, char** secondary_code);

#endif /* End of .h file. */
