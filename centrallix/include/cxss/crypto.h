#ifndef CXSS_CRYPTO_H
#define CXSS_CRYPTO_H

typedef enum {
    CXSS_CRYPTO_IVGEN_ERROR = -5,
    CXSS_CRYPTO_KEYGEN_ERROR = -4,
    CXSS_CRYPTO_SALTGEN_ERROR = -3,
    CXSS_CRYPTO_ENCR_ERROR = -2,
    CXSS_CRYPTO_DECR_ERROR = -1,
    CXSS_CRYPTO_SUCCESS = 0
} CXSS_CRYPTO_Status_e;

void cxss_initializeCrypto(void);
void cxss_cleanupCrypto(void);
int cxss_generate64bitSalt(char *salt);
int cxss_generate128bitIV(char *init_vector);
int cxss_generate256bitRandomKey(char *key);
int cxss_generate256bitPasswordBasedKey(const char *password, const char *salt, char *key);
int cxss_generateRSA4096bitKeypair(char **privatekey, int *privatekey_len, char **publickey, int *publickey_len);
int cxss_encryptAes256(const char *plaintext, int plaintext_len, const char *key, const char *init_vector, char **ciphertext, int *ciphertext_len);
int cxss_decryptAes256(const char *ciphertext, int ciphertext_len, const char *key, const char *init_vector, char **plaintext, int *plaintext_len);
size_t cxss_aes256CiphertextLength(size_t plaintext_len);
int cxss_encryptRSA(const char *data, size_t len, const char *publickey, size_t publickey_len, char *ciphertext, int *ciphertext_len);
int cxss_decryptRSA(const char *data, size_t len, const char *privatekey, size_t privatekey_len, char *plaintext, int *plaintext_len);
void cxss_destroyRSAKeypair(char *privatekey, size_t privatekey_len, char *publickey, size_t publickey_len);

#endif /* CXSS_CRYPTO_H */
