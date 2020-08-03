#ifndef CXSS_CREDENTIALS_MGR_H
#define CXSS_CREDENTIALS_MGR_H

typedef enum {
    CXSS_MGR_INIT_ERROR = -6,
    CXSS_MGR_CLOSE_ERROR = -5,
    CXSS_MGR_INSERT_ERROR = -4,
    CXSS_MGR_RETRIEVE_ERROR = -3,
    CXSS_MGR_UPDATE_ERROR = -2,
    CXSS_MGR_DELETE_ERROR = -1,
    CXSS_MGR_SUCCESS = 0
} CXSS_MGR_Status_e; 

int cxssCredentialsManagerInit(void);
void cxssCredentialsManagerClose(void);
int cxssAddUser(const char *cxss_userid, const char *encryption_key, size_t encryption_key_length, const char *salt, size_t salt_len);
int cxssRetrieveUserPrivateKey(const char *cxss_userid, const char *user_key, size_t user_key_len, char **privatekey, int *privatekey_len);
int cxssRetrieveUserPublicKey(const char *cxss_userid, char **publickey, int *publickey_len);
int cxssDeleteUser(const char *cxss_userid);
int cxssAddResource(const char *cxss_userid, const char *resource_id, const char *auth_class, const char *resource_username, size_t username_len, const char *resource_password, size_t password_len);
int cxssGetResource(const char *cxss_userid, const char *resource_id, const char *user_key, size_t user_key_len, char **resource_username, char **resource_data);
int cxssDeleteResource(const char *cxss_userid, const char *resource_id);

#endif /* CXSS_CREDENTIALS_MGR_H */
