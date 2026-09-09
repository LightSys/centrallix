#ifndef TEST_QPRINTF_H
#define	TEST_QPRINTF_H

/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2005-2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_utils.h						*/
/* Author:	Israel Fuller						*/
/* Creation:	April 10th, 2026					*/
/* Description:	Code shared between test files when testing qprintf().	*/
/************************************************************************/

#include <stdbool.h>

#include "qprintf.h"
#include "test_utils.h"

#define ALL_SPECS "\t%% %& %STR %INT %POS %DBL %CHR %LL\t"
#define ALL_SPECS_VALUES "test", -4, 4, 4.2, 'E', (__UINT32_MAX__ + 1ll)
#define ALL_SPECS_RESULT "\t% & test -4 4 4.200000 E 4294967296\t"
#define ALL_SPECS_RESULT_LEN (sizeof(ALL_SPECS_RESULT))

#define EXPECT_NO_ERRORS(s) \
    ({ \
    pQPSession _s = (s); \
    const bool success = EXPECT_EQL(qpfErrors(_s), QPF_ERR_T_NO_ERRORS, "%d"); \
    if (!success) qpfLogErrors(_s); \
    success; \
    })


#endif
