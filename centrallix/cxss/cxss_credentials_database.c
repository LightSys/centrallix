#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "cxss_credentials_database.h"


/** @brief Initialize cxss credentials database
 *
 *  This function is called by the "client" in order
 *  to prepare the database environment and its related
 *  data structures.
 *
 *  @param db_path      Path to database file
 *  @return             Database context handle
 */
DB_Context_t
cxss_init_credentials_database(const char *db_path)
{
    struct _DB_Context_t *dbcontext;

    /* Allocate context struct */
    dbcontext = malloc(sizeof(struct _DB_Context_t));
    if (!dbcontext) {
        fprintf(stderr, "Memory allocation error!\n");
        return NULL;
    }

    /* Open database file */
    if (sqlite3_open(db_path, &dbcontext->db) != SQLITE_OK) {
        fprintf(stderr, "Unable to open database: %s\n", 
                        sqlite3_errmsg(dbcontext->db));
        goto error;
    }

    /* Setup tables/statements */
    if (cxss_setup_credentials_database(dbcontext) < 0) {
        fprintf(stderr, "Failed to setup database!\n");
        goto error;
    }

    return dbcontext;
error:
    free(dbcontext);
    return (DB_Context_t)NULL;
}

/** @brief Cleanup and close credentials database
 *
 *  This function closes the sqlite3 database and
 *  deallocates the database context struct. 
 *
 *  @param dbcontext    Database context handle
 *  @return             Status code
 */
int cxss_close_credentials_database(DB_Context_t dbcontext)
{
    cxss_finalize_sqlite3_statements(dbcontext);
    sqlite3_close(dbcontext->db);
    free(dbcontext);
}

/** @brief Setup database tables/statements
 *
 *  This function creates the required tables
 *  to store the user's credentials (if they 
 *  don't already exists in the database) and
 *  pre-compiles some SQL queries into sqlite3
 *  statements.
 *
 *  @param dbcontext    Database context handle
 *  @return             Status code
 */
static int
cxss_setup_credentials_database(DB_Context_t dbcontext)
{   
    char *err_msg = NULL;

    /* Create Tables */
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserData("
                 "CXSS_UserID TEXT PRIMARY KEY,"
                 "UserSalt TEXT,"
                 "UserPublicKey BLOB,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserAuth("
                 "PK_UserAuth INTEGER PRIMARY KEY,"
                 "CXSS_UserID TEXT,"
                 "AuthClass TEXT,"
                 "UserSalt TEXT,"
                 "UserPrivateKey BLOB,"
                 "RemovalFlag INT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);

    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserResc("
                 "ResourceID TEXT PRIMARY KEY,"
                 "ResourceSalt TEXT,"
                 "ResourceUsername TEXT,"
                 "ResourcePassword BLOB,"
                 "CXSS_UserID TEXT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);

    if (err_msg) {
        fprintf(stderr, "SQL Error: %s\n", err_msg);
        goto error;
    }

    /* Compile SQL statements */
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT(*) FROM UserData;",
                    -1, &dbcontext->get_user_count_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT (*) FROM UserAuth"
                    "WHERE CXSS_UserID=?;",
                    -1, &dbcontext->get_user_pwd_count_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserData(CXSS_UserID, UserSalt, UserPublicKey,"
                    "DateCreated, DateLastUpdated) VALUES(?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserSalt, UserPublicKey," 
                    "DateCreated, DateLastUpdated FROM UserData"
                    "WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserAuth(CXSS_UserID, AuthClass, UserSalt,"
                    "UserPrivateKey, RemovalFlag, DateCreated, DateLastUpdated)"
                    "VALUES (?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_auth_stmt, NULL);
    
    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserResc (ResourceID, ResourceSalt,"
                    "ResourceUsername, ResourcePassword, CXSS_UserID,"
                    "DateCreated, DateLastUpdated)"
                    "VALUES (?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_resc_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT ResourceSalt, ResourceUsername, ResourcePassowrd,"
                    "DateCreated, DateLastUpdated FROM UserResc"
                    "WHERE CXSS_UserID=? AND ResourceID=?;",
                    -1, &dbcontext->retrieve_resc_stmt, NULL);

    return 0;
error:
    sqlite3_free(err_msg);
    sqlite3_close(dbcontext->db);
    return -1;
}

/** @brief Cleanup sqlite3 statements
 *
 *  The pre-compiled sqlite3 statements need
 *  to be cleaned up. This function does that.
 *
 *  @param dbcontext    Database context handle
 *  @return             void         
 */
static void 
cxss_finalize_sqlite3_statements(DB_Context_t dbcontext)
{
    sqlite3_finalize(dbcontext->get_user_count_stmt);
    sqlite3_finalize(dbcontext->get_user_pwd_count_stmt);
    sqlite3_finalize(dbcontext->insert_user_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_stmt);
    sqlite3_finalize(dbcontext->insert_user_auth_stmt);
    sqlite3_finalize(dbcontext->insert_resc_stmt);
    sqlite3_finalize(dbcontext->retrieve_resc_stmt);
}

/** @brief Insert new user
 *
 *  Create new user entry in 'UserData' table
 *
 *  @param dbcontext            Database context handle
 *  @param cxss_userid          CXSS user identity
 *  @param publickey            User public key
 *  @param keylen               Length of public key
 *  @param salt                 User-specific salt
 *  @param date_created         Date first created
 *  @param date_last_updated    Date last updated
 *  @return                     Status code 
 */
int
cxss_insert_user(DB_Context_t dbcontext, const char *cxss_userid, 
                 const char *publickey, size_t keylen, const char *salt,
                 const char *date_created, const char *date_last_updated)
{
    /* Bind data with sqlite3 stmts */
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 1, 
                          cxss_userid, -1, NULL) != SQLITE_OK)
        goto bind_error;

    if (sqlite3_bind_blob(dbcontext->insert_user_stmt, 2,
                          publickey, keylen, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 3, 
                          salt, -1, NULL) != SQLITE_OK)
        goto bind_error; 

    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 4,
                          date_created, -1, NULL) != SQLITE_OK)
        goto bind_error;

    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 5,
                          date_last_updated, -1, NULL) != SQLITE_OK)
        goto bind_error;
    
    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user\n");
        return -1;
    }

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n", 
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

/** @brief Insert user auth data
 *
 *  Create user auth entry in 'UserAuth' table
 *
 *  @param dbcontext            Database context handle
 *  @param cxss_userid          CXSS user identity
 *  @param privatekey           User private key (encrypted)
 *  @param encr_keylen          Length of encrypted private key
 *  @param salt                 User-specific salt
 *  @param auth_class           Authentication class ("password", "oauth", ...)
 *  @param remove_flag          Removal flag
 *  @param date_created         Date first created
 *  @param date_last_updated    Date last updated
 *  @return                     Status code 
 */
int
cxss_insert_user_auth(DB_Context_t dbcontext, const char *cxss_userid,
                      const char *privatekey, size_t encr_keylen, 
                      const char *salt, const char *auth_class, int remove_flag,
                      const char *date_created, const char *date_last_updated)
{
    /* Bind data with sqlite3 stmts */    
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 2,
                          privatekey, encr_keylen, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 3,
                          salt, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 4,
                          auth_class, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_int(dbcontext->insert_user_auth_stmt, 5,
                         remove_flag) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 6,
                          date_created, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 7,
                          date_last_updated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_auth_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user auth\n");
        return -1;
    }

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

/** @brief Insert user resource
 *
 *  Create resource entry in 'UserResc' table
 *
 *  @param dbcontext            Database context handle
 *  @param cxss_userid          CXSS user identity
 *  @param resourceid           Resource identification (must be unique)
 *  @param resource_salt        Resource-specific salt
 *  @param resource_username    Resource username (encrypted)
 *  @param resource_pwd         Resource password (encrypted)
 *  @param date_created         Date first created
 *  @param date_last_updated    Date last updated
 *  @return                     Status code 
 */
int
cxss_insert_user_resc(DB_Context_t dbcontext, const char *cxss_userid,
                      const char *resourceid, const char *resource_salt, 
                      const char *resource_username, size_t encr_username_len,
                      const char *resource_pwd, size_t encr_password_len,
                      const char *date_created, const char *date_last_updated)
{
    /* Bind data with sqlite3 stmts */    
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 1,
                          resourceid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 2,
                          resource_salt, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 3,
                          resource_username, encr_username_len, 
                          NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 4,
                         resource_pwd, encr_password_len, 
                         NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 5,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 6,
                          date_created, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 7,
                          date_last_updated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_resc_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user auth\n");
        return -1;
    }

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}
 
