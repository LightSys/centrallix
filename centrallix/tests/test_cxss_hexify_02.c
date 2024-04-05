#include <assert.h>
#include <string.h>
#include "centrallix.h"
#include "obj.h"
#include "cxss/cxss.h"

//int cxssHexify(unsigned char* bindata, size_t bindatalen, char* hexdata, size_t hexdatabuflen);
int cxss_i_Hexify(unsigned char* bindata, size_t bindatalen, char* hexdata, size_t hexdatalen);

int
test_hexify(char* str, char* cmp, int dstlen, int cmprval)
    {
    char buf1[256];
    int len = strlen(str);
    int cmplen = strlen(cmp);
    int rval;

    assert(len <= 251);
    assert(dstlen <= 252);
    assert(dstlen >= 0);
    memset(buf1, 127, sizeof(buf1));
    strcpy(buf1+2, str);

    rval = cxss_i_Hexify(buf1+2, len, buf1+2, dstlen);

    assert(rval == cmprval);

    if (rval < 0) return 0;

    assert(buf1[0] == 127);
    assert(buf1[1] == 127);
    assert(buf1[cmplen+2+1] == 127 || cmplen < len);
    assert(buf1[cmplen+2+2] == 127 || cmplen < len-1);
    assert(!strcmp(buf1+2, cmp));

    return 0;
    }

long long
test(char** name)
    {
    *name = "cxss_hexify_01 cxss_i_Hexify() same buffer";

    test_hexify("", "", 0, 0);
    test_hexify("", "", 1, 0);
    test_hexify("", "", 2, 0);
    test_hexify("A", "41", 3, 2);
    test_hexify("A", "41", 2, 2);
    test_hexify("A", "4", 1, 1);
    test_hexify("AB", "4142", 4, 4);
    test_hexify("AB", "414", 3, 3);
    test_hexify("AB", "41", 2, 2);
    test_hexify("AB", "4", 1, 1);
    test_hexify("AB", "", 0, 0);
    test_hexify("ABC", "414243", 6, 6);
    test_hexify("ABC", "41424", 5, 5);
    test_hexify("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", "303132333435363738394142434445464748494a4b4c4d4e4f505152535455565758595a", 200, 72);

    return 0;
    }
