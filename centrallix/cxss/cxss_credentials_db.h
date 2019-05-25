#ifndef CXSS_CREDENTIALS_DB
#define CXSS_CREDENTIALS_DB

#include <sqlite3.h>
#include <stdbool.h>

/* DB Context struct */
typedef struct _DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count_stmt;
    sqlite3_stmt *get_user_resc_count_stmt;
    sqlite3_stmt *is_user_in_db_stmt;
    sqlite3_stmt *is_resc_in_db_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *retrieve_user_stmt;
    sqlite3_stmt *insert_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auths_stmt;
    sqlite3_stmt *insert_resc_stmt;
    sqlite3_stmt *retrieve_resc_stmt;
} *DB_Context_t;

typedef struct {
    const char *CXSS_UserID;
    const char *PublicKey;
    const char *DateCreated;
    const char *DateLastUpdated;
    size_t KeyLength;
} CXSS_UserData;

typedef struct {
    const char *CXSS_UserID;
    const char *PrivateKey;
    const char *Salt;
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

/* Public functions */
DB_Context_t cxss_init_credentials_database(const char *dbpath);
int cxss_close_credentials_database(DB_Context_t dbcontext);
int cxss_insert_userdata(DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_insert_userauth(DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxss_insert_userresc(DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_retrieve_userdata(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
void cxss_free_userdata(CXSS_UserData *UserData);
int cxss_retrieve_userauth(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth *UserAuth);
int cxss_retrieve_userresc(DB_Context_t dbcontext, const char *cxss_userid, const char *resource_id, CXSS_UserResc *UserResc);
void cxss_free_userauth(CXSS_UserAuth *UserAuth);
void cxss_free_userresc(CXSS_UserResc *UserResc);
int cxss_retrieve_userauth_ll(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth_LLNode **node);
void cxss_print_userauth_ll(CXSS_UserAuth_LLNode *start); /* debug function */
void cxss_free_userauth_ll(CXSS_UserAuth_LLNode *start);
int cxss_get_user_count(DB_Context_t dbcontext);
int cxss_get_userresc_count(DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_db_contains_user(DB_Context_t dbcontext, const char *cxss_userid);
bool cxss_db_contains_resc(DB_Context_t dbcontext, const char *resource_id);

/* Private Functions */
static int cxss_setup_credentials_database(DB_Context_t dbcontext);
static void cxss_finalize_sqlite3_statements(DB_Context_t dbcontext);
static inline CXSS_UserAuth_LLNode *cxss_allocate_userauth_node(void);

#endif /* CXSS_CREDENTIALS_DB */
