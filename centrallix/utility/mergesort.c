#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "mergesort.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2004 LightSys Technology Services, Inc.		*/
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
/* Module: 	mergesort.c,mergesort.h					*/
/* Author:	Greg Beeley (GRB)  					*/
/* Creation:	June 16, 2010 						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: mergesort.c,v 1.1 2010/09/09 00:39:13 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/mergesort.c,v $

    $Log: mergesort.c,v $
    Revision 1.1  2010/09/09 00:39:13  gbeeley
    - (change) allowing -pg (graph profiler) to be disabled by default
      during compilation.  profile timer had some poor interaction with the
      fork() system call, causing sporadic lockups.
    - (feature) beginnings of "user agent window" widget
    - (change) broke out mergesort() routine into its own independent function
      instead of being a part of the prtmgmt module.


 **END-CVSDATA***********************************************************/


/*** mergesort_r() - Recursive function doing the actual sorting.
 ***/
int
mergesort_r(void** arr1, void** arr2, int cnt, int okleaf, int (*compare_fn)())
    {
    int i,j,found;
    int m,dst;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"MERGESORT","Resource exhaustion occurred");
	    return -1;
	    }

	/** Do a selection sort if cnt <= 32, IF we are starting with the right buffer **/
	if (cnt <= 32 && okleaf)
	    {
	    for(i=0;i<cnt;i++)
		{
		found=0;
		for(j=1;j<cnt;j++)
		    {
		    if (!arr1[found])
			found = j;
		    else if (arr1[j] && compare_fn(arr1[j], arr1[found]) < 0)
			found = j;
		    }
		arr2[i] = arr1[found];
		arr1[found] = NULL;
		}
	    }
	else
	    {
	    /** merge sort 0...m-1 and m...cnt-1 **/
	    /** sort the halves **/
	    m = cnt / 2;
	    if (mergesort_r(arr2, arr1, m, !okleaf, compare_fn) < 0) return -1;
	    if (mergesort_r(arr2+m, arr1+m, cnt-m, !okleaf, compare_fn) < 0) return -1;

	    /** Merge them **/
	    dst = 0;
	    i = 0;
	    j = m;
	    while(i<m || j<cnt)
		{
		if (i==m)
		    arr2[dst++] = arr1[j++];
		else if (j==cnt)
		    arr2[dst++] = arr1[i++];
		else if (compare_fn(arr1[i], arr1[j]) <= 0)
		    arr2[dst++] = arr1[i++];
		else
		    arr2[dst++] = arr1[j++];
		}
	    }

    return 0;
    }


/*** mergesort() - Perform a mergesort sort on the provided array, which is an
 *** array of void*, given a comparison function.  Sorting at the leaf nodes
 *** (once the array size gets down to 16-32 items) is done via a selection
 *** sort.  The mergesort is double-buffering internally.
 ***
 *** compare_fn should take two argments, item1 and item2, and return a value
 *** less than, equal to, or greater than 0, based on whether item1 is less
 *** than, equal to, or greater than item2, respectively.  See qsort(3).
 ***/
int
mergesort(void** arr, int cnt, int (*compare_fn)())
    {
    void** arr2;
    int rval;

	/** Allocate second array **/
	arr2 = (void**)nmSysMalloc(cnt * sizeof(void*));
	if (!arr2) return -1;
	memcpy(arr2, arr, cnt * sizeof(void*));

	/** Sort it.  By setting okleaf=1, arr will contain sorted result.
	 ** We could have reversed arr2 and arr, and set okleaf=0 and then
	 ** omitted the above memcpy(), but that would force the sort to
	 ** always be at least 2-ply, even on small data sets.
	 **/
	rval = mergesort_r(arr2, arr, cnt, 1, compare_fn);
   
	nmSysFree(arr2);

    return rval;
    }

