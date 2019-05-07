#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "cxss_credentials_db.h"

static int 
AES_256_Encrypt(const char *plaintext, int plaintext_len, 
                const char *key, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    unsigned char iv[128];
    int len, ciphertext_len;

    /* 
     * Compute random initialization vector.
     *
     * IMPORTANT:  Since AES is a 128-bit block cipher,
     *             the IV must be 128 bits in length.
     */
    if (RAND_bytes(iv, 128) != 1) {
        fprintf(stderr, "Rand bytes failed!\n");
        return -1;
    }

    /* Initialize the encryption operation */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
        fprintf(stderr, "Error while initializing AES256 cipher\n");
        return -1;
    }

    /* Feed the plaintext to be encrypted */
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
        return -1;
    ciphertext_len = len;


    /* Finalize the encryption */
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "Error while finalizing encryption\n");
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}


