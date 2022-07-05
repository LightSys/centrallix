#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"


long long
test(char** name)
    {
    *name = "CXSS Crypto 05: RSA Encyrpt and Decrypt";
    
    char * short1 = "a quick test";
    char * short2 = "a diff test.";
    char * data = "For God so loved the world that he gave his only begoten son, that whoever belives in him shall not perish but have everlasting life";

    /** Setup **/
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();

    /** Generate keys for use **/
    int pubSize1 = 0;
    int privSize1 = 0;
    char* pubKey1 = NULL;
    char* privKey1 = NULL;

    int result = cxssGenerateRSA4096bitKeypair(&privKey1, &privSize1, &pubKey1, &pubSize1);
    assert(result == CXSS_CRYPTO_SUCCESS);

    int pubSize2 = 0;
    int privSize2 = 0;
    char* pubKey2 = NULL;
    char* privKey2 = NULL;

    result = cxssGenerateRSA4096bitKeypair(&privKey2, &privSize2, &pubKey2, &pubSize2);
    assert(result == CXSS_CRYPTO_SUCCESS);

    /** Test encryption for key 1**/
    size_t dataLen = strlen(data);
    int cipherLen = 4096;
    char ciphertext[4096];

    result = cxssEncryptRSA(data, dataLen, pubKey1, pubSize1, ciphertext, &cipherLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(ciphertext != NULL);
    assert(cipherLen > 0);
    assert(strcmp(data, ciphertext) != 0);

    /** test decryption for key 1 **/
    char plaintext[4096];
    int plainLen = 0;

    result = cxssDecryptRSA(ciphertext, cipherLen, privKey1, privSize1, plaintext, &plainLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(plaintext != NULL);
    assert(plainLen > 0);
    assert(strcmp(data, plaintext) == 0);

    /** make sure different messages with same key are not the same **/
    int cipherLen2 = 4096;
    char ciphertext2[4096];

    result = cxssEncryptRSA(short1, strlen(short1), pubKey1, pubSize1, ciphertext, &cipherLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptRSA(short2, strlen(short2), pubKey1, pubSize1, ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext, ciphertext2) != 0);

    /** make sure same data and same key do not match (padding) but decrypt correctly **/
    result = cxssEncryptRSA(short1, strlen(short1)+1, pubKey1, pubSize1, ciphertext, &cipherLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptRSA(short1, strlen(short1)+1, pubKey1, pubSize1, ciphertext2, &cipherLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext, ciphertext2) != 0);

    int plainLen2 = 4096;
    char plaintext2[4096];

    result = cxssDecryptRSA(ciphertext, cipherLen, privKey1, privSize1, plaintext, &plainLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssDecryptRSA(ciphertext2, cipherLen2, privKey1, privSize1, plaintext2, &plainLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(plaintext, plaintext2) == 0);


    /** make sure same data and different key are not the same, but match on decrypt **/
    result = cxssEncryptRSA(short1, strlen(short1)+1, pubKey1, pubSize1, ciphertext, &cipherLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssEncryptRSA(short1, strlen(short1)+1, pubKey2, pubSize2, ciphertext2, &cipherLen2);;
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(ciphertext, ciphertext2) != 0);

    result = cxssDecryptRSA(ciphertext, cipherLen, privKey1, privSize1, plaintext, &plainLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssDecryptRSA(ciphertext2, cipherLen2, privKey2, privSize2, plaintext2, &plainLen2);
    assert(result == CXSS_CRYPTO_SUCCESS);
    assert(strcmp(plaintext, plaintext2) == 0);
    assert(strcmp(plaintext, short1) == 0);


    cxssCryptoCleanup();

    return 0;
    }
