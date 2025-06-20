#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "cxss/crypto.h"
#include "cxss/credentials_db.h"

static bool CSPRNG_Initialized = false;

/** @brief Initialize crypto module
 *
 *  This function should be called before any of the 
 *  other functions below. It takes care of initializing
 *  and seeding OpenSSL's random number generator, 
 *  which is required to perform most crypto operations.
 *
 *  @return     void
 */
void
cxssCryptoInit(void)
{
    char seed[256];

    /* Generate seed and init OpenSSL RNG */
    cxss_internal_GetBytes(seed, 256); 
    RAND_seed(seed, 256);
    CSPRNG_Initialized = true;
}        

/** @brief Cleanup crypto module
 *
 *  This function should be called after all
 *  crypto operations are done (when functions
 *  from this module are no longer needed).
 *
 *  @return     void
 */
void
cxssCryptoCleanup(void)
{
    EVP_cleanup();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    ERR_remove_thread_state(NULL); /* can't remove this yet if we might still build against 1.0.x */
#pragma GCC diagnostic pop
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();
}

/** @brief Encrypt data using AES256
 *
 *  This function is used to encrypt data using the AES symmetric block cipher
 *  with a 256-bit key. The current mode of operation is CBC (Cipher Block Chaining).
 *
 *  @param plaintext            Pointer to a buffer containing the data to encrypt
 *  @param plaintext_len        Length of the data to encrypt
 *  @param key                  256-bit AES key
 *  @param init_vector          128-bit initialization vector
 *  @param ciphertext           Pointer to pointer to a buffer to store encrypted data
 *  @param ciphertext_len       Pointer to a variable to store length of encrypted data
 *  @return                     Status code
 */
int 
cxssEncryptAES256(const char *plaintext, int plaintext_len, 
                  const char *key, const char *init_vector,
                  char **ciphertext, int *ciphertext_len)
{
    assert(CSPRNG_Initialized);
    EVP_CIPHER_CTX *ctx = NULL;
    int len;

    /* Allocate buffer to store ciphertext */
    *ciphertext = malloc(cxssAES256CiphertextLength(plaintext_len));
    if (!(*ciphertext)) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }
    
    /* Create openssl cipher context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        mssError(0, "CXSS", "Failed to create new openssl cipher context\n");
        goto error;
    }

    /* Initiate encryption */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)key, 
                           (unsigned char *)init_vector) != 1) {
        mssError(0, "CXSS", "Error while initiating AES encryption\n");
        goto error;
    }

    /* Encrypt data */
    if (EVP_EncryptUpdate(ctx, *(unsigned char **)ciphertext, &len, 
                          (unsigned char *)plaintext, plaintext_len) != 1) {
        mssError(0, "CXSS", "Error while encrypting with AES\n");
        goto error;
    }
    *ciphertext_len = len;

    /* Finalize encryption */
    if (EVP_EncryptFinal_ex(ctx, *(unsigned char **)ciphertext + len, &len) != 1) {
        mssError(0, "CXSS", "Error while finalizing AES encryption\n");
        goto error;
    }
    *ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return CXSS_CRYPTO_SUCCESS;

error:
    EVP_CIPHER_CTX_free(ctx);
    free(*ciphertext);
    return CXSS_CRYPTO_ENCR_ERROR;
}

/** @brief Decrypt data using AES256
 *
 *  This function is used to decrypt data that has been encrypted using
 *  the AES symmetric block cipher with a 256-bit key. The current mode 
 *  of operation is CBC (Cipher Block Chaining).
 *
 *  @param ciphertext           Pointer to a buffer containing the data to decrypt
 *  @param ciphertext_len       Length of the data to decrypt
 *  @param key                  256-bit AES key
 *  @param init_vector          128-bit initialization vector
 *  @param plaintext            Pointer to pointer to a buffer to store the decrypted data
 *  @param plaintext_len        Pointer to a variable to store the length of the decrypted data
 *  @return                     Status code
 */
int
cxssDecryptAES256(const char *ciphertext, int ciphertext_len,
                  const char *key, const char *init_vector,
                  char **plaintext, int *plaintext_len)
{
    assert(CSPRNG_Initialized);
    EVP_CIPHER_CTX *ctx = NULL;
    int len;
 
    /* Allocate buffer to store plaintext */
    *plaintext = malloc(cxssAES256CiphertextLength(ciphertext_len));
    if (!(*plaintext)) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }

    /* Create new openssl cipher context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        mssError(0, "CXSS", "Failed to create new openssl cipher context\n");
        goto error;
    }

    /* Initiate decryption */
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char *)key,
                           (unsigned char *)init_vector) != 1) {
        mssError(0, "CXSS", "Error while initiating AES decryption\n");
        goto error;
    }

    /* Decrypt data */
    if (EVP_DecryptUpdate(ctx, *(unsigned char **)plaintext, &len, 
                          (unsigned char *)ciphertext, ciphertext_len) != 1) {
        mssError(0, "CXSS", "Error while decrypting with AES\n");
        goto error;
    }
    *plaintext_len = len;

    /* Finalize decryption */
    if (EVP_DecryptFinal_ex(ctx, *(unsigned char **)plaintext + len, &len) != 1) {
        mssError(0, "CXSS", "Error while finalizing AES decryption!\n");
        goto error;
    }
    *plaintext_len += len;   
        
    /* Cleanup */
    EVP_CIPHER_CTX_free(ctx);
    return CXSS_CRYPTO_SUCCESS;

error:
    EVP_CIPHER_CTX_free(ctx);
    free(*plaintext);
    return CXSS_CRYPTO_DECR_ERROR;
}            

/** @brief Generate 64-bit random salt
 *
 *  Generate a 64-bit random salt using OpenSSL's 
 *  cryptographically-secure random number generator.
 *
 *  @param      Pointer to 64-bit buffer to store the salt
 *  @return     Status code
 */
int
cxssGenerate64bitSalt(char *salt)
{
    assert(CSPRNG_Initialized);

    if (RAND_bytes((unsigned char *)salt, 8) < 0) {
        mssError(0, "CXSS", "Failed to generate salt!\n");
        return CXSS_CRYPTO_SALTGEN_ERROR;
    }
    return CXSS_CRYPTO_SUCCESS;
}

/** @brief Generate 128-bit random initialization vector
 *
 *  Generate a 128-bit random initialization vector,
 *  suitable for AES, using OpenSSL's cryptographically-secure
 *  random number generator.
 *
 *  @param      Pointer to 128-bit buffer to store the iv
 *  @return     Status code
 */
int
cxssGenerate128bitIV(char *init_vector)
{
    assert(CSPRNG_Initialized);

    /* Generate random initialization vector */
    if (RAND_bytes((unsigned char *)init_vector, 16) != 1) {
        mssError(0, "CXSS", "Failed to generate initialization vector!\n");
        return CXSS_CRYPTO_IVGEN_ERROR;
    }
    return CXSS_CRYPTO_SUCCESS;
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
cxssGenerate256bitPasswordBasedKey(const char *password, const char *salt, 
                                   char *key)
{
    #define PBKDF2_ITER_NO      5000
    assert(CSPRNG_Initialized);   
 
    if (PKCS5_PBKDF2_HMAC(password, strlen(password), (unsigned char *)salt, 8, PBKDF2_ITER_NO, 
                          EVP_sha256(), 32, (unsigned char *)key) != 1) {
        mssError(0, "CXSS", "Failed to generate 256-bit key\n");
        return CXSS_CRYPTO_KEYGEN_ERROR;
    }
    return CXSS_CRYPTO_SUCCESS;
}

/** @brief Generate 256-bit random key 
 *
 *  Generate a 256-bit random key, suitable for AES-256. 
 *
 *  @param key          Pointer to a 256-bit buffer to store the key
 *  @return             Status code
 */
int
cxssGenerate256bitRandomKey(char *key)
{
    assert(CSPRNG_Initialized);

    /* Generate random initialization vector */
    if (RAND_bytes((unsigned char *)key, 32) != 1) {
        mssError(0, "CXSS", "Failed to generate key!\n");
        return CXSS_CRYPTO_KEYGEN_ERROR;
    }
    return CXSS_CRYPTO_SUCCESS;
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
cxssAES256CiphertextLength(size_t plaintext_len)
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
cxssGenerateRSA4096bitKeypair(char **privatekey, int *privatekey_len,
                              char **publickey, int *publickey_len)
{
    assert(CSPRNG_Initialized);
    BIGNUM *bne = NULL;
    RSA *rsa_keypair = NULL;
    BIO *pri = NULL;
    BIO *pub = NULL;
    long e = RSA_F4;
    int pri_len, pub_len;

    /* Generate bignum */
    bne = BN_new();
    if (BN_set_word(bne, e) != 1) {
        mssError(0, "CXSS", "Failed to generate bignum\n");
        goto error;
    }

    /* Generate keypair */
    rsa_keypair = RSA_new();
    if (RSA_generate_key_ex(rsa_keypair, 4096, bne, NULL) != 1) {
        mssError(0, "CXSS", "Failed to generate keypair\n");
        goto error;
    }
  
    /* Write keypair to BIO */ 
    pri = BIO_new(BIO_s_mem()); 
    pub = BIO_new(BIO_s_mem());
    if (!pri || !pub) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }
    if (PEM_write_bio_RSAPrivateKey(pri, rsa_keypair, 
                                    NULL, NULL, 0, NULL, NULL) != 1) {
        mssError(0, "CXSS", "Error while writing to BIO\n");
        goto error;
    }
    if (PEM_write_bio_RSAPublicKey(pub, rsa_keypair) != 1) {
        mssError(0, "CXSS", "Error while writing to BIO\n");
        goto error;
    }

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);
    if (pri_len < 0 || pub_len < 0) {
        mssError(0, "CXSS", "BIO error\n");
        goto error;
    }
    
    *privatekey = malloc(pri_len + 1);
    *publickey = malloc(pub_len + 1);
    if (!(*publickey) || !(*privatekey)) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    } 

    /* Read keys from BIO into char buffer */
    if (BIO_read(pri, *privatekey, pri_len) < 0) {
        mssError(0, "CXSS", "Error while reading from BIO\n");
        goto error;
    }
    if (BIO_read(pub, *publickey, pub_len) < 0) {
        mssError(0, "CXSS", "Error while reading from BIO\n");
        goto error;
    }
 
    *privatekey_len = pri_len;
    *publickey_len = pub_len;

    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);    
    BN_free(bne);
    return CXSS_CRYPTO_SUCCESS;  

error:
    BIO_free_all(pri);
    BIO_free_all(pub);
    RSA_free(rsa_keypair);
    BN_free(bne);
    return CXSS_CRYPTO_KEYGEN_ERROR;
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
cxssEncryptRSA(const char *data, size_t len,
               const char *publickey, size_t publickey_len,
               char *ciphertext, int *ciphertext_len)
{
    assert(CSPRNG_Initialized);
    RSA *rsa = NULL;
    BIO *bio = NULL;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());
    if (!rsa || !bio) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }
    
    /* Write key to BIO */
    if (BIO_write(bio, publickey, publickey_len) != publickey_len) {
        mssError(0, "CXSS", "Error while writing to BIO\n");
        goto error;
    }

    /* Read key from BIO into RSA struct */
    if (PEM_read_bio_RSAPublicKey(bio, &rsa, NULL, NULL) == NULL) {
        mssError(0, "CXSS", "RSA encrypt: error while reading from BIO\n");
        goto error;
    }

    /* encrypt */
    *ciphertext_len = RSA_public_encrypt(len, (unsigned char *)data, (unsigned char *)ciphertext, rsa,
                                         RSA_PKCS1_OAEP_PADDING);
    if (*ciphertext_len < 0) {
        mssError(0, "CXSS", "RSA encryption failed\n");    
        goto error;
    }

    BIO_free(bio);
    RSA_free(rsa);
    return CXSS_CRYPTO_SUCCESS;

error:
    BIO_free(bio);
    RSA_free(rsa);
    return CXSS_CRYPTO_ENCR_ERROR;
}

int
cxssDecryptRSA(const char *data, size_t len,
               const char *privatekey, size_t privatekey_len,
               char *plaintext, int *plaintext_len)
{
    assert(CSPRNG_Initialized);
    BIO *bio = NULL;
    RSA *rsa = NULL;

    rsa = RSA_new();
    bio = BIO_new(BIO_s_mem());
    if (!rsa || !bio) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }

    /* Write private key to BIO */
    if (BIO_write(bio, privatekey, privatekey_len) != privatekey_len) {
        mssError(0, "CXSS", "Failed to write to BIO\n");
        goto error;
    }

    /* Read private key from BIO into RSA struct */
    if (PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL) == NULL) {
        mssError(0, "CXSS", "RSA decrypt: failed to read from BIO\n");
        goto error;
    }

    /* decrypt */
    *plaintext_len = RSA_private_decrypt(len, (unsigned char *)data, (unsigned char *)plaintext, rsa, 
                                         RSA_PKCS1_OAEP_PADDING);
    if (*plaintext_len < 0) {
        mssError(0, "CXSS", "RSA decryption failed\n");
        goto error;
    } 

    BIO_free(bio);
    RSA_free(rsa);
    return CXSS_CRYPTO_SUCCESS;

error:
    BIO_free(bio);
    RSA_free(rsa);
    return CXSS_CRYPTO_DECR_ERROR;
}

/** @brief Erase and deallocate cryptographic key 
 *      
 *  This function is called in the credentials manager to safely destroy a
 *  dynamically-allocated cryptographic key after it has been used.
 *
 *  @param key          Key to destroy
 *  @param keylength    Length of the key
 *  @return             void
 */
void
cxssDestroyKey(char *key, size_t keylength)
{
    if (key && keylength >= 0) {
        memset(key, 0, keylength);
        free(key);
    }
}

