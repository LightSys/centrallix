#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
    /** Setup **/
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();
    
    *name = "CXSS Crypto 08: Gen key from password";

    char key1[32];
    char key2[32];
    unsigned char salt1[12];
    unsigned char salt2[8];
    char * test = salt1;

    int result = cxssGenerate64bitSalt(test);
    assert(result == CXSS_CRYPTO_SUCCESS);

    /** check consistency **/
    result = cxssGenerate256bitPasswordBasedKey("password", salt1, key1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate256bitPasswordBasedKey("password", salt1, key2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    
    for(int i = 0 ; i < 32 ; i++)
        {
        assert(key1[i] == key2[i]);
        }

    /** check that salts change result **/
    result = cxssGenerate64bitSalt(salt2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate256bitPasswordBasedKey("password", salt1, key1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate256bitPasswordBasedKey("password", salt2, key2);
    assert(result == CXSS_CRYPTO_SUCCESS);

    int isMatch = 1;
    for(int i = 0 ; i < 32 ; i++)
        {
        isMatch &= (key1[i] == key2[i]);
        }
    assert(!isMatch);

    /** check that passfrase matters  **/
    result = cxssGenerate256bitPasswordBasedKey("password", salt1, key1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate256bitPasswordBasedKey("newFraze", salt1, key2);
    assert(result == CXSS_CRYPTO_SUCCESS);

    isMatch = 1;
    for(int i = 0 ; i < 32 ; i++)
        {
        isMatch &= (key1[i] == key2[i]);
        }
    assert(!isMatch);

    cxssCryptoCleanup();

    return 0;
    }
