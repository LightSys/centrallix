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
 *  @return                     Length of AES-encrypted ciphertext
 */
size_t
cxss_aes256_ciphertext_length(size_t plaintext_len)
{
    return (plaintext_len + (16 - plaintext_len%16)); 
}

/** @brief Generate a 4096-bit RSA keypair
 *
 *  This function generates a 4096-bit RSA keypair
 *
 *  @param privatekey         Pointer to a 4096-bit buffer to store private key
 *  @param privatekey_len     Pointer to an int to store the size of private key
 *  @param publickey          Pointer to a 4096-bit buffer to store public key
 *  @param publickey_len      Pointer to an int to store the size of public key
 *  @return                   Status code
 */
int
cxss_generate_rsa_4096bit_keypair(char **privatekey, int *privatekey_len,
                                  char **publickey, int *publickey_len)
{
    RSA *rsa_keypair = NULL;
    BIGNUM *bne = NULL;
    BIO *pri = NULL;
    BIO *pub = NULL;
    unsigned long e = RSA_F4;
    int pri_len, pub_len;

    /* Generate bignum */
    bne = BN_new();
    if (BN_set_word(bne, e) != 1) {
        fprintf(stderr, "Failed to generate bignum\n");
        goto error;
    }

    /* Generate keypair */
    rsa_keypair = RSA_new();
    if (RSA_generate_key_ex(rsa_keypair, 4096, bne, NULL) != 1) {
        fprintf(stderr, "Failed to generate keypair\n");
        goto error;
    }
  
    /* Write keypair to BIO */ 
    pri = BIO_new(BIO_s_mem()); 
    pub = BIO_new(BIO_s_mem());
    if (!pri || !pub) {
        fprintf(stderr, "Memory allocation error!\n");
        goto error;
    }
    if (PEM_write_bio_RSAPrivateKey(pri, rsa_keypair, 
                                    NULL, NULL, 0, NULL, NULL) != 1) {
        fprintf(stderr, "Error while writing to BIO\n");
        goto error;
    }
    if (PEM_write_bio_RSAPublicKey(pub, rsa_keypair) != 1) {
        fprintf(stderr, "Error while writing to BIO\n");
        goto error;
    }

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);
    if (pri_len < 0 || pub_len < 0) {
        fprintf(stderr, "BIO error\n");
        goto error;
    }
    
    *privatekey = malloc(pri_len + 1);
    *publickey = malloc(pub_len + 1);
    if (!(*publickey) || !(*privatekey)) {
        fprintf(stderr, "Memory allocation error\n");
        goto error;
    } 

    /* Read from BIO and write into char buffer */
    if (BIO_read(pri, *privatekey, pri_len) < 0) {
        fprintf(stderr, "Error while reading from BIO\n");
        goto error;
    }
    if (BIO_read(pub, *publickey, pub_len) < 0) {
        fprintf(stderr, "Error while reading from BIO\n");
        goto error;
    }
 
    *privatekey_len = pri_len;
    *publickey_len = pub_len;

    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);    
    BN_free(bne);
    return 0;  

error:
    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);
    BN_free(bne);
    return -1;
} 

/** @brief Free rsa keypair
 *      
 *  Free dynamically-allocated key pair
 *
 *  @param privatekey   Privatekey
 *  @param publickey    Publickey
 *  @return             void
 */
void
cxss_free_rsa_keypair(char *privatekey, char *publickey)
{
    free(privatekey);
    free(publickey);
}

/** @brief Encrypt data with RSA
 *
 *  @param data                 Data to be encrypted
 *  @param len                  Length of data
 *  @param publickey            Public key
 *  @param publickey_len        Length of public key
 *  @param ciphertext           Pointer to char buffer (at least 4096 bytes)
 *  @param ciphertext_len       Pointer to variable to store ciphertext len
 *  @return                     Status code
 */
int
cxss_encrypt_rsa(const char *data, size_t len,
                 const char *publickey, size_t publickey_len,
                 char *ciphertext)
{
    int ciphertext_len;
    RSA *rsa = NULL;
    BIO *bio = NULL;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());
    if (!rsa || !bio) {
        fprintf(stderr, "Memory allocation error!\n");
        goto error;
    }
    
    /* write key to BIO */
    if (BIO_write(bio, publickey, publickey_len) != publickey_len) {
        fprintf(stderr, "Error while writing to BIO\n");
        goto error;
    }

    /* read key from BIO into RSA struct */
    if (PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL) == NULL) {
        fprintf(stderr, "RSA encrypt: error while reading from BIO\n");
        goto error;
    }

    /* encrypt */
    ciphertext_len = RSA_public_encrypt(len, data, ciphertext, rsa,
                                        RSA_PKCS1_OAEP_PADDING);
    if (ciphertext_len < 0) {
        fprintf(stderr, "RSA encryption failed\n");    
        goto error;
    }

    BIO_free(bio);
    RSA_free(rsa);
    return ciphertext_len;

error:
    BIO_free(bio);
    RSA_free(rsa);
    return -1;
}

int
cxss_decrypt_rsa(const char *data, size_t len,
                 const char *privatekey, size_t privatekey_len,
                 char *plaintext)
{
    int plaintext_len;
    BIO *bio = NULL;
    RSA *rsa = NULL;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());
    if (!rsa || !bio) {
        fprintf(stderr, "Memory allocation error\n");
        goto error;
    }

    /* Write private key to BIO */
    if (BIO_write(bio, privatekey, privatekey_len) != privatekey_len) {
        fprintf(stderr, "Failed to write to BIO\n");
        goto error;
    }

    /* Read private key from BIO into RSA struct */
    if (PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL) == NULL) {
        fprintf(stderr, "RSA decrypt: failed to read from BIO\n");
        goto error;
    }

    /* decrypt */
    plaintext_len = RSA_private_decrypt(len, data, plaintext, rsa, 
                                        RSA_PKCS1_OAEP_PADDING);
    if (plaintext_len < 0) {
        fprintf(stderr, "RSA decryption failed\n");
        goto error;
    } 

    BIO_free(bio);
    RSA_free(rsa);
    return plaintext_len;

error:
    BIO_free(bio);
    RSA_free(rsa);
    return -1;
}

