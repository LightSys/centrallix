#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xhash.h"
#include <assert.h>

#define TEST_LENGTH 10

/*** xhi_internal_MatchKey - matches a given key with the corresponding content
 *** for the XHashTable iterator test.
 ***/
char*
xhi_internal_MatchKey(char* key)
    {
	switch(key[3])
	    {
	    case '2':
		return "contents2";
	    case '3':
		return "contents3";
	    case '4':
		return "contents4";
	    case '5':
		return "contents5";
	    case '6':
		return "contents6";
	    case '7':
		return "contents7";
	    case '8':
		return "contents8";
	    case '9':
		return "contents9";
	    case '1':
		return "contents1";
	    }
    }

long long
test(char** tname)
    {
    int i;
    int iter;
    pXHashEntry testEntry;
    XHashTable testHash;

	*tname = "xhashtable-00 ITERATOR test";

	iter = 60000;

	for (i = 0; i < iter; i++)
	    {
	    /** Initialize the test array. **/
	    xhInit(&testHash, 7, 4);

	    xhAdd(&testHash, "key1", "contents1");
	    xhAdd(&testHash, "key2", "contents2");
	    xhAdd(&testHash, "key3", "contents3");
	    xhAdd(&testHash, "key4", "contents4");
	    xhAdd(&testHash, "key5", "contents5");
	    xhAdd(&testHash, "key6", "contents6");
	    xhAdd(&testHash, "key7", "contents7");
	    xhAdd(&testHash, "key8", "contents8");
	    xhAdd(&testHash, "key9", "contents9");

	    /** Test the initial element. **/
	    testEntry = xhGetNextElement(&testHash, NULL);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 1);

	    /** Run all tests, checking for correct content. **/
	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 2);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 3);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 4);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 5);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 6);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 7);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 8);

	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(!strcmp(testEntry->Data, xhi_internal_MatchKey(testEntry->Key)) && 9);

	    /** Test the final element as NULL. **/
	    testEntry = xhGetNextElement(&testHash, testEntry);
	    assert(testEntry == NULL);

	    xhDeInit(&testHash);
	    }

    return iter;
    }

