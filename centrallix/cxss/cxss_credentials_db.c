#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sqlite3.h>
#include "cxss_util.h"
#include "cxss_credentials_db.h"


/** @brief Initialize cxss credentials database
 *
 *  This function is called when starting CXSS in order
 *  to prepare the database environment and its related 
 *  data structures.
 *
 *  @param db_path      Path to database file
 *  @return             Database context handle
 */
DB_Context_t
cxss_init_credentials_database(const char *db_path)
{
    /* Allocate context struct */
    DB_Context_t dbcontext = malloc(sizeof(struct _DB_Context_t));
    if (!dbcontext) {
        fprintf(stderr, "Memory allocation error!\n");
        goto error;
    }

    /* Open database file and set up tables/stmts */
    if (sqlite3_open(db_path, &dbcontext->db) != SQLITE_OK) {
        fprintf(stderr, "Failed to open database file: %s\n", sqlite3_errmsg(dbcontext->db));
        goto error;
    }
    if (cxss_setup_credentials_database(dbcontext) < 0) {
        fprintf(stderr, "Failed to prepare database tables/stmts\n");
        goto error;
    }
    return dbcontext;

error:
    free(dbcontext);
    return NULL;
}

/** @brief Cleanup and close cxss credentials database
 *
 *  This function frees some db data structures and
 *  closes the connection to the database.
 *
 *  @param dbcontext    Database context handle
 *  @return             Status code
 */
int
cxss_close_credentials_database(DB_Context_t dbcontext)
{
    cxss_finalize_sqlite3_statements(dbcontext);
    sqlite3_close(dbcontext->db);
    free(dbcontext);
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
cxss_setup_credentials_database(DB_Context_t dbcontext)
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
                 "UserSalt BLOB,"
                 "PrivateKeyIV BLOB,"
                 "UserPrivateKey BLOB,"
                 "RemovalFlag INT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserResc("
                 "ResourceID TEXT PRIMARY KEY,"
                 "AuthClass TEXT,"
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
        fprintf(stderr, "SQL Error: %s\n", err_msg);
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
                    ", UserSalt, UserPrivateKey, PrivateKeyIV, RemovalFlag"
                    ", DateCreated, DateLastUpdated)"
                    "  VALUES (?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserPrivateKey, UserSalt, PrivateKeyIV"
                    ", DateCreated, DateLastUpdated FROM UserAuth"
                    "  WHERE CXSS_UserID=? AND RemovalFlag=0;",
                    -1, &dbcontext->retrieve_user_auth_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserPrivateKey, UserSalt, PrivateKeyIV"
                    ", RemovalFlag, DateCreated, DateLastUpdated FROM UserAuth"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_auths_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserResc (ResourceID, AuthClass"
                    ", AESKey, ResourceUsernameIV, ResourceAuthDataIV"
                    ", ResourceUsername, ResourceAuthData, CXSS_UserID"
                    ", DateCreated, DateLastUpdated)"
                    "  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "UPDATE UserResc SET AESKey=?, ResourceUsername=?"
                    ", ResourceAuthData=?, ResourceUsernameIV=?"
                    ", ResourceAuthDataIV=?, DateLastUpdated=?"
                    "  WHERE CXSS_UserID=? AND ResourceID=?;", 
                    -1, &dbcontext->update_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT ResourceUsernameIV, ResourceAuthDataIV"
                    ", AuthClass, AESKey, ResourceUsername, ResourceAuthData"
                    ", DateCreated, DateLastUpdated FROM UserResc"
                    "  WHERE CXSS_UserID=? AND ResourceID=?;",
                    -1, &dbcontext->retrieve_resc_stmt, NULL);
    sqlite3_prepare_v2(dbcontext->db,
                    "DELETE FROM UserResc WHERE CXSS_UserID=? AND ResourceID=?;",
                    -1, &dbcontext->delete_resc_stmt, NULL);
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
cxss_insert_userdata(DB_Context_t dbcontext, CXSS_UserData *UserData)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->insert_user_stmt); 
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 1, 
        UserData->CXSS_UserID, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->insert_user_stmt, 2,
        UserData->PublicKey, UserData->KeyLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 3,
        UserData->DateCreated, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->insert_user_stmt, 4,
        UserData->DateLastUpdated, -1, NULL) != SQLITE_OK)
        goto bind_error;
    
    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
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
cxss_insert_userauth(DB_Context_t dbcontext, CXSS_UserAuth *UserAuth)
{
    /* Bind data with sqlite3 stmts */    
    sqlite3_reset(dbcontext->insert_user_auth_stmt);
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 1,
        UserAuth->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 2,
        UserAuth->Salt, UserAuth->SaltLength, NULL) != SQLITE_OK) {
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
    if (sqlite3_bind_int(dbcontext->insert_user_auth_stmt, 5,
        UserAuth->RemovalFlag) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 6,
        UserAuth->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 7,
        UserAuth->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_user_auth_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user auth\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
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
cxss_insert_userresc(DB_Context_t dbcontext, CXSS_UserResc *UserResc)
{
    /* Bind data with sqlite3 stmts */    
    sqlite3_reset(dbcontext->insert_resc_stmt);
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 1, 
        UserResc->ResourceID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 2,
        UserResc->AuthClass, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 3,
        UserResc->AESKey, UserResc->AESKeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 4, 
        UserResc->UsernameIV, UserResc->UsernameIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 5,
        UserResc->PasswordIV, UserResc->PasswordIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 6,
        UserResc->ResourceUsername, UserResc->UsernameLength, 
        NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 7,
        UserResc->ResourcePassword, UserResc->PasswordLength, 
        NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 8, 
        UserResc->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 9,
        UserResc->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 10,
        UserResc->DateLastUpdated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->insert_resc_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user auth\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user data
 *
 *  Retrieve user data from 'UserData' table
 * 
 *  @param dbcontext        Database context handle 
 *  @param cxss_userid      CXSS user identity
 *  @param UserData         Pointer to CXSS_UserData struct
 *  @return                 Status code
 */
int 
cxss_retrieve_userdata(DB_Context_t dbcontext, const char *cxss_userid, 
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
    publickey = sqlite3_column_blob(dbcontext->retrieve_user_stmt, 0);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_stmt, 0);
    date_created = sqlite3_column_text(dbcontext->retrieve_user_stmt, 1);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_stmt, 2);

    /* Populate UserData struct */
    UserData->CXSS_UserID = cxss_strdup(cxss_userid);
    UserData->PublicKey = cxss_strdup(publickey);
    UserData->DateCreated = cxss_strdup(date_created);
    UserData->DateLastUpdated = cxss_strdup(date_last_updated);
    UserData->KeyLength = keylength;
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user authentication data
 *
 *  Retrieve user auth data from 'UserAuth' table
 * 
 *  @param dbcontext        Database context handle 
 *  @param cxss_userid      CXSS user identity
 *  @param UserAuth         Pointer to head of a CXSS_UserAuth linked list 
 *  @return                 Status code
 */
int
cxss_retrieve_userauth(DB_Context_t dbcontext, const char *cxss_userid, 
                        CXSS_UserAuth *UserAuth)
{
    const char *privatekey, *salt, *iv;
    const char *date_created, *date_last_updated;
    size_t keylength, salt_length, iv_length;

    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->retrieve_user_auth_stmt);
    if (sqlite3_bind_text(dbcontext->retrieve_user_auth_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_user_auth_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Unable to retrieve user auth info: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_QUERY_ERROR;
    }

    /* Retrieve results */
    privatekey = sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 0);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 0);
    salt = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 1);
    salt_length = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 1);
    iv = sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 2);
    iv_length = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 2);
    date_created = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 3);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 4);
     
    /* Populate UserAuth struct */
    UserAuth->CXSS_UserID = cxss_strdup(cxss_userid);
    UserAuth->PrivateKey = cxss_blobdup(privatekey, keylength);
    UserAuth->PrivateKeyIV = cxss_blobdup(iv, iv_length);
    UserAuth->Salt = cxss_blobdup(salt, salt_length);
    UserAuth->DateCreated = cxss_strdup(date_created);
    UserAuth->DateLastUpdated = cxss_strdup(date_last_updated);
    UserAuth->RemovalFlag = false;
    UserAuth->KeyLength = keylength;
    UserAuth->IVLength = iv_length;
    UserAuth->SaltLength = salt_length;
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve all user authentication entries
 *
 *  This function retrieves all user auth entries and
 *  returns a linked list of them.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  CXSS user identity
 *  @return             void
 */
int
cxss_retrieve_userauth_ll(DB_Context_t dbcontext, const char *cxss_userid, 
                          CXSS_UserAuth_LLNode **node)
{
    CXSS_UserAuth_LLNode *head, *prev, *current;
    const char *privatekey, *salt, *iv;
    const char *date_created, *date_last_updated;
    size_t keylength, salt_length, iv_length;
    bool removal_flag;

    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->retrieve_user_auths_stmt);
    if (sqlite3_bind_text(dbcontext->retrieve_user_auths_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Allocate head (dummy node) */
    head = malloc(sizeof(CXSS_UserAuth_LLNode));
    prev = head;

    /* Execute query */
    while (sqlite3_step(dbcontext->retrieve_user_auths_stmt) == SQLITE_ROW) { 
        
        /* Allocate and chain new node */
        current = malloc(sizeof(CXSS_UserAuth_LLNode));       
        prev->next = current;
        
        /* Retrieve results */
        privatekey = sqlite3_column_blob(dbcontext->retrieve_user_auths_stmt, 0);
        keylength = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 0);
        salt = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 1);
        salt_length = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 1);
        iv = sqlite3_column_blob(dbcontext->retrieve_user_auths_stmt, 2);
        iv_length = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 2); 
        removal_flag = sqlite3_column_int(dbcontext->retrieve_user_auths_stmt, 4); 
        date_created = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 5);
        date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 6);
     
        /* Populate node */
        current->UserAuth.CXSS_UserID = cxss_strdup(cxss_userid);
        current->UserAuth.PrivateKey = cxss_blobdup(privatekey, keylength);
        current->UserAuth.Salt = cxss_blobdup(salt, salt_length);
        current->UserAuth.PrivateKeyIV = cxss_blobdup(iv, iv_length);
        current->UserAuth.DateCreated = cxss_strdup(date_created);
        current->UserAuth.DateLastUpdated = cxss_strdup(date_last_updated);
        current->UserAuth.RemovalFlag = removal_flag;
        current->UserAuth.KeyLength = keylength;
        current->UserAuth.SaltLength = salt_length;
        current->UserAuth.IVLength = iv_length;
        
        /* Advance */ 
        prev = current;
    }

    current->next = NULL;
    *(node) = head;
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Retrieve user resource
 *
 *  Retrieve a given user's resource
 *  from the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix user identity
 *  @param resource_id  Resource identity
 *  @param UserResc     Pointer to a CXSS_UserResc struct
 *  @return             Status code
 */
int 
cxss_retrieve_userresc(DB_Context_t dbcontext, const char *cxss_userid, 
                        const char *resource_id, CXSS_UserResc *UserResc)
{
    const char *auth_class;
    const char *resource_username, *resource_password;
    const char *aeskey;
    const char *resource_salt, *username_iv, *password_iv;
    const char  *date_created, *date_last_updated;
    size_t salt_len, aeskey_len, username_len, password_len;
    size_t username_iv_len, password_iv_len;       

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
        fprintf(stderr, "Unable to retrieve user resc info\n");
        return CXSS_DB_QUERY_ERROR;
    }

    /* Retrieve results */
    username_iv = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 0);
    username_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 0);
    password_iv = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 1);
    password_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 1);
    auth_class = sqlite3_column_text(dbcontext->retrieve_resc_stmt, 2);
    aeskey = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 3);
    aeskey_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 3);
    resource_username = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 4);
    username_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 4);
    resource_password = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 5);
    password_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 5);
    date_created = sqlite3_column_text(dbcontext->retrieve_resc_stmt, 6);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_resc_stmt, 7);

    /* Build struct */
    UserResc->ResourceID = cxss_strdup(resource_id);
    UserResc->AuthClass = cxss_strdup(auth_class);
    UserResc->CXSS_UserID = cxss_strdup(cxss_userid);
    UserResc->AESKey = cxss_blobdup(aeskey, aeskey_len);
    UserResc->ResourceUsername = cxss_blobdup(resource_username, username_len);
    UserResc->ResourcePassword = cxss_blobdup(resource_password, password_len);
    UserResc->UsernameIV = cxss_blobdup(username_iv, username_iv_len);
    UserResc->PasswordIV = cxss_blobdup(password_iv, password_iv_len);
    UserResc->DateCreated = cxss_strdup(date_created);
    UserResc->DateLastUpdated = cxss_strdup(date_last_updated);
    UserResc->AESKeyLength = aeskey_len; 
    UserResc->UsernameLength = username_len;
    UserResc->PasswordLength = password_len;
    UserResc->UsernameIVLength = username_iv_len;
    UserResc->PasswordIVLength = password_iv_len;
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
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
cxss_update_userdata(DB_Context_t dbcontext, CXSS_UserData *UserData)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->update_user_stmt); 
    if (sqlite3_bind_text(dbcontext->update_user_stmt, 1, 
        UserData->PublicKey, UserData->KeyLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->update_user_stmt, 2,
        UserData->DateLastUpdated, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->update_user_stmt, 3,
        UserData->CXSS_UserID, -1, NULL) != SQLITE_OK)
        goto bind_error;
    
    /* Execute query */
    if (sqlite3_step(dbcontext->update_user_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to update user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
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
cxss_update_userresc(DB_Context_t dbcontext, CXSS_UserResc *UserResc)
{
    /* Bind data with sqlite3 stmts */
    sqlite3_reset(dbcontext->update_resc_stmt); 
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 1,
        UserResc->AESKey, UserResc->AESKeyLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 2,
        UserResc->ResourceUsername, UserResc->UsernameLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 3,
        UserResc->ResourcePassword, UserResc->PasswordLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 4,
        UserResc->UsernameIV, UserResc->UsernameIVLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_blob(dbcontext->update_resc_stmt, 5,
        UserResc->PasswordIV, UserResc->PasswordIVLength, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 6,
        UserResc->DateLastUpdated, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 7,
        UserResc->CXSS_UserID, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->update_resc_stmt, 8,
        UserResc->ResourceID, -1, NULL) != SQLITE_OK)
        goto bind_error;

    /* Execute query */
    if (sqlite3_step(dbcontext->update_user_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to update resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete user data
 *
 *  Delete user data from CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix user identity
 *  @return             Status code
 */
int
cxss_delete_userdata(DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->delete_user_stmt, 1,
        cxss_userid, -1, NULL) != SQLITE_OK)
        goto bind_error;
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_user_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete user\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;

bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Delete user resource
 *
 *  Delete a given user's resource from CXSS
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix user identity
 *  @param resource_id  Resource identity
 *  @return             Status code
 */
int
cxss_delete_userresc(DB_Context_t dbcontext, const char *cxss_userid,
                     const char *resource_id)
{
    /* Bind data with sqlite3 stmts */
    if (sqlite3_bind_text(dbcontext->delete_resc_stmt, 1,
        cxss_userid, -1, NULL) != SQLITE_OK)
        goto bind_error;
    if (sqlite3_bind_text(dbcontext->delete_resc_stmt, 2,
        resource_id, -1, NULL) != SQLITE_OK)
        goto bind_error;
    
    /* Execute query */
    if (sqlite3_step(dbcontext->delete_resc_stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to delete resource\n");
        return CXSS_DB_QUERY_ERROR;
    }
    return CXSS_DB_SUCCESS;
    
bind_error:
    fprintf(stderr, "Failed to bind value with SQLite statement: %s\n", sqlite3_errmsg(dbcontext->db));
    return CXSS_DB_BIND_ERROR;
}

/** @brief Print linked list (debug function)
 *
 */
void
cxss_print_userauth_ll(CXSS_UserAuth_LLNode *start)
{
    /* Skip head (dummy node) */
    if (start != NULL)
        start = start->next;
    while (start != NULL) {
        if (start->UserAuth.CXSS_UserID)
            printf("UserID:             %s\n", start->UserAuth.CXSS_UserID);
        if (start->UserAuth.PrivateKey)
            printf("PrivateKey:         %s\n", start->UserAuth.PrivateKey);
        printf("KeyLength:          %ld\n", start->UserAuth.KeyLength);
        printf("RemovalFlag:        %d\n", start->UserAuth.RemovalFlag);
        if (start->UserAuth.DateCreated)
            printf("DateCreated:        %s\n", start->UserAuth.DateCreated);
        if (start->UserAuth.DateLastUpdated)
            printf("DateLastUpdated:    %s\n", start->UserAuth.DateLastUpdated);
        start = start->next;
    }
}

/** @brief Free linked list
 *  
 *  Free user_auth linked list
 *
 *  @param start        Pointer to head of linked list
 *  @return             void
 */
void
cxss_free_userauth_ll(CXSS_UserAuth_LLNode *start)
{
    /* Free head (dummy node) */
    CXSS_UserAuth_LLNode *next = start->next;
    free(start);
    start = next;

    while (start != NULL) {
        next = start->next;
        cxss_free_userauth(&start->UserAuth);
        free(start);
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
cxss_get_user_count(DB_Context_t dbcontext)
{
    /* Execute query */
    sqlite3_reset(dbcontext->get_user_count_stmt);
    if (sqlite3_step(dbcontext->get_user_count_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Could not get count!\n");
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
 *  @param cxss_userid  CXSS user identity
 *  @return             Resource count
 */
int
cxss_get_userresc_count(DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->get_user_resc_count_stmt);
    if (sqlite3_bind_text(dbcontext->get_user_resc_count_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return -1;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->get_user_resc_count_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Failed to retrieve user count\n");
        return -1;
    }
    return sqlite3_column_int(dbcontext->get_user_resc_count_stmt, 0);
}

/** @brief Checks if a user is in CXSS database
 *
 *  This function checks if a given user is 
 *  already present in the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  CXSS user identity
 *  @return             True if found, false if NOT found
 */
bool
cxss_db_contains_user(DB_Context_t dbcontext, const char *cxss_userid)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->is_user_in_db_stmt);
    if (sqlite3_bind_text(dbcontext->is_user_in_db_stmt, 1, cxss_userid, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_BIND_ERROR;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_user_in_db_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Failed to verify user existence in database\n");
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
cxss_db_contains_resc(DB_Context_t dbcontext, const char *resource_id)
{
    /* Bind data with sqlite3 stmt */
    sqlite3_reset(dbcontext->is_resc_in_db_stmt);
    if (sqlite3_bind_text(dbcontext->is_resc_in_db_stmt, 1, resource_id, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n", sqlite3_errmsg(dbcontext->db));
        return CXSS_DB_BIND_ERROR;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_resc_in_db_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Failed to verify user existence in database\n");
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
cxss_free_userdata(CXSS_UserData *UserData)
{
    free((void*)UserData->CXSS_UserID);
    free((void*)UserData->PublicKey);
    free((void*)UserData->DateCreated);
    free((void*)UserData->DateLastUpdated);
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
cxss_free_userauth(CXSS_UserAuth *UserAuth)
{
    free((void*)UserAuth->CXSS_UserID);
    free((void*)UserAuth->PrivateKey);
    free((void*)UserAuth->Salt);
    free((void*)UserAuth->PrivateKeyIV);
    free((void*)UserAuth->DateCreated);
    free((void*)UserAuth->DateLastUpdated);
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
cxss_free_userresc(CXSS_UserResc *UserResc)
{
    free((void*)UserResc->CXSS_UserID);
    free((void*)UserResc->ResourceID);
    free((void*)UserResc->AuthClass);
    free((void*)UserResc->AESKey);
    free((void*)UserResc->ResourceUsername);
    free((void*)UserResc->ResourcePassword);
    free((void*)UserResc->UsernameIV);
    free((void*)UserResc->PasswordIV);
    free((void*)UserResc->DateCreated);
    free((void*)UserResc->DateLastUpdated);
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
cxss_finalize_sqlite3_statements(DB_Context_t dbcontext)
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


