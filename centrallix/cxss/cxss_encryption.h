#ifndef CXSS_ENCRYPTION
#define CXSS_ENCRYPTION

/* Public functions */
int cxss_generate_64bit_salt(char *salt);
int cxss_generate_128bit_iv(char *init_vector);
int cxss_generate_256bit_key(const char *password, const char *salt, char *key);
int cxss_encrypt_aes256(const char *plaintext, int plaintext_len, const char *key, const char *init_vector, char *ciphertext);
int cxss_decrypt_aes256(const char *ciphertext, int ciphertext_len, const char *key, const char *init_vector, char *plaintext);

/* Private functions */
void cxss_initialize_csprng(void);

#endif /* CXSS_ENCRYPTION */
