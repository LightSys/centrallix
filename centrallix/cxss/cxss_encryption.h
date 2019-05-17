#ifndef CXSS_ENCRYPTION
#define CXSS_ENCRYPTION

/* Public functions */
int cxss_generate_64bit_salt(char *salt);
int cxss_generate_128bit_iv(char *init_vector);
int cxss_generate_256bit_rand_key(char *key);
int cxss_generate_256bit_key(const char *password, const char *salt, char *key);
int cxss_generate_rsa_4096bit_keypair(char *privatekey, int *privatekey_len, char *publickey, int *publickey_len);
int cxss_encrypt_aes256(const char *plaintext, int plaintext_len, const char *key, const char *init_vector, char *ciphertext);
int cxss_decrypt_aes256(const char *ciphertext, int ciphertext_len, const char *key, const char *init_vector, char *plaintext);
size_t cxss_aes256_ciphertext_length(size_t plaintext_len);
int cxss_encrypt_rsa(const char *data, size_t len, const char *publickey, size_t publickey_len, char *ciphertext);
char *cxss_decrypt_rsa(const char *data, size_t len, const char *privatekey, size_t privatekey_len);

/* Private functions */
void cxss_initialize_crypto(void);
void cxss_cleanup_crypto(void);

#endif /* CXSS_ENCRYPTION */
