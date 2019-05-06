#ifndef CXSS_CREDENTIALS_DATABASE
#define CXSS_CREDENTIALS_DATABASE

#include <sqlite3.h>

/* DB Context struct */
typedef struct _DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count_stmt;
    sqlite3_stmt *get_user_pwd_count_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *retrieve_user_stmt;
    sqlite3_stmt *insert_user_auth_stmt;
    sqlite3_stmt *insert_resc_stmt;
    sqlite3_stmt *retrieve_resc_stmt;
} *DB_Context_t;

typedef struct {
    char *CXSS_UserID;
    char *PublicKey;
    size_t KeyLength;
    char *Salt;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserData;

typedef struct {
    char *CXSS_UserID;
    char *PrivateKey;
    size_t KeyLength;
    char *Salt;
    char *AuthClass;
    int RemovalFlag;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserAuth;

typedef struct {
    char *CXSS_UserID;
    char *ResourceID;
    char *ResourceSalt;
    char *ResourceUsername;
    size_t UsernameLength;
    char *ResourcePwd;
    size_t PwdLength;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserResc;

/* Public functions */
DB_Context_t cxss_init_credentials_database(const char *dbpath);
int cxss_close_credentials_database(DB_Context_t dbcontext);
int cxss_insert_user(DB_Context_t dbcontext, const char *cxss_userid, const char *publickey, size_t keylen, const char *salt, const char *date_created, const char *date_last_updated);
int cxss_insert_user_auth(DB_Context_t dbcontext, const char *cxss_userid, const char *privatekey, size_t keylen, const char *salt, const char *auth_class, int removal_flag, const char *date_created, const char *date_last_updated);
int cxss_insert_user_resc(DB_Context_t dbcontext, const char *cxss_userid, const char *resourceid, const char *resource_salt, const char *resource_username, size_t encr_username_len, const char *resource_pwd, size_t encr_password_len, const char *date_created, const char *date_last_updated);
int cxss_retrieve_user(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
void cxss_free_userdata(CXSS_UserData *UserData);

/* Private Functions */
static int cxss_setup_credentials_database(DB_Context_t dbcontext);
static void cxss_finalize_sqlite3_statements(DB_Context_t dbcontext);

#endif /* CXSS_CREDENTIALS_DATABASE */
