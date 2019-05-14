#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
//#include "cxss/cxss.h"
#include "cxss_credentials_db.h"

static int CSPRNG_Initialized = 0;

#define DEBUG(x) fprintf(stderr, (x));

void
cxss_initialize_crypto(void)
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

void cxss_cleanup_crypto(void)
{
    /* clear any error */
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    ERR_remove_state(0);
    EVP_cleanup();
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
        goto error;

    /* Decrypt data */
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        goto error;
    plaintext_len = len;

    /* Finalize decryption */
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        fprintf(stderr, "Error while finalizing decryption!\n");
        goto error;
    }
    plaintext_len += len;   
        
    /* Cleanup */
    EVP_CIPHER_CTX_free(ctx);
    
    return plaintext_len;
error:
    EVP_CIPHER_CTX_free(ctx);
    return -1;
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

/** @brief Generate 256-bit random key 
 *
 *  Generate a 256-bit random key, suitable for AES-256. 
 *
 *  @param key          Pointer to a 256-bit buffer to store the key
 *  @return             Status code
 */
int
cxss_generate_256bit_rand_key(char *key)
{
    assert(CSPRNG_Initialized == 1);

    /* Generate random initialization vector */
    if (RAND_bytes(key, 32) != 1) {
        fprintf(stderr, "Failed to generate key!\n");
        return -1;
    }

    return 0;
}

/** @brief Compute length of AES ciphertext
 *
 *  This function computes the length of the output
 *  ciphertext of an AES (128-bit block) cipher.
 *
 *  @param plaintext_len        Length of plaintext
 *  @return                     Ciphertext length
 */
size_t
cxss_aes256_ciphertext_length(size_t plaintext_len)
{
    return (plaintext_len + (16 - plaintext_len%16)); 
}

/** @brief Generate a 2048-bit RSA keypair
 *
 *  This function generates a 2048-bit RSA keypair
 *
 *  @param privatekey           Pointer to a 2048-bit buffer to store pri_key
 *  @param publickey            Pointer to a 2048-bit buffer to store pub_key
 *  @return                     Status code
 */
int
cxss_generate_rsa_2048bit_keypair(char **privatekey, char **publickey,
                                  size_t *privatekey_len, size_t *publickey_len)
{
    RSA *rsa_keypair = NULL;
    BIGNUM *bne = NULL;
    BIO *pri = NULL, *pub = NULL;
    char *pri_key = NULL;
    char *pub_key = NULL;
    unsigned long e = RSA_F4;
    size_t pri_len, pub_len;

    /* Generate bignum */
    bne = BN_new();
    if (BN_set_word(bne, e) != 1) {
        fprintf(stderr, "Failed to generate bignum\n");
        goto error;
    }

    /* Generate keypair */
    rsa_keypair = RSA_new();
    if (RSA_generate_key_ex(rsa_keypair, 2048, bne, NULL) != 1) {
        fprintf(stderr, "Failed to generate keypair\n");
        goto error;
    }
    
    pri = BIO_new(BIO_s_mem()); 
    pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, rsa_keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, rsa_keypair);

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    pri_key = malloc(pri_len + 1);
    pub_key = malloc(pub_len + 1);

    if (!pri_key || !pub_key) {
        fprintf(stderr, "Memory allocation error\n");
        goto error;
    }

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);
  
    pri_key[pri_len++] = '\0';
    pub_key[pub_len++] = '\0';

    *privatekey = pri_key;
    *publickey = pub_key;
    *privatekey_len = pri_len;
    *publickey_len = pub_len;

    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);    
    BN_free(bne);
    return 0;  

error:
    free(privatekey);
    free(publickey);
    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);
    BN_free(bne);
    return -1;
} 

/** @brief Free public/private keypair
 *
 *  Free public/private keypair
 *
 *  @param      Pointer to privatekey
 *  @param      Pointer to publickey
 *  @return     void
 */
void
cxss_free_rsa_keypair(char *privatekey, char *publickey)
{
    free(privatekey);
    free(publickey);
}

size_t
cxss_encrypt_rsa(const char *data, size_t len,
                 const char *publickey, size_t publickey_len,
                 char **ciphertext)
{
    size_t ciphertext_len;
    BIO *bio;
    RSA *rsa;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());       
    BIO_write(bio, publickey, publickey_len);

    /* read in the key */
    PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL);

    /* malloc */
    *ciphertext = malloc(4096);

    /* encrypt */
    ciphertext_len = RSA_public_encrypt(len, data, *ciphertext, rsa,
                                        RSA_PKCS1_OAEP_PADDING);

    if (ciphertext_len < 0) {
        fprintf(stderr, "Failed to encrypt (RSA)\n");
    }

    BIO_free(bio);
    RSA_free(rsa);
    return ciphertext_len;
}

char *
cxss_decrypt_rsa(const char *data, size_t len,
                 const char *privatekey, size_t privatekey_len)
{
    char *plaintext;
    BIO *bio;
    RSA *rsa;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());
    BIO_write(bio, privatekey, privatekey_len);

    PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL);
    
    plaintext = malloc(4096);

    /* decrypt */
    if (RSA_private_decrypt(len, data, plaintext, rsa, 
                            RSA_PKCS1_OAEP_PADDING) < 0) {
        fprintf(stderr, "Failed to decrypt (RSA)\n");
    } 

    BIO_free(bio);
    RSA_free(rsa);
    return plaintext;
}

