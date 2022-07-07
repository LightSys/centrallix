#ifndef CXSS_CREDENTIALS_DB_H
#define CXSS_CREDENTIALS_DB_H

#include <sqlite3.h>
#include <stdbool.h>
#include <errno.h>

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
    sqlite3_stmt *update_auth_stmt;
    sqlite3_stmt *retrieve_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auths_stmt;
    sqlite3_stmt *delete_user_auth_stmt;
    sqlite3_stmt *delete_user_auths_stmt;
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
     int PK_UserAuth;
    const char *CXSS_UserID;
    const char *AuthClass;
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
    const char *AESKey;
    const char *UsernameIV;
    const char *AuthDataIV;
    const char *ResourceUsername;
    const char *ResourceAuthData;
    const char *DateCreated;
    const char *DateLastUpdated;
    size_t AESKeyLength;
    size_t UsernameIVLength;
    size_t AuthDataIVLength;
    size_t UsernameLength;
    size_t AuthDataLength;
} CXSS_UserResc;

typedef struct _CXSS_LLNode {
    CXSS_UserAuth UserAuth;
    struct _CXSS_LLNode *next;
} CXSS_UserAuth_LLNode;

typedef enum {
    CXSS_DB_NOENT_ERROR = -ENOENT,
    CXSS_DB_SETUP_ERROR = -3,
    CXSS_DB_BIND_ERROR = -2,
    CXSS_DB_QUERY_ERROR = -1,
    CXSS_DB_SUCCESS = 0
} CXSS_DB_Status_e;

CXSS_DB_Context_t cxssCredentialsDatabaseInit(const char *dbpath);
void cxssCredentialsDatabaseClose(CXSS_DB_Context_t dbcontext);
int cxssInsertUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxssInsertUserAuth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxssInsertUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxssRetrieveUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
int cxssRetrieveUserAuth(CXSS_DB_Context_t dbcontext, int pk_userAuth, CXSS_UserAuth *UserAuth);
int cxssRetrieveUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id, CXSS_UserResc *UserResc);
int cxssUpdateUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxssUpdateUserAuth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxssUpdateUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxssDeleteUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
int cxssDeleteUserAuth(CXSS_DB_Context_t dbcontext, int pk_userAuth);
int cxssDeleteUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id);
int cxssDeleteAllUserAuth(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
int cxssDeleteAllUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
void cxssFreeUserData(CXSS_UserData *UserData);
void cxssFreeUserAuth(CXSS_UserAuth *UserAuth);
void cxssFreeUserResc(CXSS_UserResc *UserResc);
int cxssRetrieveUserAuthLL(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth_LLNode **node);
void cxssFreeUserAuthLL(CXSS_UserAuth_LLNode *start);
int cxssGetUserCount(CXSS_DB_Context_t dbcontext);
int cxssGetUserRescCount(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxssDbContainsUser(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxssDbContainsResc(CXSS_DB_Context_t dbcontext, const char *resource_id);

#endif /* CXSS_CREDENTIALS_DB_H */
