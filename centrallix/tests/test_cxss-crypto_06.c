#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"


long long
test(char** name)
    {
    *name = "CXSS Crypto 06: AES Encyrpt and Decrypt";
    
    char * short1 = "a quick test";
    char * short2 = "a diff test.";
    char * data = "For God so loved the world that he gave his only begotten son, that whoever belives in him shall not perish but have everlasting life";

    /** Setup **/
    cxss_internal_InitEntropy(1280);
    cxssCryptoInit();

    /** test encrypt changes data **/
    char key1 [32];
    char iv1 [16];
    char* ciphertext1 = NULL;
    int cipherLen1 = 0;

    int result = cxssGenerate256bitRandomKey(&key1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate128bitIV(&iv1);
    assert(result == CXSS_CRYPTO_SUCCESS);

    result = cxssEncryptAES256(data, strlen(data)+1, key1, iv1, &ciphertext1, &cipherLen1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(data, ciphertext1) != 0);

    /** test decrpty restores data **/
    char* plaintext1 = NULL;
    int plainLen1 = 0;

    result = cxssDecryptAES256(ciphertext1, cipherLen1, key1, iv1, &plaintext1, &plainLen1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(data, plaintext1) == 0);
    assert(cxssAES256CiphertextLength(plainLen1) == cipherLen1); /* check length prediction */
    
    /** make sure same key, iv, and text comes out the same **/
    char* ciphertext2 = NULL;
    int cipherLen2 = 0;

    result = cxssEncryptAES256(data, strlen(data)+1, key1, iv1, &ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext2, ciphertext1) == 0);
    assert(cipherLen1 == cipherLen2);

    /** make sure same key, iv with idfferent text is different. **/
    result = cxssEncryptAES256(short1, strlen(short1)+1, key1, iv1, &ciphertext1, &cipherLen1);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptAES256(short2, strlen(short2)+1, key1, iv1, &ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext2, ciphertext1) != 0);

    /** make sure changing the IV changes it **/
    char iv2 [16];
    result = cxssGenerate128bitIV(&iv2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptAES256(short1, strlen(short1)+1, key1, iv2, &ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext2, ciphertext1) != 0);
    
    /** make sure key changes it **/
    char key2 [32];
    result = cxssGenerate256bitRandomKey(&key2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptAES256(short1, strlen(short1)+1, key2, iv1, &ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext2, ciphertext1) != 0);

    cxssCryptoCleanup();

    return 0;
    }
