#ifndef CXSS_CREDENTIALS_DB
#define CXSS_CREDENTIALS_DB

#include <sqlite3.h>

/* DB Context struct */
typedef struct _DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count_stmt;
    sqlite3_stmt *get_user_resc_count_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *retrieve_user_stmt;
    sqlite3_stmt *insert_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auth_stmt;
    sqlite3_stmt *retrieve_user_auths_stmt;
    sqlite3_stmt *insert_resc_stmt;
    sqlite3_stmt *retrieve_resc_stmt;
} *DB_Context_t;

typedef struct {
    char *CXSS_UserID;
    char *PublicKey;
    size_t KeyLength;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserData;

typedef struct {
    char *CXSS_UserID;
    char *PrivateKey;
    size_t KeyLength;
    char *Salt;
    size_t SaltLength;
    char *UserIV;
    size_t IVLength;
    char *AuthClass;
    int RemovalFlag;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserAuth;

typedef struct {
    char *CXSS_UserID;
    char *ResourceID;
    char *ResourceSalt;
    size_t SaltLength;
    char *UsernameIV;
    size_t UsernameIVLength;
    char *PasswordIV;
    size_t PasswordIVLength;
    char *ResourceUsername;
    size_t UsernameLength;
    char *ResourcePassword;
    size_t PasswordLength;
    char *DateCreated;
    char *DateLastUpdated;
} CXSS_UserResc;

typedef struct _CXSS_LLNode {
    CXSS_UserAuth UserAuth;
    struct _CXSS_LLNode *next;
} CXSS_UserAuth_LLNode;

/* Public functions */
DB_Context_t cxss_init_credentials_database(const char *dbpath);
int cxss_close_credentials_database(DB_Context_t dbcontext);
int cxss_insert_user(DB_Context_t dbcontext, CXSS_UserData *UserData);
int cxss_insert_user_auth(DB_Context_t dbcontext, CXSS_UserAuth *UserAuth);
int cxss_insert_user_resc(DB_Context_t dbcontext, CXSS_UserResc *UserResc);
int cxss_retrieve_user(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserData *UserData);
void cxss_free_userdata(CXSS_UserData *UserData);
int cxss_retrieve_user_auth(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth *UserAuth);
void cxss_free_userauth(CXSS_UserAuth *UserAuth);
int cxss_retrieve_user_auths(DB_Context_t dbcontext, const char *cxss_userid, CXSS_UserAuth_LLNode **node);
void cxss_print_userauth_ll(CXSS_UserAuth_LLNode *start);
void cxss_free_userauth_ll(CXSS_UserAuth_LLNode *start);
int cxss_get_user_count(DB_Context_t dbcontext);
int cxss_get_user_resc_count(DB_Context_t dbcontext, const char *cxss_userid);

/* Private Functions */
static int cxss_setup_credentials_database(DB_Context_t dbcontext);
static void cxss_finalize_sqlite3_statements(DB_Context_t dbcontext);
static inline CXSS_UserAuth_LLNode *cxss_allocate_userauth_node(void);
static inline char *cxss_strdup(const char *str);
static inline char *cxss_blobdup(const char *str, size_t len);

#endif /* CXSS_CREDENTIALS_DB */
