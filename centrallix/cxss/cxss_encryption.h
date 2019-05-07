#ifndef CXSS_ENCRYPTION
#define CXSS_ENCRYPTION

/* Public functions */
int cxss_generate_64bit_salt(char *salt);
int cxss_generate_256bit_key(const char *password, const char *salt, char *key);
int cxss_encrypt_aes256(const char *plaintext, int plaintext_len, const char *key, const char *ciphertext);

/* Private functions */
void cxss_initialize_csprng(void);

#endif /* CXSS_ENCRYPTION */
