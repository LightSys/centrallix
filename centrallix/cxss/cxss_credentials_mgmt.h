#ifndef CXSS_CREDENTIALS_MGMT
#define CXSS_CREDENTIALS_MGMT

int cxss_init_credentials_mgmt(void);
int cxss_close_credentials_mgmt(void);
int cxss_adduser(const char *cxss_userid, const char *encryption_key, size_t encryption_key_length, const char *salt, size_t salt_len);
char *cxss_retrieve_user_privatekey(const char *cxss_userid, const char *user_key, size_t user_key_len);
char *cxss_retrieve_user_publickey(const char *cxss_userid);
int cxss_add_resource(const char *cxss_userid, const char *resource_id, const char *resource_username, size_t username_len, const char *resource_password, size_t password_len);
int cxss_get_resource(const char *cxss_userid, const char *resource_id);

static char *get_timestamp(void);

#endif /* CXSS_CREDENTIALS_MGMT */
