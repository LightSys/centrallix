#include <assert.h>
#include "charsets.h"
long long
test(char** name)
    {
    int* len;
    char* out;
    char* in;
    char* wordlist;
    char* buffer;
    int i;
    
    *name = "Exp UTF-8: charsets mixed"; /* provides tests for the internal function found in charsets */

    len = nmMalloc(sizeof(int));
    setlocale(0, "en_US.UTF-8"); /* the 0 may not be the propper setting */
    
    /** basic test **/
    chrToMixed(NULL, NULL, len, NULL);
    assert(*len == CHR_INVALID_ARGUMENT);


    /*** Test simple c style strings examples with no buffer ***/

    *len = 0;
    in = "test";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("Test", out) == 0);
    nmSysFree(out);

    *len = 0;
    in = "a longer test";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("A Longer Test", out) == 0);
    nmSysFree(out);
 
    *len = 0;
    in = "a test with dash-in it";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("A Test With Dash-in It", out) == 0);
    nmSysFree(out);

    *len = 0;
    in = "a test with quote'in it";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("A Test With Quote'in It", out) == 0);
    nmSysFree(out);

    *len = 0;
    in = "a test with underscore_in it";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("A Test With Underscore_In It", out) == 0);
    nmSysFree(out);

    *len = 0;
    in = "a tEst With MIXED cAsEs";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s \n", in, out);
    assert(*len > 0);
    assert(strcmp("A Test With Mixed Cases", out) == 0);
    nmSysFree(out);

    /** Try with more complex utf-8 characters**/
    *len = 0;
    in = "à sèntence that ábÙsés latìn-Líkê chãräctërs";
    out = chrToMixed(in, NULL, len, NULL);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("À Sèntence That Ábùsés Latìn-líkê Chãräctërs", out) == 0);
    nmSysFree(out);
    

    /*** Test with wordlist ***/
    *len = 0;
    wordlist = "test,McDonald,a,fArM";
    in = "old mcdonald had a farm";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("Old McDonald Had a fArM", out) == 0);
    nmSysFree(out);

    *len = 0;
    wordlist = "test,Mc*,a,fArM";
    in = "old mcdonald had a farm";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("Old McDonald Had a fArM", out) == 0);
    nmSysFree(out);

    *len = 0;
    wordlist = "the,beginning,created,aNd,test,not-used,useless,etc";
    in = "in THE BeGinNiNg goD CREATED the heavEns and the EARTH";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("In the beginning God created the Heavens aNd the Earth", out) == 0);
    nmSysFree(out);

    *len = 0;
    wordlist = "";
    in = "in THE BeGinNiNg goD CREATED the heavEns and the EARTH";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("In The Beginning God Created The Heavens And The Earth", out) == 0);
    nmSysFree(out);

    *len = 0;
    wordlist = "test,not-used,useless,etc";
    in = "В начале сотворил Бог небо и землю.";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("В Начале Сотворил Бог Небо И Землю.", out) == 0);
    nmSysFree(out);

    *len = 0;
    wordlist = "сотворил,БОГ,НАЧ*";
    in = "В начале сотворил Бог небо и землю.";
    out = chrToMixed(in, NULL, len, wordlist);
    printf("%s >> %s\n", in, out);
    assert(*len > 0);
    assert(strcmp("В НАЧАле сотворил БОГ Небо И Землю.", out) == 0);
    nmSysFree(out);


    /*** Test errors ***/

    /** No input string **/
    *len = 0;
    wordlist = "test,just,here,for,show";
    out = chrToMixed(NULL, NULL, len, wordlist);
    assert(*len == CHR_INVALID_ARGUMENT);

    *len = 0;
    out = chrToMixed(NULL, NULL, len, NULL);
    assert(*len == CHR_INVALID_ARGUMENT);

    /** Invalid char **/
    *len = 0;
    in = "A normal string until \xFF";
    out = chrToMixed(in, NULL, len, NULL);
    assert(*len == CHR_INVALID_CHAR);
    
    *len = 0;
    in = "A normal string for real";
    wordlist = "just,a,bad,wordlist,\xFF";
    out = chrToMixed(in, NULL, len, wordlist);
    assert(*len == CHR_INVALID_CHAR);

    //TODO: find a way to test out of memory errors. 

    
    /*** Tests with buffer ***/

    /** exact size **/
    *len = 10;
    buffer = nmMalloc(10);
    in = "ten chars"; /* with null terminator */
    out = chrToMixed(in, buffer, len, NULL);
    assert(strcmp(out, "Ten Chars") == 0);
    assert(buffer == out);
    nmFree(buffer, 10);

    /** too big **/
    *len = 100;
    buffer = nmMalloc(100);
    in = "less than one hundred chars"; /* with null terminator */
    out = chrToMixed(in, buffer, len, NULL);
    assert(strcmp(out, "Less Than One Hundred Chars") == 0);
    assert(buffer == out);
    nmFree(buffer, 100);

    /** too small **/
    *len = 5;
    buffer = nmMalloc(5);
    in = "more than five chars";
    out = chrToMixed(in, buffer, len, NULL);
    assert(strcmp(out, "More Than Five Chars") == 0);
    assert(buffer != out);
    nmFree(buffer, 5);
    nmSysFree(out);

    nmFree(len, sizeof(int));
    
    return 0;
    }
