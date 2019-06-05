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
CXSS_DB_Context_t cxss_init_credentials_database(const char *dbpath);
void cxss_close_credentials_database(CXSS_DB_Context_t dbcontext);
int cxss_insert_userdata(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_insert_userauth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxss_insert_userresc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_retrieve_userdata(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
void cxss_free_userdata(CXSS_UserData *UserData);
int cxss_retrieve_userauth(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth *UserAuth);
int cxss_retrieve_userresc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id, CXSS_UserResc *UserResc);
int cxss_update_userdata(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_update_userresc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_delete_userdata(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
int cxss_delete_userresc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id);
void cxss_free_userauth(CXSS_UserAuth *UserAuth);
void cxss_free_userresc(CXSS_UserResc *UserResc);
int cxss_retrieve_userauth_ll(CXSS_DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth_LLNode **node);
void cxss_free_userauth_ll(CXSS_UserAuth_LLNode *start);
int cxss_get_user_count(CXSS_DB_Context_t dbcontext);
int cxss_get_userresc_count(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_db_contains_user(CXSS_DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_db_contains_resc(CXSS_DB_Context_t dbcontext, const char *resource_id);

/* Private Functions */
int cxss_setup_credentials_database(CXSS_DB_Context_t dbcontext);
void cxss_finalize_sqlite3_statements(CXSS_DB_Context_t dbcontext);

#endif /* CXSS_CREDENTIALS_DB_H */
