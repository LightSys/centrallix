#ifndef CXSS_CREDENTIALS_DATABASE
#define CXSS_CREDENTIALS_DATABASE

#include <sqlite3.h>

typedef struct _DB_Context_t {
    sqlite3 *db;
    sqlite3_stmt *get_user_count;
    sqlite3_stmt *get_user_pwd_count;
    sqlite3_stmt *insert_user;
    sqlite3_stmt *retrieve_user;
    sqlite3_stmt *insert_resc_credentials;
    sqlite3_stmt *retrieve_resc_credentials;
} *DB_Context_t;

/* Public functions */
DB_Context_t cxss_init_credentials_database(const char *dbpath);
int cxss_close_credentials_database(DB_Context_t dbcontext);

/* Private Functions */
static int cxss_setup_credentials_database(DB_Context_t dbcontext);
static void cxss_finalize_sqlite3_statements(DB_Context_t dbcontext);

#endif /* CXSS_CREDENTIALS_DATABASE */
