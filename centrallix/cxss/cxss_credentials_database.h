#ifndef CXSS_CREDENTIALS_DATABASE
#define CXSS_CREDENTIALS_DATABASE

#include <sqlite3.h>

typedef struct _DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count_stmt;
    sqlite3_stmt *get_user_pwd_count_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *retrieve_user_stmt;
    sqlite3_stmt *insert_resc_credentials_stmt;
    sqlite3_stmt *retrieve_resc_credentials_stmt;
} *DB_Context_t;

/* Public functions */
DB_Context_t cxss_init_credentials_database(const char *dbpath);
int cxss_close_credentials_database(DB_Context_t dbcontext);
int cxss_insert_userdata(DB_Context_t dbcontext, const char *cxss_userid, const char *salt, const char *date_created, const char *date_last_updated);


/* Private Functions */
static int cxss_setup_credentials_database(DB_Context_t dbcontext);
static void cxss_finalize_sqlite3_statements(DB_Context_t dbcontext);

#endif /* CXSS_CREDENTIALS_DATABASE */
