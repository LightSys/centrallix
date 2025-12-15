/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2005 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_xarray_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 25th, 2025					*/
/* Description:	Test all the functions in the xarray library, except	*/
/* 		xaInit() and xaDeInit() because testing xaNew() and	*/
/* 		xaFree() should cover them, and xaAddItemSorted() and	*/
/* 		xaAddItemSortedInt32().					*/
/************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "util.h"

/** Tested module. **/
#include "xarray.h"

/** Helper used by xaClear/xaClearR to free allocated items and count them. **/
static int test_free(void* p, void* arg)
    {
    if (p == NULL) return 0;
    
    free(p);
    if (arg != NULL) (*(int*)arg)++;
    return 0;
    }

static unsigned int seed_counter = 0;

static bool do_tests(void)
    {
    bool success = true;
    
    /** Create random values using a consistent, distinct seed. **/
    srand(seed_counter++);
    const int value1 = rand();
    const int value2 = rand();
    const int value3 = rand();
    const int value4 = rand();
    const int value5 = rand();
    const int value_not = rand(); /* Probably not used... */
    
    /** Create a new xarray. **/
    pXArray xa = xaNew(2);
    success &= EXPECT_EQL(xa != NULL, true, "%d");
    if (xa == NULL) return false;
    
    /** A new xarray should have 0 items. **/
    success &= EXPECT_EQL(xaCount(xa), 0, "%d");
    
    /** Test adding an item. **/
    int* v1 = malloc(sizeof(int)); *v1 = value1;
    success &= EXPECT_EQL(xaAddItem(xa, v1), 0, "%d");
    success &= EXPECT_EQL(xaCount(xa), 1, "%d");
    success &= EXPECT_EQL(*(int*)xaGetItem(xa, 0), value1, "%d");
    
    /** Test adding another item. **/
    int* v2 = malloc(sizeof(int)); *v2 = value2;
    success &= EXPECT_EQL(xaAddItem(xa, v2), 1, "%d");
    success &= EXPECT_EQL(xaCount(xa), 2, "%d");
    success &= EXPECT_EQL(*(int*)xaGetItem(xa, 1), value2, "%d");
    
    /** Test finding items. **/
    success &= EXPECT_EQL(xaFindItem(xa, v2), 1, "%d");
    success &= EXPECT_EQL(xaFindItem(xa, v1), 0, "%d");
    success &= EXPECT_EQL(xaFindItemR(xa, v2), 1, "%d");
    success &= EXPECT_EQL(xaFindItemR(xa, v1), 0, "%d");
    
    /** Test finding items that don't exist. **/
    int* v_not = malloc(sizeof(int)); *v_not = value_not;
    success &= EXPECT_EQL(xaFindItem(xa, NULL), -1, "%d");
    success &= EXPECT_EQL(xaFindItemR(xa, NULL), -1, "%d");
    success &= EXPECT_EQL(xaFindItem(xa, &v_not), -1, "%d");
    success &= EXPECT_EQL(xaFindItemR(xa, &v_not), -1, "%d");
    free(v_not); v_not = NULL;
    
    /** Insert before index 1. **/
    int* v3 = malloc(sizeof(int)); *v3 = value3;
    success &= EXPECT_EQL(xaInsertBefore(xa, 1, v3), 1, "%d");
    success &= EXPECT_EQL(*(int*)xaGetItem(xa, 1), value3, "%d");
    success &= EXPECT_EQL(xaCount(xa), 3, "%d");
    
    /** Insert after index 2. **/
    int* v4 = malloc(sizeof(int)); *v4 = value4;
    success &= EXPECT_EQL(xaInsertAfter(xa, 2, v4), 3, "%d");
    success &= EXPECT_EQL(*(int*)xaGetItem(xa, 3), value4, "%d");
    success &= EXPECT_EQL(xaCount(xa), 4, "%d");
    
    /** xaSetItem() beyond current end should create NULL gaps. **/
    int* vset = malloc(sizeof(int)); *vset = value5;
    success &= EXPECT_EQL(xaSetItem(xa, 5, vset), 5, "%d");
    success &= EXPECT_EQL(xaCount(xa), 6, "%d");
    success &= EXPECT_EQL(*(int*)xaGetItem(xa, 5), value5, "%d");
    success &= EXPECT_EQL(xaGetItem(xa, 4), NULL, "%p"); /* Check null gap. */
    
    /** Remove an item and ensure that it is gone. **/
    success &= EXPECT_EQL(xaRemoveItem(xa, 2), 0, "%d"); /* Remove original index 2. */
    success &= EXPECT_EQL(xaCount(xa), 5, "%d");
    
    /** Count non-NULL items prior to clearing. **/
    int count_before = xaCount(xa);
    int nonnull = 0;
    for (int i = 0; i < count_before; i++)
        if (xaGetItem(xa, i) != NULL) nonnull++;
    
    /** Clear and verify free was called for each non-NULL item. **/
    int freed_count = 0;
    success &= EXPECT_EQL(xaClear(xa, test_free, &freed_count), 0, "%d");
    success &= EXPECT_EQL(freed_count, nonnull, "%d");
    success &= EXPECT_EQL(xaCount(xa), 0, "%d");
    
    /** Clean up. **/
    success &= EXPECT_EQL(xaFree(xa), 0, "%d");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "xarray-00 Full Test";
    return loop_tests(do_tests);
    }
