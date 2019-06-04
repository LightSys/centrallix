#ifndef CXSS_CRYPTO_H
#define CXSS_CRYPTO_H

typedef enum {
    CXSS_CRYPTO_BYTES_ERROR = -3,
    CXSS_CRYPTO_ENCR_ERROR = -2,
    CXSS_CRYPTO_DECR_ERROR = -1,
    CXSS_CRYPTO_SUCCESS = 0
} CXSS_CRYPTO_Status_e;

void cxss_initialize_crypto(void);
void cxss_cleanup_crypto(void);
int cxss_generate_64bit_salt(unsigned char *salt);
int cxss_generate_128bit_iv(unsigned char *init_vector);
int cxss_generate_256bit_rand_key(unsigned char *key);
int cxss_generate_256bit_key(const char *password, const unsigned char *salt, unsigned char *key);
int cxss_generate_rsa_4096bit_keypair(char **privatekey, int *privatekey_len, char **publickey, int *publickey_len);
int cxss_encrypt_aes256(const char *plaintext, int plaintext_len, const unsigned char *key, const unsigned char *init_vector, unsigned char **ciphertext, int *ciphertext_len);
int cxss_decrypt_aes256(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key, const unsigned char *init_vector, char **plaintext, int *plaintext_len);
size_t cxss_aes256_ciphertext_length(size_t plaintext_len);
int cxss_encrypt_rsa(const unsigned char *data, size_t len, const char *publickey, size_t publickey_len, unsigned char *ciphertext);
int cxss_decrypt_rsa(const unsigned char *data, size_t len, const char *privatekey, size_t privatekey_len, char *plaintext);
void cxss_destroy_rsa_keypair(char *privatekey, size_t privatekey_len, char *publickey, size_t publickey_len);

#endif /* CXSS_CRYPTO_H */
