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
#include "util.h"

/** Tested module. **/
#include "newmalloc.h"

static unsigned int seed_counter = 0;
static char* err_buf;
static unsigned int err_buf_i;
static unsigned int err_buf_size;

static int mock_error_fn(char* error_msg)
    {
    const size_t len = strlen(error_msg) + 1lu;
    
	/** Ensure enough space to store the error. **/
	while (len > err_buf_size - err_buf_i)
	    {
	    err_buf_size *= 2;
	    err_buf = check_ptr(realloc(err_buf, err_buf_size));
	    }
	
	err_buf_i += snprintf(
	    err_buf + err_buf_i,
	    err_buf_size - err_buf_i,
	    "> %s\n", error_msg
	);
    
    return 0;
    }

/** Initialize memory of a given size with random data. **/
static void* random_init(void* ptr, size_t size)
    {
	if (ptr == NULL) return NULL;
	unsigned char* p = (unsigned char*)ptr;
	for (size_t i = 0; i < size; i++) {
	    p[i] = (unsigned char)(rand() % 256);
	}
	return ptr;
    }

static bool do_tests(void)
    {
    bool success = true;
    
	/** Set a consistent, distinct seed for each test iteration. **/
	srand(seed_counter++);
	
	/** Initialize the mock error function. **/
	err_buf = check_ptr(malloc(err_buf_size = 256));
	err_buf_i = snprintf(err_buf, err_buf_size, "%s", "");
	nmSetErrFunction(mock_error_fn);
	
	/** Baseline: Should leak. **/
	success &= EXPECT_NOT_NULL(nmMalloc(42));
	
	/** Basic string data. **/
	char* str1;
	success &= EXPECT_NOT_NULL(str1 = nmMalloc(16));
	snprintf(str1, 16, "ThisIsSomeData!");
	char* str2;
	success &= EXPECT_NOT_NULL(str2 = nmMalloc(32));
	snprintf(str2, 32, "ThisDataIsDifferentStringData.\n");
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");
	
	/** 128 MB random data, varying sizes. **/
	#define TEST_LIMIT 16384
	void** data = check_ptr(malloc(TEST_LIMIT * sizeof(void*)));
	void** test = check_ptr(malloc(TEST_LIMIT * sizeof(void*)));
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    success &= EXPECT_NOT_NULL(test[i] = nmMalloc(i));
	    data[i] = random_init(check_ptr(malloc(i)), i);
	    memcpy(test[i], data[i], i);
	    }
	for (size_t i = TEST_LIMIT - 1lu; i > 0lu; i--)
	    success &= EXPECT_EQL(memcmp(data[i], test[i], i), 0, "%d");
	
	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");
	
	/** Large singular allocation. **/
	#define _256MB 256000000lu
	void* large_buf;
	success &= EXPECT_NOT_NULL(large_buf = nmMalloc(_256MB));
	for (size_t i = _256MB - 1lu; i > 0lu; i--)
	    *((unsigned char*)large_buf + i) = (unsigned char)(i % 255lu);
	*(unsigned char*)large_buf = 0u;
	for (size_t i = 0lu; i < _256MB; i++)
	    success &= EXPECT_EQL(*((unsigned char*)large_buf + i), (unsigned char)(i % 255lu), "%d");
	
	/** Dup string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");
	
	/** Free random data, varying sizes. **/
	for (size_t i = 1lu; i < TEST_LIMIT; i++)
	    {
	    free(data[i]);
	    nmFree(test[i], i);
	    }
	
	/** Basic string data is unharmed. **/
	success &= EXPECT_STR_EQL(str1, "ThisIsSomeData!");
	success &= EXPECT_STR_EQL(str2, "ThisDataIsDifferentStringData.\n");
	
	/** Free data. **/
	nmFree(str1, 16);
	nmFree(str2, 32);
	
	/** Free large allocation. **/
	nmFree(large_buf, _256MB);
	
	/** Clear cache. **/
	nmClear();
	
	/** Debug info. **/
	printf("\n");
	nmStats();
	
	/** Expect no captured errors. **/
	success &= EXPECT_STR_EQL(err_buf, "");
    
    return success;
    }

long long test(char** tname)
    {
    *tname = "newmalloc-01 nmMalloc(), nmFree(), & nmClear()";
    return loop_tests(do_tests);
    }

/** Scope cleanup. **/
#undef TEST_LIMIT
#undef _256MB
