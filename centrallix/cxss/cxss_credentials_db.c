#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sqlite3.h>
#include <cxlib/newmalloc.h>
#include "cxss/credentials_db.h"
#include "cxss/util.h"

/* Private functions (credentials_db) */
static int cxss_i_SetupCredentialsDatabase(CXSS_DB_Context_t dbcontext);
static void cxss_i_FinalizeSqliteStatements(CXSS_DB_Context_t dbcontext);

/** @brief Initialize cxss credentials database
 *
 *  This function takes care of initializing this module (cxss_credentials_db.c), 
 *  and should be called before using any of the other functions below. It establishes
 *  a connection to the SQLite database file and sets up the database context.
 *
 *  @param db_path      Path to database file
 *  @return             Database context handle
 */
CXSS_DB_Context_t
cxssCredentialsDatabaseInit(const char *db_path)
{
    /* Allocate context struct */
    CXSS_DB_Context_t dbcontext = (CXSS_DB_Context_t)nmMalloc(sizeof(struct _CXSS_DB_Context_t));
    if (!dbcontext) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }

    /* Open database file and set up tables/stmts */
    if (sqlite3_open(db_path, &dbcontext->db) != SQLITE_OK) {
        mssError(0, "CXSS", "Failed to open database file: %s\n", sqlite3_errmsg(dbcontext->db));
        goto error;
    }
    if (cxss_i_SetupCredentialsDatabase(dbcontext) < 0) {
        mssError(0, "CXSS", "Failed to prepare database tables/stmts\n");
        goto error;
    }
    return dbcontext;

error:
    nmFree(dbcontext, sizeof(struct _CXSS_DB_Context_t));
    return NULL;
}

/** @brief Cleanup and close cxss credentials database
 *
 *  This function closes the connection to the SQLite
 *  credentials database and deallocates the database
 *  context.
 *
 *  @param dbcontext    Database context handle
 *  @return             Status code
 */
void
cxssCredentialsDatabaseClose(CXSS_DB_Context_t dbcontext)
{
    cxss_i_FinalizeSqliteStatements(dbcontext);
    sqlite3_close(dbcontext->db);
    nmFree(dbcontext, sizeof(struct _CXSS_DB_Context_t));
}

/** @brief Setup database tables/statements
 *
 *  This function creates the required tables to store the user's 
 *  credentials (if they don't already exists in the database file)
 *  and pre-compiles some SQL queries into perpared statements.
 *
 *  @param dbcontext    Database context handle
 *  @return             Status code
 */
static int
cxss_i_SetupCredentialsDatabase(CXSS_DB_Context_t dbcontext)
{   
    char *err_msg = NULL;

    /* Create Tables */
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserData("
                 "CXSS_UserID TEXT PRIMARY KEY,"
                 "UserPublicKey BLOB,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserAuth("
                 "PK_UserAuth INTEGER PRIMARY KEY,"
                 "CXSS_UserID TEXT,"
                 "AuthClass TEXT,"
                 "UserPrivateKey BLOB,"
                 "PrivateKeyIV BLOB,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT,"
                 "RemovalFlag INT);",
                 (void*)NULL, NULL, &err_msg);
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserResc("
                 "ResourceID TEXT PRIMARY KEY,"
                 "AESKey BLOB,"
                 "ResourceUsername BLOB,"
                 "ResourceAuthData BLOB,"
                 "ResourceUsernameIV BLOB,"
                 "ResourceAuthDataIV BLOB,"
                 "CXSS_UserID TEXT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    if (err_msg) {
        mssError(0, "CXSS", "SQL Error: %s\n", err_msg);
        goto error;
    }

    /* Compile SQL statements */
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT(*) FROM UserData;",
                    -1, &dbcontext->get_user_count_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT (*) FROM UserAuth"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->get_user_resc_count_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT(*) FROM UserData"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->is_user_in_db_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT COUNT(*) FROM UserResc"
                    "  WHERE ResourceID=?;",
                    -1, &dbcontext->is_resc_in_db_stmt, NULL); 
    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserData(CXSS_UserID, UserPublicKey"
                    ", DateCreated, DateLastUpdated) VALUES(?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "UPDATE UserData SET UserPublicKey=?, DateLastUpdated=?"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->update_user_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserPublicKey, DateCreated"
                    ", DateLastUpdated FROM UserData"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserData WHERE CXSS_UserID=?;",
                    -1, &dbcontext->delete_user_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserAuth (CXSS_UserID"
                    ", AuthClass, UserPrivateKey, PrivateKeyIV"
                    ", DateCreated, DateLastUpdated, RemovalFlag)"
                    "  VALUES ( ?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "UPDATE UserAuth SET CXSS_UserID=?"
                    ", AuthClass=?, UserPrivateKey=?, PrivateKeyIV=?"
                    ", DateLastUpdated=?, RemovalFlag=? WHERE PK_UserAuth=?;", 
                    -1, &dbcontext->update_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT CXSS_UserID, AuthClass, UserPrivateKey"
                    ", PrivateKeyIV, DateCreated, DateLastUpdated"
                    " FROM UserAuth  WHERE PK_UserAuth=? AND RemovalFlag=0;",
                    -1, &dbcontext->retrieve_user_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT PK_UserAuth, AuthClass, UserPrivateKey"
                    ", PrivateKeyIV, DateCreated, DateLastUpdated RemovalFlag"
                    " FROM UserAuth  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_auths_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT PK_UserAuth, AuthClass, UserPrivateKey"
                    ", PrivateKeyIV, DateCreated, DateLastUpdated, RemovalFlag"
                    " FROM UserAuth  WHERE CXSS_UserID=? AND AuthClass=?;",
                    -1, &dbcontext->retrieve_user_auths_class_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserAuth WHERE PK_UserAuth=?;",
                    -1, &dbcontext->delete_user_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserAuth WHERE CXSS_UserID=?;",
                    -1, &dbcontext->delete_user_auths_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserAuth WHERE CXSS_UserID=? AND AuthClass=?;",
                    -1, &dbcontext->delete_user_auths_class_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserResc (ResourceID, AESKey"
                    ", ResourceUsernameIV, ResourceAuthDataIV"
                    ", ResourceUsername, ResourceAuthData, CXSS_UserID"
                    ", DateCreated, DateLastUpdated)"
                    "  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "UPDATE UserResc SET AESKey=?, ResourceUsername=?"
                    ", ResourceAuthData=?, ResourceUsernameIV=?"
                    ", ResourceAuthDataIV=?, DateLastUpdated=?"
                    "  WHERE CXSS_UserID=? AND ResourceID=?;", 
                    -1, &dbcontext->update_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT ResourceUsernameIV, ResourceAuthDataIV"
                    ", AESKey, ResourceUsername, ResourceAuthData"
                    ", DateCreated, DateLastUpdated FROM UserResc"
                    "  WHERE CXSS_UserID=? AND ResourceID=?;",
                    -1, &dbcontext->retrieve_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserResc WHERE CXSS_UserID=? AND ResourceID=?;",
                    -1, &dbcontext->delete_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserResc WHERE CXSS_UserID=?;",
                    -1, &dbcontext->delete_rescs_stmt, NULL);
    return CXSS_DB_SUCCESS;

error:
    sqlite3_free(err_msg);
    sqlite3_close(dbcontext->db);
    return CXSS_DB_SETUP_ERROR;
}

/** @brief Insert new user
 *
 *  Create new user entry in 'UserData' table.
 *
 *  @param dbcontext            Database context handle
 *  @param UserData             Pointer to CXSS_UserData struct
 *  @return                     Status code 
 */
int
cxssInsertUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->insert_user_stmt); 
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 1, 
        UserData->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_user_stmt, 2,
        UserData->PublicKey, UserData->KeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 3,
        UserData->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 4,
        UserData->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to insert user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Insert user auth data
 *
 *  Create user auth entry in 'UserAuth' table
 *
 *  @param dbcontext            Database context handle
 *  @param UserAuth             Pointer to CXSS_UserAuth struct
 *  @return                     Status code 
 */
int
cxssInsertUserAuth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth)
{
    /* Bind data with sqlite3 stmts */    
    sqlite3_reset(dbcontext->insert_user_auth_stmt);
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 1,
        UserAuth->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 2,
        UserAuth->AuthClass, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 3,
        UserAuth->PrivateKey, UserAuth->KeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 4,
        UserAuth->PrivateKeyIV, UserAuth->IVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 5,
        UserAuth->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 6,
        UserAuth->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_int(dbcontext->insert_user_auth_stmt, 7,
        UserAuth->RemovalFlag) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_auth_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to insert user auth\n");
        return CXSS_DB_QUERY_ERROR;
    }

    /* set id to assigned value */
    UserAuth->PK_UserAuth = sqlite3_last_insert_rowid(dbcontext->db);

    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Insert user resource
 *
 *  Create resource entry in 'UserResc' table
 *
 *  @param dbcontext            Database context handle
 *  @param UserResc             Pointer to CXSS_UserResc struct
 *  @return                     Status code 
 */
int
cxssInsertUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc)
{
    /* Bind data with sqlite3 stmts */    
    sqlite3_reset(dbcontext->insert_resc_stmt);
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 1, 
        UserResc->ResourceID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 2,
        UserResc->AESKey, UserResc->AESKeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 3, 
        UserResc->UsernameIV, UserResc->UsernameIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 4,
        UserResc->AuthDataIV, UserResc->AuthDataIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 5,
        UserResc->ResourceUsername, UserResc->UsernameLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 6,
        UserResc->ResourceAuthData, UserResc->AuthDataLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 7, 
        UserResc->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 8,
        UserResc->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 9,
        UserResc->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_resc_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to insert user resc\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user data
 *
 *  Retrieve user data from 'UserData' table
 * 
 *  @param dbcontext        Database context handle 
 *  @param cxss_userid      CXSS User ID
 *  @param UserData         Pointer to CXSS_UserData struct
 *  @return                 Status code
 */
int 
cxssRetrieveUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid, 
                     CXSS_UserData *UserData)
{
    const char *publickey;
    const char *date_created, *date_last_updated;
    size_t keylength;

    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->retrieve_user_stmt);
    if (sqlite3_bind_text(dbcontext->retrieve_user_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_user_stmt) != SQLITE_ROW) 
        return CXSS_DB_QUERY_ERROR;
    
    /* Retrieve results */
    publickey = (char*)sqlite3_column_blob(dbcontext->retrieve_user_stmt, 0);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_stmt, 0);
    date_created = (char*)sqlite3_column_text(dbcontext->retrieve_user_stmt, 1);
    date_last_updated = (char*)sqlite3_column_text(dbcontext->retrieve_user_stmt, 2);

    /* Populate UserData struct */
    UserData->CXSS_UserID = cxssStrdup(cxss_userid);
    UserData->PublicKey = cxssStrdup(publickey);
    UserData->DateCreated = cxssStrdup(date_created);
    UserData->DateLastUpdated = cxssStrdup(date_last_updated);
    UserData->KeyLength = keylength;
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user authentication data
 *
 *  Retrieve user auth data from 'UserAuth' table
 * 
 *  @param dbcontext        Database context handle 
 *  @param cxss_userid      CXSS User ID
 *  @param cxss_pkUserAuth  The key for the desired item
 *  @return                 Status code
 */
int
cxssRetrieveUserAuth(CXSS_DB_Context_t dbcontext, int cxss_pkUserAuth, 
                     CXSS_UserAuth *UserAuth)
{
    const char *cxss_userid, *auth_class, *privatekey; 
    const char *iv, *date_created, *date_last_updated;
    size_t keylength, iv_length;
    int removal_flag;

    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->retrieve_user_auth_stmt);
    if (sqlite3_bind_int(dbcontext->retrieve_user_auth_stmt, 1, cxss_pkUserAuth) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_user_auth_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Unable to retrieve user auth info: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_QUERY_ERROR;
    }

    /* Retrieve results */
    cxss_userid = (char*) sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 0);
    auth_class = (char*) sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 1);
    privatekey = (char*)sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 2);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 2);
    iv = (char*)sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 3);
    iv_length = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 3);
    date_created = (char*)sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 4);
    date_last_updated = (char*)sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 5);
    removal_flag = (char*)sqlite3_column_int(dbcontext->retrieve_user_auth_stmt, 6);
     
    /* Populate UserAuth struct */
    UserAuth->PK_UserAuth = cxss_pkUserAuth;
    UserAuth->CXSS_UserID = cxssStrdup(cxss_userid);
    UserAuth->AuthClass = cxssStrdup(auth_class);
    UserAuth->PrivateKey = cxssBlobdup(privatekey, keylength);
    UserAuth->PrivateKeyIV = cxssBlobdup(iv, iv_length);
    UserAuth->DateCreated = cxssStrdup(date_created);
    UserAuth->DateLastUpdated = cxssStrdup(date_last_updated);
    UserAuth->RemovalFlag = removal_flag;
    UserAuth->KeyLength = keylength;
    UserAuth->IVLength = iv_length;
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve all user authentication entries
 *
 *  This function retrieves all user auth entries and
 *  returns a linked list of them.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  CXSS User ID
 *  @return             void
 */
 //FIXME: skipping for now
int
cxssRetrieveUserAuthLL(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char* auth_class,
                       CXSS_UserAuth_LLNode **node)
{
    CXSS_UserAuth_LLNode *head, *prev, *current;
    const char *pk_userAuth, *privatekey; 
    const char *iv, *date_created, *date_last_updated;
    size_t keylength, iv_length;
    bool removal_flag;

    /* Bind data with sqlite3 stmt */
    /*determine if need auth_class or not */
    sqlite3_stmt *stmt;

    if(auth_class != NULL){
        stmt = dbcontext->retrieve_user_auths_class_stmt;
        sqlite3_reset(stmt);
        if (sqlite3_bind_text(stmt, 2, auth_class, -1, NULL) != SQLITE_OK) {
            goto bind_error;
        }
    } else {
        stmt = dbcontext->retrieve_user_auths_stmt;
        sqlite3_reset(stmt);
    }
    if (sqlite3_bind_text(stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Allocate head (dummy node) */
    head = (CXSS_UserAuth_LLNode*)nmMalloc(sizeof(CXSS_UserAuth_LLNode));
    prev = head;
    head->next = NULL; /* need to detect if failed */

    /* Execute query */
    int code = sqlite3_step(stmt);
    while (code == SQLITE_ROW) { 
        
        /* Allocate and chain new node */
        current = (CXSS_UserAuth_LLNode*)nmMalloc(sizeof(CXSS_UserAuth_LLNode));       
        prev->next = current;
        
        /* Retrieve results */
        char *tempUserid = nmSysMalloc(strlen(cxss_userid));
        strcpy(tempUserid, cxss_userid);
        pk_userAuth = (int) sqlite3_column_int(stmt, 0);
        auth_class = (char*) sqlite3_column_text(stmt, 1);
        privatekey = (char*)sqlite3_column_blob(stmt, 2);
        keylength = sqlite3_column_bytes(stmt, 2);
        iv = (char*)sqlite3_column_blob(stmt, 3);
        iv_length = sqlite3_column_bytes(stmt, 3);
        date_created = (char*)sqlite3_column_text(stmt, 4);
        date_last_updated = (char*)sqlite3_column_text(stmt, 5);
        removal_flag = (bool) sqlite3_column_int(stmt, 6);
        
        /* Populate node */
        current->UserAuth.PK_UserAuth = pk_userAuth;
        current->UserAuth.CXSS_UserID = tempUserid;
        current->UserAuth.AuthClass = cxssStrdup(auth_class);
        current->UserAuth.PrivateKey = cxssBlobdup(privatekey, keylength);
        current->UserAuth.PrivateKeyIV = cxssBlobdup(iv, iv_length);
        current->UserAuth.DateCreated = cxssStrdup(date_created);
        current->UserAuth.DateLastUpdated = cxssStrdup(date_last_updated);
        current->UserAuth.RemovalFlag = removal_flag;
        current->UserAuth.KeyLength = keylength;
        current->UserAuth.IVLength = iv_length;
        
        /* Advance */ 
        prev = current;
        code = sqlite3_step(stmt);
    }

    if(code != SQLITE_DONE || head->next == NULL){
        return CXSS_DB_QUERY_ERROR;
        nmFree(head, sizeof(CXSS_UserAuth_LLNode));
    }

    current->next = NULL;
    *(node) = head;
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    if(head != NULL) nmFree(head, sizeof(CXSS_UserAuth_LLNode));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user resource
 *
 *  Retrieve a given user's resource
 *  from the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix User ID
 *  @param resource_id  Resource ID
 *  @param UserResc     Pointer to a CXSS_UserResc struct
 *  @return             Status code
 */
int 
cxssRetrieveUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid, 
                     const char *resource_id, CXSS_UserResc *UserResc)
{
    const char *resource_username, *resource_authdata;
    const char *aeskey;
    const char *username_iv, *authdata_iv;
    const char  *date_created, *date_last_updated;
    size_t aeskey_len, username_len, authdata_len;
    size_t username_iv_len, authdata_iv_len;       

    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->retrieve_resc_stmt);
    if (sqlite3_bind_text(dbcontext->retrieve_resc_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->retrieve_resc_stmt, 2, resource_id, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_resc_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Unable to retrieve user resc info\n");
        return CXSS_DB_QUERY_ERROR;
    }

    /* Retrieve results */
    username_iv = (char*)sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 0);
    username_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 0);
    authdata_iv = (char*)sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 1);
    authdata_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 1);
    aeskey = (char*)sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 2);
    aeskey_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 2);
    resource_username = (char*)sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 3);
    username_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 3);
    resource_authdata = (char*)sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 4);
    authdata_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 4);
    date_created = (char*)sqlite3_column_text(dbcontext->retrieve_resc_stmt, 5);
    date_last_updated = (char*)sqlite3_column_text(dbcontext->retrieve_resc_stmt, 6);

    /* Build struct */
    UserResc->ResourceID = cxssStrdup(resource_id);
    UserResc->CXSS_UserID = cxssStrdup(cxss_userid);
    UserResc->AESKey = cxssBlobdup(aeskey, aeskey_len);
    UserResc->ResourceUsername = cxssBlobdup(resource_username, username_len);
    UserResc->ResourceAuthData = cxssBlobdup(resource_authdata, authdata_len);
    UserResc->UsernameIV = cxssBlobdup(username_iv, username_iv_len);
    UserResc->AuthDataIV = cxssBlobdup(authdata_iv, authdata_iv_len);
    UserResc->DateCreated = cxssStrdup(date_created);
    UserResc->DateLastUpdated = cxssStrdup(date_last_updated);
    UserResc->AESKeyLength = aeskey_len; 
    UserResc->UsernameLength = username_len;
    UserResc->AuthDataLength = authdata_len;
    UserResc->UsernameIVLength = username_iv_len;
    UserResc->AuthDataIVLength = authdata_iv_len;
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Update user data
 *
 *  Update user data in CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param UserData     Pointer to a CXSS_UserData struct
 *  @return             Status code
 */
int
cxssUpdateUserData(CXSS_DB_Context_t dbcontext, CXSS_UserData *UserData)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->update_user_stmt); 
    if (sqlite3_bind_text(dbcontext->update_user_stmt, 1, 
        UserData->PublicKey, UserData->KeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_user_stmt, 2,
        UserData->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_user_stmt, 3,
        UserData->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->update_user_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to update user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No user found to update\n");
        return CXSS_DB_NOENT_ERROR;
    }

    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}      

/** @brief Update user authentication
 *
 *  Update a given user's authentication in CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param UserAuth     Pointer to a CXSS_UserAuth struct
 *  @return             Status code
 */
int
cxssUpdateUserAuth(CXSS_DB_Context_t dbcontext, CXSS_UserAuth *UserAuth)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->update_auth_stmt); 
    if (sqlite3_bind_text(dbcontext->update_auth_stmt, 1,
        UserAuth->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_auth_stmt, 2,
        UserAuth->AuthClass, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_auth_stmt, 3,
        UserAuth->PrivateKey, UserAuth->KeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_auth_stmt, 4,
        UserAuth->PrivateKeyIV, UserAuth->IVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_auth_stmt, 5,
        UserAuth->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_int(dbcontext->update_auth_stmt, 6,
        UserAuth->RemovalFlag) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_int(dbcontext->update_auth_stmt, 7,
        UserAuth->PK_UserAuth) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->update_auth_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to update resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No resource found to update\n");
        return CXSS_DB_NOENT_ERROR;
    }

    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Update user resource
 *
 *  Update a given user's resource in CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param UserResc     Pointer to a CXSS_UserResc struct
 *  @return             Status code
 */
int
cxssUpdateUserResc(CXSS_DB_Context_t dbcontext, CXSS_UserResc *UserResc)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->update_resc_stmt); 
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 1,
        UserResc->AESKey, UserResc->AESKeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 2,
        UserResc->ResourceUsername, UserResc->UsernameLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 3,
        UserResc->ResourceAuthData, UserResc->AuthDataLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 4,
        UserResc->UsernameIV, UserResc->UsernameIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 5,
        UserResc->AuthDataIV, UserResc->AuthDataIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 6,
        UserResc->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 7,
        UserResc->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 8,
        UserResc->ResourceID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->update_resc_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to update resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No resource found to update\n");
        return CXSS_DB_NOENT_ERROR;
    }

    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete user data
 *
 *  Delete user data from CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix User ID
 *  @return             Status code
 */
int
cxssDeleteUserData(CXSS_DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->delete_user_stmt);
    if (sqlite3_bind_text(dbcontext->delete_user_stmt, 1,
        cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_user_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to delete user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No user found to delete\n");
        return CXSS_DB_NOENT_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete user auth
 *
 *  Delete user auth from CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param pk_userAuth  primary key for auth entry
 *  @return             Status code
 */
int
cxssDeleteUserAuth(CXSS_DB_Context_t dbcontext, int pk_userAuth)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->delete_user_auth_stmt);
    if (sqlite3_bind_int(dbcontext->delete_user_auth_stmt, 1,
        pk_userAuth) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_user_auth_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to delete user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No user found to delete\n");
        return CXSS_DB_NOENT_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete user resource
 *
 *  Delete a given user's resource from CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix User ID
 *  @param resource_id  Resource ID
 *  @return             Status code
 */
int
cxssDeleteUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid,
                   const char *resource_id)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->delete_resc_stmt);
    if (sqlite3_bind_text(dbcontext->delete_resc_stmt, 1,
        cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->delete_resc_stmt, 2,
        resource_id, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_resc_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to delete resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No resource found to delete\n");
        return CXSS_DB_NOENT_ERROR;
    }
    return CXSS_DB_SUCCESS;
    
bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete all user auth entries
 *
 *  Delete all user auth entries for a given CXSS User ID
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix User ID
 *  @return             Status code
 */
int
cxssDeleteAllUserAuth(CXSS_DB_Context_t dbcontext, const char *cxss_userid, const char *auth_class)
{
    /* base off of auth class as well as user, if inlcuded */
    /* Bind data with sqlite3 stmts */
    sqlite3_stmt *stmt;
    if(auth_class != NULL){
        stmt = dbcontext->delete_user_auths_class_stmt;
        sqlite3_reset(stmt);
        
        if (sqlite3_bind_text(stmt, 2, auth_class, -1, NULL) != SQLITE_OK) {
            goto bind_error;
        }
    }else {
        stmt = dbcontext->delete_user_auths_stmt;
        sqlite3_reset(stmt);
    }
    if (sqlite3_bind_text(stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to delete user auths\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No user auths found to delete\n");
        return CXSS_DB_NOENT_ERROR;
    }
    return CXSS_DB_SUCCESS;
    
bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete all user resources
 *
 *  Delete all user resources registered under a given CXSS User ID
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix User ID
 *  @return             Status code
 */
int
cxssDeleteAllUserResc(CXSS_DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->delete_rescs_stmt);
    if (sqlite3_bind_text(dbcontext->delete_rescs_stmt, 1,
        cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_rescs_stmt) != SQLITE_DONE) {
        mssError(0, "CXSS", "Failed to delete resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    if(sqlite3_changes(dbcontext->db) < 1) { /* Make sure an update occured */
        mssError(0, "CXSS", "No resources found to delete\n");
        return CXSS_DB_NOENT_ERROR;
    }
    return CXSS_DB_SUCCESS;
    
bind_error:
    mssError(0, "CXSS", "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Free linked list
 *  
 *  Free user_auth linked list
 *
 *  @param start        Pointer to head of linked list
 *  @return             void
 */
void
cxssFreeUserAuthLL(CXSS_UserAuth_LLNode *start)
{
    /* Free head (dummy node) */
    CXSS_UserAuth_LLNode *next = start->next;
    nmFree(start, sizeof(CXSS_UserAuth_LLNode));
    start = next;

    while (start != NULL) {
        next = start->next;
        cxssFreeUserAuth(&start->UserAuth); 
        nmFree(start, sizeof(CXSS_UserAuth_LLNode));
        start = next;
    }
}

/** @brief Get user count
 *
 *  Returns the number of users registerd in CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @return             User count
 */
int
cxssGetUserCount(CXSS_DB_Context_t dbcontext)
{
    /* Execute query */
    sqlite3_reset(dbcontext->get_user_count_stmt);
    if (sqlite3_step(dbcontext->get_user_count_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Failed to retrieve user count\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return sqlite3_column_int(dbcontext->get_user_count_stmt, 0);
}


/** @brief Get resource count per user
 *
 *  Returns the number of resources registered
 *  for a given Centrallix user.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  CXSS User ID
 *  @return             Resource count
 */
int
cxssGetUserRescCount(CXSS_DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->get_user_resc_count_stmt);
    if (sqlite3_bind_text(dbcontext->get_user_resc_count_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        mssError(0, "CXSS", "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_BIND_ERROR;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->get_user_resc_count_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Failed to retrieve resource count\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return sqlite3_column_int(dbcontext->get_user_resc_count_stmt, 0);
}

/** @brief Checks if a user is in CXSS database
 *
 *  This function checks if a given user is 
 *  already present in the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  CXSS User ID
 *  @return             True if found, false if NOT found
 */
bool
cxssDbContainsUser(CXSS_DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->is_user_in_db_stmt);
    if (sqlite3_bind_text(dbcontext->is_user_in_db_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        mssError(0, "CXSS", "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_BIND_ERROR;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_user_in_db_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Failed to verify user existence in database\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return sqlite3_column_int(dbcontext->is_user_in_db_stmt, 0);
}

/** @brief Checks if a resource is in CXSS database
 *
 *  This function checks if a given resource is 
 *  already present in the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @param resource_id  Resource ID
 *  @return             True if found, false if NOT found
 */
bool
cxssDbContainsResc(CXSS_DB_Context_t dbcontext, const char *resource_id)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->is_resc_in_db_stmt);
    if (sqlite3_bind_text(dbcontext->is_resc_in_db_stmt, 1, resource_id, -1, NULL) != SQLITE_OK) {
        mssError(0, "CXSS", "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_BIND_ERROR;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_resc_in_db_stmt) != SQLITE_ROW) {
        mssError(0, "CXSS", "Failed to verify user existence in database\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return sqlite3_column_int(dbcontext->is_resc_in_db_stmt, 0);
}

/** @brief Free dynamic CXSS_UserData struct members
 *
 *  Free dynamically-allocated struct members 
 *  containing query results.
 *
 *  @param UserData     Pointer to CXSS_UserData struct
 *  @return             void
 */
void
cxssFreeUserData(CXSS_UserData *UserData)
{
    nmSysFree((void*)UserData->CXSS_UserID);
    nmSysFree((void*)UserData->PublicKey);
    nmSysFree((void*)UserData->DateCreated);
    nmSysFree((void*)UserData->DateLastUpdated);
}

/** @brief Free dynamic CXSS_UserAuth struct members
 *
 *  Free dynamically-allocated struct members 
 *  containing query results.
 *
 *  @param UserAuth     Pointer to CXSS_UserAuth struct
 *  @return             void
 */
void
cxssFreeUserAuth(CXSS_UserAuth *UserAuth)
{
    nmSysFree((void*)UserAuth->CXSS_UserID);
    nmSysFree((void*)UserAuth->PrivateKey);
    nmSysFree((void*)UserAuth->PrivateKeyIV);
    nmSysFree((void*)UserAuth->DateCreated);
    nmSysFree((void*)UserAuth->DateLastUpdated);
}

/** @brief Free dynamic CXSS_UserResc struct members
 *
 *  Free dynamically-allocated struct members 
 *  containing query results.
 *
 *  @param UserAuth     Pointer to CXSS_UserResc struct
 *  @return             void
 */
void
cxssFreeUserResc(CXSS_UserResc *UserResc)
{
    nmSysFree((void*)UserResc->CXSS_UserID);
    nmSysFree((void*)UserResc->ResourceID);
    nmSysFree((void*)UserResc->AESKey);
    nmSysFree((void*)UserResc->ResourceUsername);
    nmSysFree((void*)UserResc->ResourceAuthData);
    nmSysFree((void*)UserResc->UsernameIV);
    nmSysFree((void*)UserResc->AuthDataIV);
    nmSysFree((void*)UserResc->DateCreated);
    nmSysFree((void*)UserResc->DateLastUpdated);
}

/** @brief Cleanup sqlite3 statements
 *
 *  This function should be called when closing CXSS. 
 *  Its job is to de-allocate the pre-compiled sqlite3 statements.
 *
 *  @param dbcontext    Database context handle
 *  @return             void         
 */
static void 
cxss_i_FinalizeSqliteStatements(CXSS_DB_Context_t dbcontext)
{
    sqlite3_finalize(dbcontext->get_user_count_stmt);
    sqlite3_finalize(dbcontext->get_user_resc_count_stmt);
    sqlite3_finalize(dbcontext->is_user_in_db_stmt);
    sqlite3_finalize(dbcontext->is_resc_in_db_stmt);
    sqlite3_finalize(dbcontext->insert_user_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_stmt);
    sqlite3_finalize(dbcontext->update_user_stmt);
    sqlite3_finalize(dbcontext->delete_user_stmt);
    sqlite3_finalize(dbcontext->insert_user_auth_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_auth_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_auths_stmt);
    sqlite3_finalize(dbcontext->insert_resc_stmt);
    sqlite3_finalize(dbcontext->retrieve_resc_stmt);
    sqlite3_finalize(dbcontext->update_resc_stmt);
    sqlite3_finalize(dbcontext->delete_resc_stmt);
}