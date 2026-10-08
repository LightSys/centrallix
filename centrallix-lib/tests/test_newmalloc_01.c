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
/* Module:	test_newmalloc_01.c					*/
/* Author:	Israel Fuller						*/
/* Creation:	December 15th, 2025					*/
/* Description:	Test the nmMalloc(), nmFree(), and nmClear() functions	*/
/* 		from the NewMalloc library.				*/
/************************************************************************/

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Test dependencies. **/
#include "test_utils.h"

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
    const size_t len = strlen(error_msg) + 4lu; /* "> " + message + "\n" + NUL */

	/** Ensure enough space to store the error. **/
	while (len > err_buf_size - err_buf_i)
	    {
	    char* new_buf = realloc(err_buf, err_buf_size * 2);
	    if (!ASSERT_NOT_NULL(new_buf)) return -1;
	    err_buf = new_buf;
	    err_buf_size *= 2;
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

/** Free the bulk data arrays and the entries allocated so far. **/
static void freeBulk(void** data, void** test)
    {
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    if (data != NULL) free(data[i]);
	    if (test != NULL && test[i] != NULL) nmFree(test[i], i);
	    }
	free(data);
	free(test);
    }

static bool doTest(void)
    {
    bool success = true;
    bool finished = false;
    char* str1 = NULL;
    char* str2 = NULL;
    void** data = NULL;
    void** test = NULL;
    void* large_buf = NULL;

	/** Set a consistent, distinct seed for each test iteration. **/
	srand(seed_counter++);

	/** Initialize the mock error function. **/
	err_buf = malloc(err_buf_size = 256);
	if (!ASSERT_NOT_NULL(err_buf)) goto end;
	err_buf_i = snprintf(err_buf, err_buf_size, "%s", "");
	nmSetErrFunction(mockErrorFn);

	/** Basic string data. **/
	if (!ASSERT_NOT_NULL(str1 = nmMalloc(16))) goto end;
	snprintf(str1, 16, "ThisIsSomeData!");
	if (!ASSERT_NOT_NULL(str2 = nmMalloc(32))) goto end;
	snprintf(str2, 32, "ThisDataIsDifferentStringData.\n");
	success &= ASSERT_STR_EQL(str1, "ThisIsSomeData!");
	success &= ASSERT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Random data, varying sizes. **/
	if (!ASSERT_NOT_NULL(data = calloc(TEST_LIMIT, sizeof(void*)))) goto end;
	if (!ASSERT_NOT_NULL(test = calloc(TEST_LIMIT, sizeof(void*)))) goto end;
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    if (!ASSERT_NOT_NULL(test[i] = nmMalloc(i))) goto end;
	    if (!ASSERT_NOT_NULL(data[i] = randomInit(malloc(i), i))) goto end;
	    memcpy(test[i], data[i], i);
	    }
	for (size_t i = TEST_LIMIT - 1lu; i > 0lu; i--)
	    success &= ASSERT_EQL(memcmp(data[i], test[i], i), 0, "%d");

	/** Basic string data is unharmed. **/
	success &= ASSERT_STR_EQL(str1, "ThisIsSomeData!");
	success &= ASSERT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Large singular allocation. **/
	if (!ASSERT_NOT_NULL(large_buf = nmMalloc(LARGE_BUF_SIZE))) goto end;
	for (size_t i = LARGE_BUF_SIZE - 1lu; i > 0lu; i--)
	    *((unsigned char*)large_buf + i) = (unsigned char)(i % 255lu);
	*(unsigned char*)large_buf = 0u;
	size_t mismatches = 0lu;
	for (size_t i = 0lu; i < LARGE_BUF_SIZE; i++)
	    if (*((unsigned char*)large_buf + i) != (unsigned char)(i % 255lu)) mismatches++;
	success &= ASSERT_EQL(mismatches, 0lu, "%zu");

	/** Basic string data is unharmed. **/
	success &= ASSERT_STR_EQL(str1, "ThisIsSomeData!");
	success &= ASSERT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Free random data, varying sizes. **/
	freeBulk(data, test);
	data = NULL;
	test = NULL;

	/** Basic string data is unharmed. **/
	success &= ASSERT_STR_EQL(str1, "ThisIsSomeData!");
	success &= ASSERT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");

	/** Free data. **/
	nmFree(str1, 16);
	str1 = NULL;
	nmFree(str2, 32);
	str2 = NULL;

	/** Free large allocation. **/
	nmFree(large_buf, LARGE_BUF_SIZE);
	large_buf = NULL;

	/** Clear cache. **/
	nmClear();

	/** Expect no captured errors. **/
	success &= ASSERT_STR_EQL(err_buf, "");

	finished = true;

    end:
	/** Clean up whatever is still allocated. **/
	if (large_buf != NULL) nmFree(large_buf, LARGE_BUF_SIZE);
	freeBulk(data, test);
	if (str2 != NULL) nmFree(str2, 32);
	if (str1 != NULL) nmFree(str1, 16);

	/** Unregister the mock error function before freeing its buffer. **/
	nmSetErrFunction(NULL);
	free(err_buf);
	err_buf = NULL;

    return finished && success;
    }

long long test(char** tname)
    {
    *tname = "newmalloc-01 nmMalloc(), nmFree(), & nmClear()";
    return loopTest(doTest) * ((long long)TEST_LIMIT + 3ll);
    }

/** Scope cleanup. **/
#undef BULK_DIVISOR
#undef TEST_LIMIT
#undef LARGE_BUF_SIZE
