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

void cxssCryptoInit(void);
void cxssCryptoCleanup(void);
int cxssGenerate64bitSalt(char *salt);
int cxssGenerate128bitIV(char *init_vector);
int cxssGenerate256bitRandomKey(char *key);
int cxssGenerate256bitPasswordBasedKey(const char *password, const char *salt, char *key);
int cxssGenerateRSA4096bitKeypair(char **privatekey, int *privatekey_len, char **publickey, int *publickey_len);
int cxssEncryptAES256(const char *plaintext, int plaintext_len, const char *key, const char *init_vector, char **ciphertext, int *ciphertext_len);
int cxssDecryptAES256(const char *ciphertext, int ciphertext_len, const char *key, const char *init_vector, char **plaintext, int *plaintext_len);
size_t cxssAES256CiphertextLength(size_t plaintext_len);
int cxssEncryptRSA(const char *data, size_t len, const char *publickey, size_t publickey_len, char *ciphertext, int *ciphertext_len);
int cxssDecryptRSA(const char *data, size_t len, const char *privatekey, size_t privatekey_len, char *plaintext, int *plaintext_len);
void cxssDestroyKey(char *key, size_t keylength);

#endif /* CXSS_CRYPTO_H */
