#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
//#include "cxss/cxss.h"
#include "cxss_credentials_db.h"

static int CSPRNG_Initialized = 0;

void
cxss_initialize_csprng(void)
{
    char seed[256];
    memset(seed, 0, 256);    

    /*
    if (cxss_internal_GetBytes(seed, 256) != 0) {
        fprintf(stderr, "Failed to seed random number generator\n");
        return -1;
    }
    */

    /* Seed RNG */
    RAND_seed(seed, 256);

    /* Mark RNG as initialized */
    CSPRNG_Initialized = 1;
}        

int 
cxss_encrypt_aes256(const char *plaintext, int plaintext_len, 
                    const char *key, const char *init_vector,
                    char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;
    
    /* Create new openssl cipher context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        fprintf(stderr, "Failed to create new openssl cipher context\n");
        return -1;
    }

    /* Initiate encryption */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, init_vector) != 1)        return -1;


    /* Encrypt data */
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
        return -1;
    ciphertext_len = len;


    /* Finalize encryption */
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "Error while finalizing encryption\n");
        return -1;
    }
    ciphertext_len += len;

    /* Close openssl cipher context */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int
cxss_decrypt_aes256(const char *ciphertext, int ciphertext_len,
                    const char *key, const char *init_vector,
                    char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;
    
    /* Create new openssl cipher context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        fprintf(stderr, "Failed to create new openssl cipher context\n");
        return -1;
    }

    /* Initiate decryption */
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, init_vector) != 1)
        return -1;

    /* Decrypt data */
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        return -1;
    plaintext_len = len;

    /* Finalize decryption */
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        fprintf(stderr, "Error while finalizing decryption!\n");
        return -1;
    }
    plaintext_len += len;   
        
    /* Cleanup */
    EVP_CIPHER_CTX_free(ctx);
    
    return plaintext_len;
}            

/** @brief Generate 64-bit random salt
 *
 *  Generate a 64-bit random salt using
 *  openssl's cryptographically-secure 
 *  random number generator.
 *
 *  @param      Pointer to 64-bit buffer to store the salt
 *  @return     Status code
 */
int
cxss_generate_64bit_salt(char *salt)
{
    assert(CSPRNG_Initialized == 1);

    if (RAND_bytes(salt, 8) < 0) {
        fprintf(stderr, "Failed to generate salt!\n");
        return -1;
    }

    return 0;
}

/** @brief Generate 128-bit random initialization vector
 *
 *  Generate a 128-bit random initialization vector,
 *  suitable for AES, using openssl's cryptographically-secure
 *  random number generator.
 *
 *  @param      Pointer to 128-bit buffer to store the iv
 *  @return     Status code
 */
int
cxss_generate_128bit_iv(char *init_vector)
{
    assert(CSPRNG_Initialized == 1);

    /* Generate random initialization vector */
    if (RAND_bytes(init_vector, 16) != 1) {
        fprintf(stderr, "Failed to generate initialization vector!\n");
        return -1;
    }

    return 0;
}

/** @brief Generate 256-bit key from password+salt
 *
 *  Generate a 256-bit key suitable for AES-256 from a 
 *  password/passphrase and a random salt. The alogrithm 
 *  uses a password-based key derivation function. 
 *
 *  IMPORTANT: For better security the password fed to this 
 *             function should already have good entropy! 
 *
 *  @param password     User password
 *  @param salt         Salt
 *  @param key          Pointer to a 256-bit buffer to store the key
 *  @return             Status code
 */
int
cxss_generate_256bit_key(const char *password, const char *salt, char *key)
{
    #define PBKDF2_ITER_NO      100
    
    if (PKCS5_PBKDF2_HMAC(password, strlen(password),
                          salt, 8, PBKDF2_ITER_NO, 
                          EVP_sha256(), 32, key) != 1) {
        fprintf(stderr, "Failed to generate 256-bit key\n");
        return -1;
    }

    return 0;
}

