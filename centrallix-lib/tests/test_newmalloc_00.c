/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2025-2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_newmalloc_00.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	November 25th, 2025					*/
/* Description:	Test the nmSysMalloc(), nmSysFree(), nmSysRealloc(),	*/
/* 		and nmSysStrDup functions from the NewMalloc library.	*/
/************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"
#include "check.h"
#include "range.h"

/** Tested module. **/
#include "newmalloc.h"

/*** Valgrind instruments every memory access, so the bulk data sizes are
 *** divided by this factor when running under it, keeping the test inside
 *** the driver's lockup timeout.
 ***/
#ifdef USING_VALGRIND
#include "valgrind/valgrind.h"
#define BULK_DIVISOR	(RUNNING_ON_VALGRIND ? 16lu : 1lu)
#else
#define BULK_DIVISOR	1lu
#endif

#define TEST_LIMIT	(16384lu / BULK_DIVISOR)
#define LARGE_BUF_SIZE	(256000000lu / BULK_DIVISOR)

static unsigned int seed_counter = 0;
static char* err_buf;
static unsigned int err_buf_i;
static unsigned int err_buf_size;

static int mockErrorFn(char* error_msg)
    {
    const size_t len = strlen(error_msg) + 1lu;

	/** Ensure enough space to store the error. **/
	while (len > err_buf_size - err_buf_i)
	    {
	    err_buf_size *= 2;
	    err_buf = checkPtr(realloc(err_buf, err_buf_size));
	    }

	err_buf_i += snprintf(
	    err_buf + err_buf_i,
	    err_buf_size - err_buf_i,
	    "> %s\n", error_msg
	);

    return 0;
    }

/** Initialize memory of a given size with random data. **/
static void* randomInit(void* ptr, size_t size)
    {
	if (ptr == NULL) return NULL;
	unsigned char* p = (unsigned char*)ptr;
	for (size_t i = 0; i < size; i++) {
	    p[i] = (unsigned char)(rand() % 256);
	}
	return ptr;
    }

static bool doTest(void)
    {
    bool success = true;

	/** Set a consistent, distinct seed for each test iteration. **/
	srand(seed_counter++);

	/** Initialize the mock error function. **/
	err_buf = checkPtr(malloc(err_buf_size = 256));
	err_buf_i = snprintf(err_buf, err_buf_size, "%s", "");
	nmSetErrFunction(mockErrorFn);

	/** Basic string data. **/
	char* str1;
	success &= EXPECT_NOT_NULL(str1 = nmSysMalloc(16));
	snprintf(str1, 16, "ThisIsSomeData!");
	char* str2;
	success &= EXPECT_NOT_NULL(str2 = nmSysMalloc(32));
	snprintf(str2, 32, "ThisDataIsDifferentStringData.\n");
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Random data, varying sizes. **/
	void** data = checkPtr(malloc(TEST_LIMIT * sizeof(void*)));
	void** test = checkPtr(malloc(TEST_LIMIT * sizeof(void*)));
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    success &= EXPECT_NOT_NULL(test[i] = nmSysMalloc(i));
	    data[i] = randomInit(checkPtr(malloc(i)), i);
	    memcpy(test[i], data[i], i); /* Write test data into test memory. */
	    }
	for (size_t i = TEST_LIMIT - 1lu; i > 0lu; i--)
	    success &= EXPECT_EQL(memcmp(data[i], test[i], i), 0, "%d");

	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Reallocate all variably sized memory to a different size. **/
	for (size_t i = TEST_LIMIT - 1lu; i > 0lu; i--)
	    success &= EXPECT_NOT_NULL(test[i] = nmSysRealloc(test[i], TEST_LIMIT - i));
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    success &= EXPECT_EQL(memcmp(data[i], test[i], min(i, TEST_LIMIT - i)), 0, "%d");

	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Testing strdup. **/
	char* str_dup1;
	char* str_dup2;
	success &= EXPECT_NOT_NULL(str_dup1 = nmSysStrdup(str1));
	success &= EXPECT_NOT_NULL(str_dup2 = nmSysStrdup(str2));
	success &= EXPECT_STR_EQL(str_dup1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str_dup2, "ThisDataIsDifferentStringData.\n");
	str_dup1[12] = '\0';
	str_dup2[2] = 'a';
	str_dup2[3] = 't';
	success &= EXPECT_STR_EQL(str_dup1, "ThisIsSomeDa");
	success &= EXPECT_STR_EQL(str_dup2, "ThatDataIsDifferentStringData.\n");

	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Free random data, varying sizes. **/
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    free(data[i]);
	    nmSysFree(test[i]);
	    }
	free(data);
	free(test);

	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Free data. **/
	nmSysFree(str1);
	nmSysFree(str2);

	/** Dup string data is unharmed. **/
	success &= EXPECT_STR_EQL(str_dup1, "ThisIsSomeDa");
	success &= EXPECT_STR_EQL(str_dup2, "ThatDataIsDifferentStringData.\n");

	/** Large singular allocation. **/
	void* large_buf;
	success &= EXPECT_NOT_NULL(large_buf = nmSysMalloc(LARGE_BUF_SIZE));
	for (size_t i = LARGE_BUF_SIZE - 1lu; i > 0lu; i--)
	    *((unsigned char*)large_buf + i) = (unsigned char)(i % 255lu);
	*(unsigned char*)large_buf = 0u;
	size_t mismatches = 0lu;
	for (size_t i = 0lu; i < LARGE_BUF_SIZE; i++)
	    if (*((unsigned char*)large_buf + i) != (unsigned char)(i % 255lu)) mismatches++;
	success &= EXPECT_EQL(mismatches, 0lu, "%zu");

	/** Dup string data is unharmed. **/
	success &= EXPECT_STR_EQL(str_dup1, "ThisIsSomeDa");
	success &= EXPECT_STR_EQL(str_dup2, "ThatDataIsDifferentStringData.\n");

	/** Free dups. **/
	nmSysFree(str_dup1);
	nmSysFree(str_dup2);

	/** Free large allocation. **/
	nmSysFree(large_buf);

	/** Expect no captured errors. **/
	success &= EXPECT_STR_EQL(err_buf, "");

	/** Clean up. **/
	free(err_buf);

    return success;
    }

long long test(char** tname)
    {
    *tname = "newmalloc-00 nmSysMalloc(), nmSysFree(), nmSysRealloc(), & nmSysStrdup()";
    return loopTest(doTest) * ((long long)TEST_LIMIT + 3ll);
    }

/** Scope cleanup. **/
#undef BULK_DIVISOR
#undef TEST_LIMIT
#undef LARGE_BUF_SIZE
