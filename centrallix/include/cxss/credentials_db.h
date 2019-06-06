#ifndef CXSS_CREDENTIALS_DB_H
#define CXSS_CREDENTIALS_DB_H

#include <sqlite3.h>
#include <stdbool.h>

/* DB Context struct */
typedef struct _CXSS_DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count_stmt;
    sqlite3_stmt *get_user_resc_count_stmt;
    sqlite3_stmt *is_user_in_db_stmt;
    sqlite3_stmt *is_resc_in_db_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *retrieve_user_stmt;
    sqlite3_stmt *update_user_stmt;
    sqlite3_stmt *delete_user_stmt;
    sqlite3_stmt *insert_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auths_stmt;
    sqlite3_stmt *insert_resc_stmt;
    sqlite3_stmt *retrieve_resc_stmt;
    sqlite3_stmt *update_resc_stmt;
    sqlite3_stmt *delete_resc_stmt;
    sqlite3_stmt *delete_rescs_stmt;
} *CXSS_DB_Context_t;

typedef struct {
    const char *CXSS_UserID;
    const char *PublicKey;
    const char *DateCreated;
    const char *DateLastUpdated;
    size_t KeyLength;
} CXSS_UserData;

typedef struct {
    const char *CXSS_UserID;
    const char *Salt;
    const char *PrivateKey;
    const char *PrivateKeyIV;
    const char *DateCreated;
    const char *DateLastUpdated;
    bool RemovalFlag;
    size_t KeyLength;
    size_t SaltLength;
    size_t IVLength;
} CXSS_UserAuth;

typedef struct {
    const char *CXSS_UserID;
    const char *ResourceID;
    const char *AuthClass;
    const char *AESKey;
    const char *UsernameIV;
    const char *PasswordIV;
    const char *ResourceUsername;
    const char *ResourcePassword;
    const char *DateCreated;
    const char *DateLastUpdated;
    size_t AESKeyLength;
    size_t UsernameIVLength;
    size_t PasswordIVLength;
    size_t UsernameLength;
    size_t PasswordLength;
} CXSS_UserResc;

typedef struct _CXSS_LLNode {
    CXSS_UserAuth UserAuth;
    struct _CXSS_LLNode *next;
} CXSS_UserAuth_LLNode;

typedef enum {
    CXSS_DB_SETUP_ERROR = -3,
    CXSS_DB_BIND_ERROR = -2,
    CXSS_DB_QUERY_ERROR = -1,
    CXSS_DB_SUCCESS = 0
} CXSS_DB_Status_e;

/* Public functions */
CXSS_DB_Context_t cxss_initCredentialsDatabase(const char *dbpath);
void cxss_closeCredentialsDatabase(CXSS_DB_Context_t dbcontext);
int cxss_insertUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_insertUserAuth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxss_insertUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_retrieveUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
int cxss_retrieveUserAuth(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth *UserAuth);
int cxss_retrieveUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id, CXSS_UserResc *UserResc);
int cxss_updateUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_updateUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_deleteUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
int cxss_deleteUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id);
int cxss_deleteAllUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
void cxss_freeUserData(CXSS_UserData *UserData);
void cxss_freeUserAuth(CXSS_UserAuth *UserAuth);
void cxss_freeUserResc(CXSS_UserResc *UserResc);
int cxss_retrieveUserAuthLL(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth_LLNode **node);
void cxss_freeUserAuthLL(CXSS_UserAuth_LLNode *start);
int cxss_getUserCount(CXSS_DB_Context_t dbcontext);
int cxss_getUserRescCount(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_dbContainsUser(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_dbContainsResc(CXSS_DB_Context_t dbcontext, const char *resource_id);

#endif /* CXSS_CREDENTIALS_DB_H */
