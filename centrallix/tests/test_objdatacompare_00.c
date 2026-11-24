#include <assert.h>
#include "obj.h"

/*** Compare integers far enough apart that subtracting them overflows, in
 *** each integer branch of objDataCompare() and in both operand orders.
 ***/
long long
test(char** name)
    {
    int big = 2000000000;
    int small = -2000000000;
    char* small_str = "-2000000000";
    int big_ints[] = { 0, 2000000000 };
    int small_ints[] = { 0, -2000000000 };
    IntVec small_one = { 1, &small };
    IntVec big_vec = { 2, big_ints };
    IntVec small_vec = { 2, small_ints };

	*name = "objdatacompare_00 objDataCompare() integer overflow";

	/** Integer and integer **/
	assert(objDataCompare(DATA_T_INTEGER, &big, DATA_T_INTEGER, &small) == 1);
	assert(objDataCompare(DATA_T_INTEGER, &small, DATA_T_INTEGER, &big) == -1);

	/** Integer and string **/
	assert(objDataCompare(DATA_T_INTEGER, &big, DATA_T_STRING, small_str) == 1);
	assert(objDataCompare(DATA_T_STRING, small_str, DATA_T_INTEGER, &big) == -1);

	/** Integer and one-element integer vector **/
	assert(objDataCompare(DATA_T_INTEGER, &big, DATA_T_INTVEC, &small_one) == 1);
	assert(objDataCompare(DATA_T_INTVEC, &small_one, DATA_T_INTEGER, &big) == -1);

	/** Integer vectors whose first difference is far apart **/
	assert(objDataCompare(DATA_T_INTVEC, &big_vec, DATA_T_INTVEC, &small_vec) == 1);
	assert(objDataCompare(DATA_T_INTVEC, &small_vec, DATA_T_INTVEC, &big_vec) == -1);

    return 0;
    }
