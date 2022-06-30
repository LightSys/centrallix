#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
    *name = "CXSS Crypto 04: RSA Key Generaion";
    
    int pubSize1 = 0;
    int priSize1 = 0;
    char* pubKey1 = NULL;
    char* priKey1 = NULL;
    int pubSize2 = 0;
    int priSize2 = 0;
    char* pubKey2 = NULL;
    char* priKey2 = NULL;


    /** Setup **/
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();

    int result = cxssGenerateRSA4096bitKeypair(&priKey1, &priSize1, &pubKey1, &pubSize1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    
    assert(pubSize1 > 0);
    assert(priSize1 > 0);
    assert(pubKey1 != 0);
    assert(priKey1 != 0);

    result = cxssGenerateRSA4096bitKeypair(&priKey2, &priSize2, &pubKey2, &pubSize2);
    assert(result == CXSS_CRYPTO_SUCCESS);

    assert(pubSize2 > 0);
    assert(priSize2 > 0);
    assert(pubKey2 != 0);
    assert(priKey2 != 0);

    /** make sure keys are not the same **/
    int minLen = (pubSize1 < pubSize2)? pubSize1 : pubSize2;
    int isMatch = 1;
    for(int i = 0 ; i < minLen ; i++)
        {
        isMatch &= pubKey1[i] == pubKey2[i];
        }
    assert(!isMatch);

    /** now check private key **/
    minLen = (priSize1 < priSize2)? priSize1 : priSize2;
    isMatch = 1;
    for(int i = 0 ; i < minLen ; i++)
        {
        isMatch &= priKey1[i] == priKey2[i];
        }
    assert(!isMatch);

    /** check for header text **/
    assert(strstr(pubKey1, "BEGIN RSA PUBLIC KEY") > 0);
    assert(strstr(pubKey2, "BEGIN RSA PUBLIC KEY") > 0);
    assert(strstr(priKey1, "BEGIN RSA PRIVATE KEY") > 0);
    assert(strstr(priKey2, "BEGIN RSA PRIVATE KEY") > 0);

    /** now footer **/
    assert(strstr(pubKey1, "END RSA PUBLIC KEY") > 0);
    assert(strstr(pubKey2, "END RSA PUBLIC KEY") > 0);
    assert(strstr(priKey1, "END RSA PRIVATE KEY") > 0);
    assert(strstr(priKey2, "END RSA PRIVATE KEY") > 0);

    cxssCryptoCleanup();

    return 0;
    }
