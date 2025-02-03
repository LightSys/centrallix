#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
    *name = "CXSS Crypto 07: RSA Key Destruction";
    
    int pubSize = 0;
    int priSize = 0;
    char* pubKey = NULL;
    char* priKey = NULL;


    /** Setup **/
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();

    int result = cxssGenerateRSA4096bitKeypair(&priKey, &priSize, &pubKey, &pubSize);
    assert(result == CXSS_CRYPTO_SUCCESS);


    /** Make sure it is able to destroy the public key **/
    char* backup;
    backup = nmSysMalloc(pubSize);
    strcpy(backup, pubKey);

    cxssDestroyKey(pubKey, pubSize);
    int match = 0;
    for(int i = 0 ; i < pubSize ; i++)
        {
        if(pubKey[i] == backup[i]) match++;
        }
    /** Data is deallocated so may have new data in it. Assert no more than twice the 
     ** number of matches expected from random chance 
     **/
    assert(match < (((pubSize/256)+1))*2); 
    nmFree(backup, pubSize);

    /** now test on private key **/
    backup = nmSysMalloc(priSize);
    strcpy(backup, priKey);

    cxssDestroyKey(priKey, priSize);
    match = 0;
    for(int i = 0 ; i < priSize ; i++)
        {
        if(priKey[i] == backup[i]) match++;
        }

    assert(match < (((priSize/256)+1))*2); 
    nmFree(backup, priSize);

    cxssCryptoCleanup();

    return 0;
    }
