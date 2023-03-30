#include <assert.h>
#include "include/mime.h"

void writeFile(char* rpt[], char * name);
pStructInf testFile(char * name);

long long
test(char** name)  
    {
    *name = "mime_00 DecodeBase64 with invalid NULL bytes";
    setlocale(0, "en_US.UTF-8");

    char buf[50];

    /** basic **/
    int rval = libmime_DecodeBase64(buf, "YSBxdWljayB0ZXN0", 50);
    assert(strcmp(buf, "a quick test") == 0);
    assert(rval == 12);

    /** ensure ignore nulls in odd places **/
    rval = libmime_DecodeBase64(buf, "\0SBxdWljayB0ZXN0", 50);
    assert(rval == 0);
    rval = libmime_DecodeBase64(buf, "Y\0BxdWljayB0ZXN0", 50);
    assert(rval < 0);
    rval = libmime_DecodeBase64(buf, "YS\0xdWljayB0ZXN0", 50);
    assert(rval < 0);
    rval = libmime_DecodeBase64(buf, "YSB\0dWljayB0ZXN0", 50);
    assert(rval < 0);
    memset(buf, '\0', 50);
    rval = libmime_DecodeBase64(buf, "YSBx\0WljayB0ZXN0", 50);
    assert(strcmp(buf, "a q") == 0);
    assert(rval == 3);

    return;
    }
