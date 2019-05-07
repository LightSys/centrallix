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
cxss_encrypt_aes256(unsigned char *plaintext, int plaintext_len, 
                    unsigned char *key, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    char init_vector[128];
    int len, ciphertext_len;
    
    assert(CSPRNG_Initialized == 1);

    /* Generate random initialization vector */
    if (RAND_bytes(init_vector, 128) != 1) {
        fprintf(stderr, "Failed to generate initialization vector!\n");
        return -1;
    }

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

/** @brief Generate 32-bit random salt
 *
 *  Generate a 32-bit random salt using
 *  openssl's cryptographically-secure 
 *  random number generator.
 *
 *  @param      Pointer to 32-bit buffer to store the salt
 *  @return     Status code
 */
int
cxss_generate_32bit_salt(char *salt)
{
    assert(CSPRNG_Initialized == 1);

    if (RAND_bytes(salt, 32) < 0) {
        fprintf(stderr, "Failed to generate salt!\n");
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
                          salt, 32, PBKDF2_ITER_NO, 
                          EVP_sha256(), 256, key) != 1) {
        fprintf(stderr, "Failed to generate 256-bit key\n");
        return -1;
    }

    return 0;
}

