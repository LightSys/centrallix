#ifndef CXSS_CREDENTIALS_MGMT
#define CXSS_CREDENTIALS_MGMT

int cxss_init_credentials_mgmt(void);
int cxss_close_credentials_mgmt(void);
int cxss_adduser(const char *cxss_userid, const char *publickey, size_t keylength);
int cxss_adduser_auth(const char *cxss_userid, const char *user_key, size_t user_key_len, const char *salt, size_t salt_len, const char *privatekey, size_t privatekey_len);
char *cxss_retrieve_user_privatekey(const char *cxss_userid, const char *user_key, size_t user_key_len);

static char *get_timestamp(void);

#endif /* CXSS_CREDENTIALS_MGMT */
