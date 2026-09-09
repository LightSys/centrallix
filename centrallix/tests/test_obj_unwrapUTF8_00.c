#include <assert.h>
#include <string.h>
#include "centrallix.h"
#include "obj.h"

int
test_unwrap_success(char* in_buf, size_t in_len, char* expect_buf, size_t expect_len)
    {
    size_t out_len = 0;
    char* out_buf;
    int ret = 0;

    /** test normal **/
    if(objUnwrapUTF8(in_buf, in_len, &out_buf, &out_len) != 0)
	{
	ret = -1;
	goto end;
	}
    if(out_len != expect_len)
	{
	ret = -2;
	goto end;
	} 
    if(memcmp(out_buf, expect_buf, out_len) != 0)
	{
	ret = -3;
	goto end;
	}
end:
    if(out_buf) nmSysFree(out_buf);
    return ret;
    }

int
test_unwrap_fail(char* in_buf, size_t in_len)
    {
    size_t out_len = 0;
    char* out_buf = NULL;

    /** test normal **/
    if(objUnwrapUTF8(in_buf, in_len, &out_buf, &out_len) != -1) return -1;
    if(out_len != 0) return -2;
    if(out_buf != NULL) return -3;
    return 0;
    }

long long
test(char** name)
    {
    *name = "obj_unwrapUTF8_00 Unwrap UTF-8 Strings";
    char* in_buf;
    char* expect_buf;

    /** test basic ASCII **/
    in_buf = "\x01\x02\x03\x04\x05\x06\x07\x09\x0a\x0b\x0c\x0d\x0e\x0f"
	     "\x10\x11\x12\x13\x14\x15\x16\x17\x19\x1a\x1b\x1c\x1d\x1e\x1f"
	     " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	     "[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";
    expect_buf = in_buf;
    assert(test_unwrap_success(in_buf, strlen(in_buf), expect_buf, strlen(expect_buf)) == 0);
    assert(test_unwrap_success(in_buf, strlen(in_buf)+1, expect_buf, strlen(expect_buf)+1) == 0);


    /** range of UTF-8 characters **/
    in_buf = "¡¢¥ÆÃ";
    expect_buf = "\xA1\xA2\xA5\xC6\xC3";
    assert(test_unwrap_success(in_buf, strlen(in_buf), expect_buf, strlen(expect_buf)) == 0);

    in_buf = "ぁあぃいぅうぇえぉ";
    expect_buf = "\x30\x41\x30\x42\x30\x43\x30\x44\x30\x45\x30\x46\x30\x47\x30\x48\x30\x49";
    assert(test_unwrap_success(in_buf, strlen(in_buf), expect_buf, strlen(expect_buf)) == 0);

    in_buf = "🨀🨁🨂🨃🨄🨅";
    expect_buf = "\x01\xfA\x00\x01\xfA\x01\x01\xfA\x02\x01\xfA\x03\x01\xfA\x04\x01\xfA\x05";
    assert(test_unwrap_success(in_buf, 24, expect_buf, 18) == 0);

    in_buf = "AaBbCc0123,!'¡¢¥ÆÃぁあぃいぅうぇえぉ🨀🨁🨂🨃🨄🨅";
    expect_buf = "AaBbCc0123,!'"
		 "\xA1\xA2\xA5\xC6\xC3"
		 "\x30\x41\x30\x42\x30\x43\x30\x44\x30\x45\x30\x46\x30\x47\x30\x48\x30\x49"
		 "\x01\xfA\x00\x01\xfA\x01\x01\xfA\x02\x01\xfA\x03\x01\xfA\x04\x01\xfA\x05";
    assert(test_unwrap_success(in_buf, 74, expect_buf, 54) == 0);


    /** containing min/max for each range **/
    in_buf = "\x00"             "\x7f"              /* asccii range */
	     "\xc2\x80"         "\xDF\xBF"          /* 2 byte range */
	     "\xE0\xA0\x80"     "\xEF\xBF\xBF"      /* 3 byte range */
	     "\xF0\x90\x80\x80" "\xF4\x8F\xBF\xBF"; /* 4 byte range */
    expect_buf = "\x00"         "\x7f"          /* asccii range */
		 "\x80"         "\x07\xFF"      /* 2 byte range */
		 "\x08\x00"     "\xFF\xFF"      /* 3 byte range */
		 "\x01\x00\x00" "\x10\xFF\xFF"; /* 4 byte range */
    assert(test_unwrap_success(in_buf, 21, expect_buf, 16) == 0);


    /** overlong forms **/
    /** 2-byte overlong: **/
    in_buf = "\xC0\x80";           /* encodes U+0000 (smallest 2 byte overlong) */
    assert(test_unwrap_fail(in_buf, 2) == 0); 

    in_buf = "\xC0\xA0";           /* encodes U+0020 (middle 2 byte overlong) */
    assert(test_unwrap_fail(in_buf, 2) == 0);

    in_buf = "\xC1\xBF";           /* encodes U+007F (largest 2 byte overlong) */
    assert(test_unwrap_fail(in_buf, 2) == 0);

    /**  3-byte overlong **/
    in_buf = "\xE0\x80\x80";       /* encodes U+0000 (smallest 3 byte overlong) */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xE0\x84\x80";       /* encodes  U+0100 (middle 3 byte overlong) */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xE0\x9F\xBF";       /* encodes  U+07FF (largest 3 byte overlong) */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    /** 4-byte overlong: **/
    in_buf = "\xF0\x80\x80\x80";   /* encodes U+0000 (smallest 4 byte overlong) */
    assert(test_unwrap_fail(in_buf, 4) == 0);

    in_buf = "\xF0\x81\x80\x80";   /* encodes U+1000 (middle 4 byte overlong) */
    assert(test_unwrap_fail(in_buf, 4) == 0);

    in_buf = "\xF0\x8F\xBF\xBF";   /* encodes U+FFFF (largest 4 byte overlong) */
    assert(test_unwrap_fail(in_buf, 4) == 0);


    /** Stray continuation bytes **/
    in_buf = "\x80"; /* a lone continuation byte with nothing before it */
    assert(test_unwrap_fail(in_buf, 1) == 0);

    in_buf = "\x80" "AB"; /* a lone continuation byte at the start of a longer string */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "AB\x80" "CD"; /* a stray continuation byte in the middle of a string */
    assert(test_unwrap_fail(in_buf, 5) == 0);

    in_buf = "AB\x80"; /* a stray continuation byte at the end of a string */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\x80\x81\x82"; /* multiple consecutive stray continuation bytes */
    assert(test_unwrap_fail(in_buf, 3) == 0);


    /** Truncated multibyte continuation bytes */
    
    in_buf = "\xC2" "AB"; /* 2-byte character: leading byte only, nothing else in the string */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "A\xC2"; /* 2-byte character truncated at the very end of the string */
    assert(test_unwrap_fail(in_buf, 2) == 0);

    in_buf = "\xE3" "AB"; /* 3-byte character: leading byte only */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xE3\x81" "AB"; /* 3-byte character: leading byte + first continuation byte only */
    assert(test_unwrap_fail(in_buf, 4) == 0);

    in_buf = "AB\xE3\x81"; /* 3-byte character truncated (after 1 continuation byte) at the end of the string */
    assert(test_unwrap_fail(in_buf, 4) == 0);

    in_buf = "\xF0" "AB"; /* 4-byte character: leading byte only */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xF0\x9F" "AB"; /* 4-byte character: leading byte + first continuation byte only */
    assert(test_unwrap_fail(in_buf, 4) == 0);

    in_buf = "\xF0\x9F\xA8" "AB"; /* 4-byte character: leading byte + first two continuation bytes only */
    assert(test_unwrap_fail(in_buf, 5) == 0);

    in_buf = "AB\xF0\x9F\xA8"; /* 4-byte character truncated (after 2 continuation bytes) at the end of the string */
    assert(test_unwrap_fail(in_buf, 5) == 0);


    /** Surrogates **/
    in_buf = "\xED\xA0\x80"; /* smallest surrogate U+D800 */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xED\xAF\xBF"; /* middle surrogate U+DBFF */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    in_buf = "\xED\xBF\xBF"; /* largest surrogate U+DFFF */
    assert(test_unwrap_fail(in_buf, 3) == 0);

    return 0;
    }
