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
                 "UserID TEXT PRIMARY KEY,"
                 "UserSalt TEXT,"
                 "UserPublicKey TEXT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserAuth("
                 "AuthClass TEXT,"
                 "UserSalt TEXT,"
                 "UserPrivateKey TEXT,"
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
                 "UserID TEXT,"
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
                    -1, &dbcontext->get_user_count, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT (*) FROM UserAuth"
                    "WHERE UserID=?;",
                    -1, &dbcontext->get_user_pwd_count, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserData(UserID, UserSalt, UserPublicKey,"
                    "DateCreated, DateLastUpdated) VALUES(?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserSalt, UserPublicKey FROM UserData"
                    "WHERE UserID=?;",
                    -1, &dbcontext->retrieve_user, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserResc (ResourceID, ResourceSalt,"
                    "ResourceUsername, ResourcePassword, UserID, DateCreated,"
                    "DateLastUpdated) VALUES (?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_credentials, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT ResourceUsername, ResourcePassowrd FROM UserResc"
                    "WHERE UserID=? AND ResourceID=?;",
                    -1, &dbcontext->retrieve_credentials, NULL);

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
    sqlite3_finalize(dbcontext->get_user_count);
    sqlite3_finalize(dbcontext->get_user_pwd_count);
    sqlite3_finalize(dbcontext->insert_user);
    sqlite3_finalize(dbcontext->retrieve_user);
    sqlite3_finalize(dbcontext->insert_credentials);
    sqlite3_finalize(dbcontext->retrieve_credentials);
}

