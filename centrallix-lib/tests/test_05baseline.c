/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Base Library                                              */
/*                                                                      */
/* Copyright (C) 2026 LightSys Technology Services, Inc.                */
/*                                                                      */
/* You may use these files and this library under the terms of the      */
/* GNU Lesser General Public License, Version 2.1, contained in the     */
/* included file "COPYING".                                             */
/*                                                                      */
/* Module:      test_05baseline.c                                       */
/* Author:      Israel Fuller                                           */
/* Creation:    October 9th, 2026                                       */
/* Description: Baseline test that runs for a while, then reports no    */
/*              operations, so the driver should mark it skipped.       */
/************************************************************************/

#include <stdbool.h>

long long
test(char** tname)
    {
    *tname = "BASELINE no operations - should skip";
    return 0;
    }
