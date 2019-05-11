#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sqlite3.h>
#include "cxss_credentials_db.h"


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
                 "UserPublicKey BLOB,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);
    
    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserAuth("
                 "PK_UserAuth INTEGER PRIMARY KEY,"
                 "CXSS_UserID TEXT,"
                 "AuthClass TEXT,"
                 "UserSalt BLOB,"
                 "UserIV BLOB,"
                 "UserPrivateKey BLOB,"
                 "RemovalFlag INT,"
                 "DateCreated TEXT,"
                 "DateLastUpdated TEXT);",
                 (void*)NULL, NULL, &err_msg);

    sqlite3_exec(dbcontext->db,
                 "CREATE TABLE IF NOT EXISTS UserResc("
                 "ResourceID TEXT PRIMARY KEY,"
                 "ResourceSalt BLOB,"
                 "ResourceUsernameIV BLOB,"
                 "ResourcePasswordIV BLOB,"
                 "ResourceUsername BLOB,"
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
                    "SELECT UserPublicKey, DateCreated"
                    ", DateLastUpdated FROM UserData"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserAuth(CXSS_UserID, UserPrivateKey"
                    ", UserSalt, UserIV, AuthClass, RemovalFlag"
                    ", DateCreated, DateLastUpdated)"
                    "  VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_user_auth_stmt, NULL);
   
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserPrivateKey, UserSalt, UserIV, AuthClass"
                    ", DateCreated, DateLastUpdated FROM UserAuth"
                    "  WHERE CXSS_UserID=? AND RemovalFlag=0;",
                    -1, &dbcontext->retrieve_user_auth_stmt, NULL);
 
    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT UserPrivateKey, UserSalt, UserIV, AuthClass"
                    ", RemovalFlag, DateCreated, DateLastUpdated FROM UserAuth"
                    "  WHERE CXSS_UserID=?;",
                    -1, &dbcontext->retrieve_user_auths_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "INSERT INTO UserResc (ResourceID, ResourceSalt"
                    ", ResourceUsernameIV, ResourcePasswordIV"
                    ", ResourceUsername, ResourcePassword, CXSS_UserID"
                    ", DateCreated, DateLastUpdated) "
                    "  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
                    -1, &dbcontext->insert_resc_stmt, NULL);

    sqlite3_prepare_v2(dbcontext->db,
                    "SELECT ResourceUsernameIV, ResourcePasswordIV"
                    ", ResourceUsername, ResourcePassword"
                    ", DateCreated, DateLastUpdated FROM UserResc"
                    "  WHERE CXSS_UserID=? AND ResourceID=?;",
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
    sqlite3_finalize(dbcontext->get_user_resc_count_stmt);
    sqlite3_finalize(dbcontext->is_user_in_db_stmt);
    sqlite3_finalize(dbcontext->is_resc_in_db_stmt);
    sqlite3_finalize(dbcontext->insert_user_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_stmt);
    sqlite3_finalize(dbcontext->insert_user_auth_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_auth_stmt);
    sqlite3_finalize(dbcontext->retrieve_user_auths_stmt);
    sqlite3_finalize(dbcontext->insert_resc_stmt);
    sqlite3_finalize(dbcontext->retrieve_resc_stmt);
}

/** @brief Insert new user
 *
 *  Create new user entry in 'UserData' table
 *
 *  @param dbcontext            Database context handle
 *  @param UserData             Pointer to CXSS_UserData struct
 *  @return                     Status code 
 */
int
cxss_insert_user(DB_Context_t dbcontext, CXSS_UserData *UserData)
{
    sqlite3_reset(dbcontext->insert_user_stmt);

    /* Bind data with sqlite3 stmts */
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
 *  @param UserAuth             Pointer to CXSS_UserAuth struct
 *  @return                     Status code 
 */
int
cxss_insert_user_auth(DB_Context_t dbcontext, CXSS_UserAuth *UserAuth)
{
    sqlite3_reset(dbcontext->insert_user_auth_stmt);

    /* Bind data with sqlite3 stmts */    
    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 1,
        UserAuth->CXSS_UserID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 2,
        UserAuth->PrivateKey, UserAuth->KeyLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 3,
        UserAuth->Salt, UserAuth->SaltLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_user_auth_stmt, 4,
        UserAuth->UserIV, UserAuth->IVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 5,
        UserAuth->AuthClass, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_int(dbcontext->insert_user_auth_stmt, 6,
        UserAuth->RemovalFlag) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 7,
        UserAuth->DateCreated, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->insert_user_auth_stmt, 8,
        UserAuth->DateLastUpdated, -1, NULL) != SQLITE_OK) {
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
 *  @param UserResc             Pointer to CXSS_UserResc struct
 *  @return                     Status code 
 */
int
cxss_insert_user_resc(DB_Context_t dbcontext, CXSS_UserResc *UserResc)
{
    sqlite3_reset(dbcontext->insert_resc_stmt);

    /* Bind data with sqlite3 stmts */    
    if (sqlite3_bind_text(dbcontext->insert_resc_stmt, 1, 
        UserResc->ResourceID, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }
    
    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 2,
        UserResc->ResourceSalt, UserResc->SaltLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 3, 
        UserResc->UsernameIV, UserResc->UsernameIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 4,
        UserResc->PasswordIV, UserResc->PasswordIVLength, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 5,
        UserResc->ResourceUsername, UserResc->UsernameLength, 
        NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_blob(dbcontext->insert_resc_stmt, 6,
        UserResc->ResourcePassword, UserResc->PasswordLength, 
        NULL) != SQLITE_OK) {
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
        fprintf(stderr, "Failed to insert user auth\n");
        return -1;
    }

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
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
cxss_retrieve_user(DB_Context_t dbcontext, const char *cxss_userid, 
                   CXSS_UserData *UserData)
{
    const char *publickey;
    const char *date_created, *date_last_updated;
    size_t keylength;

    sqlite3_reset(dbcontext->retrieve_user_stmt);

    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->retrieve_user_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_user_stmt) != SQLITE_ROW) 
        return -1;
    
    /* Retrieve results */
    publickey = sqlite3_column_blob(dbcontext->retrieve_user_stmt, 0);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_stmt, 0);
    date_created = sqlite3_column_text(dbcontext->retrieve_user_stmt, 1);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_stmt, 2);

    /* Populate UserData struct */
    UserData->CXSS_UserID = cxss_strdup(cxss_userid);
    UserData->PublicKey = cxss_strdup(publickey);
    UserData->KeyLength = keylength;
    UserData->DateCreated = cxss_strdup(date_created);
    UserData->DateLastUpdated = cxss_strdup(date_last_updated);
   
    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

/** @brief Free CXSS_UserData struct members
 *
 *  Free struct members containing query results.
 *
 *  @param UserData     Pointer to CXSS_UserData struct
 *  @return             void
 */
void
cxss_free_userdata(CXSS_UserData *UserData)
{
    free(UserData->CXSS_UserID);
    free(UserData->PublicKey);
    free(UserData->DateCreated);
    free(UserData->DateLastUpdated);
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
cxss_retrieve_user_auth(DB_Context_t dbcontext, const char *cxss_userid, 
                        CXSS_UserAuth *UserAuth)
{
    const char *privatekey, *salt, *iv, *auth_class;
    const char *date_created, *date_last_updated;
    size_t keylength, salt_length, iv_length;

    sqlite3_reset(dbcontext->retrieve_user_auth_stmt);

    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->retrieve_user_auth_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_user_auth_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Unable to retrieve user auth info: %s\n",
                        sqlite3_errmsg(dbcontext->db));
        return -1;
    }

    /* Retrieve results */
    privatekey = sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 0);
    keylength = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 0);
    salt = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 1);
    salt_length = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 1);
    iv = sqlite3_column_blob(dbcontext->retrieve_user_auth_stmt, 2);
    iv_length = sqlite3_column_bytes(dbcontext->retrieve_user_auth_stmt, 2);
    auth_class = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 3);
    date_created = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 4);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_auth_stmt, 5);
     
    /* Populate UserAuth struct */
    UserAuth->CXSS_UserID = cxss_strdup(cxss_userid);
    UserAuth->PrivateKey = cxss_blobdup(privatekey, keylength);
    UserAuth->KeyLength = keylength;
    UserAuth->Salt = cxss_blobdup(salt, salt_length);
    UserAuth->UserIV = cxss_blobdup(iv, iv_length);
    UserAuth->IVLength = iv_length;
    UserAuth->SaltLength = salt_length;
    UserAuth->AuthClass = cxss_strdup(auth_class);
    UserAuth->RemovalFlag = 0;
    UserAuth->DateCreated = cxss_strdup(date_created);
    UserAuth->DateLastUpdated = cxss_strdup(date_last_updated);

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

/** @brief Free CXSS_UserAuth struct members
 *
 *  Free struct members containing query results.
 *
 *  @param UserAuth     Pointer to CXSS_UserAuth struct
 *  @return             void
 */
void
cxss_free_userauth(CXSS_UserAuth *UserAuth)
{
    free(UserAuth->CXSS_UserID);
    free(UserAuth->PrivateKey);
    free(UserAuth->Salt);
    free(UserAuth->UserIV);
    free(UserAuth->AuthClass);
    free(UserAuth->DateCreated);
    free(UserAuth->DateLastUpdated);
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
cxss_retrieve_user_auths(DB_Context_t dbcontext, const char *cxss_userid, 
                         CXSS_UserAuth_LLNode **node)
{
    CXSS_UserAuth_LLNode *head, *prev, *current;
    const char *privatekey, *salt, *iv, *auth_class;
    const char *date_created, *date_last_updated;
    size_t keylength, salt_length, iv_length;
    int removal_flag;

    sqlite3_reset(dbcontext->retrieve_user_auths_stmt);

    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->retrieve_user_auths_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Allocate head (dummy node) */
    head = cxss_allocate_userauth_node();
    prev = head;

    /* Execute query */
    while (sqlite3_step(dbcontext->retrieve_user_auths_stmt) == SQLITE_ROW) { 
        
        /* Allocate and chain new node */
        current = cxss_allocate_userauth_node();       
        prev->next = current;
        
        /* Retrieve results */
        privatekey = sqlite3_column_blob(dbcontext->retrieve_user_auths_stmt, 0);
        keylength = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 0);
        salt = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 1);
        salt_length = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 1);
        iv = sqlite3_column_blob(dbcontext->retrieve_user_auths_stmt, 2);
        iv_length = sqlite3_column_bytes(dbcontext->retrieve_user_auths_stmt, 2); 
        auth_class = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 3);
        removal_flag = sqlite3_column_int(dbcontext->retrieve_user_auths_stmt, 4); 
        date_created = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 5);
        date_last_updated = sqlite3_column_text(dbcontext->retrieve_user_auths_stmt, 6);
     
        /* Populate node */
        current->UserAuth.CXSS_UserID = cxss_strdup(cxss_userid);
        current->UserAuth.PrivateKey = cxss_blobdup(privatekey, keylength);
        current->UserAuth.KeyLength = keylength;
        current->UserAuth.Salt = cxss_blobdup(salt, salt_length);
        current->UserAuth.SaltLength = salt_length;
        current->UserAuth.UserIV = cxss_blobdup(iv, iv_length);
        current->UserAuth.IVLength = iv_length;
        current->UserAuth.AuthClass = cxss_strdup(auth_class);
        current->UserAuth.RemovalFlag = removal_flag;
        current->UserAuth.DateCreated = cxss_strdup(date_created);
        current->UserAuth.DateLastUpdated = cxss_strdup(date_last_updated);
        
        /* Advance */ 
        prev = current;
    }

    current->next = NULL;
    *(node) = head;
    
    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

int 
cxss_retrieve_user_resc(DB_Context_t dbcontext, const char *cxss_userid, 
                        const char *resource_id, CXSS_UserResc *UserResc)
{
    const char *resource_username, *resource_password;
    const char *resource_salt, *username_iv, *password_iv;
    const char  *date_created, *date_last_updated;
    size_t salt_len, username_len, password_len;
    size_t username_iv_len, password_iv_len;       

    sqlite3_reset(dbcontext->retrieve_resc_stmt);

    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->retrieve_resc_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    if (sqlite3_bind_text(dbcontext->retrieve_resc_stmt, 2,
                          resource_id, -1, NULL) != SQLITE_OK) {
        goto bind_error;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->retrieve_resc_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Unable to retrieve user resc info\n");
        return -1;
    }

    /* Retrieve results */
    username_iv = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 0);
    username_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 0);
    password_iv = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 1);
    password_iv_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 1);
    resource_username = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 2);
    username_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 2);
    resource_password = sqlite3_column_blob(dbcontext->retrieve_resc_stmt, 3);
    password_len = sqlite3_column_bytes(dbcontext->retrieve_resc_stmt, 3);
    date_created = sqlite3_column_text(dbcontext->retrieve_resc_stmt, 4);
    date_last_updated = sqlite3_column_text(dbcontext->retrieve_resc_stmt, 5);     
    /* Build struct */
    UserResc->ResourceID = cxss_strdup(resource_id);
    UserResc->CXSS_UserID = cxss_strdup(cxss_userid);
    UserResc->UsernameIV = cxss_blobdup(username_iv, username_iv_len);
    UserResc->UsernameIVLength = username_iv_len;
    UserResc->PasswordIV = cxss_blobdup(password_iv, password_iv_len);
    UserResc->PasswordIVLength = password_iv_len;
    UserResc->ResourceUsername = cxss_blobdup(resource_username, username_len);
    UserResc->UsernameLength = username_len;
    UserResc->ResourcePassword = cxss_blobdup(resource_password, password_len);
    UserResc->PasswordLength = password_len;
    UserResc->DateCreated = cxss_strdup(date_created);
    UserResc->DateLastUpdated = cxss_strdup(date_last_updated);

    return 0;
bind_error:
    fprintf(stderr, "Failed to bind value with stmt: %s\n",
                    sqlite3_errmsg(dbcontext->db));
    return -1;
}

/** @brief Free CXSS_UserResc struct members
 *
 *  Free struct members containing query results.
 *
 *  @param UserAuth     Pointer to CXSS_UserResc struct
 *  @return             void
 */
void
cxss_free_userresc(CXSS_UserResc *UserResc)
{
    free(UserResc->CXSS_UserID);
    free(UserResc->ResourceID);
    free(UserResc->ResourceUsername);
    free(UserResc->ResourcePassword);
    free(UserResc->UsernameIV);
    free(UserResc->PasswordIV);
    free(UserResc->DateCreated);
    free(UserResc->DateLastUpdated);
}

/** @brief Allocate a CXSS_UserAuth_LLNode
 *
 *  Allocate a node for a linked list of 
 *  CXSS_UserAuth structs.
 *
 *  @return     Pointer to allocated node
 */  
static inline CXSS_UserAuth_LLNode* 
cxss_allocate_userauth_node(void)
{
    CXSS_UserAuth_LLNode *new_node;

    new_node = malloc(sizeof(CXSS_UserAuth_LLNode));
    if (!new_node) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    //memset(new_node, 0, sizeof(CXSS_UserAuth_LLNode));
    return new_node;
}

/** @brief Print linked list
 *
 */
void cxss_print_userauth_ll(CXSS_UserAuth_LLNode *start)
{
    static unsigned int count = 0;

    /* Skip head (dummy node) */
    if (start != NULL)
        start = start->next;

    while (start != NULL) {
        printf("COUNT IS: %d\n", count++);
        if (start->UserAuth.CXSS_UserID)
            printf("UserID:             %s\n", start->UserAuth.CXSS_UserID);
        if (start->UserAuth.PrivateKey)
            printf("PrivateKey:         %s\n", start->UserAuth.PrivateKey);
        
        printf("KeyLength:          %ld\n", start->UserAuth.KeyLength);
 
        if (start->UserAuth.AuthClass)
            printf("AuthClass:          %s\n", start->UserAuth.AuthClass);
        
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
void cxss_free_userauth_ll(CXSS_UserAuth_LLNode *start)
{
    CXSS_UserAuth_LLNode *next;

    /* Free head (dummy node) */
    next = start->next;
    free(start);
    start = next;

    while (start != NULL) {
        next = start->next;
        cxss_free_userauth(&start->UserAuth);
        free(start);
        start = next;
    }
}

/** @brief Duplicate a string
 *
 *  This is just a strdup wrapper function
 *  to nicely handle cases in which the SQL 
 *  query returns NULL.
 *
 *  @param str   String to dup.
 *  @return      Dupped string
 */
static inline char *
cxss_strdup(const char *str)
{
    if (!str)
        return NULL;
    
    return strdup(str);
}

/** @brief Duplicate an array of bytes
 *
 *  This function returns a pointer to
 *  a dynamic array containing a copy of 
 *  the dupped data.
 *
 *  @param blob         Blob to dup
 *  @param len          Length of blob
 *  @return             Dupped blob
 */
static inline char *
cxss_blobdup(const char *blob, size_t len)
{
    char *copy;

    if (!blob)
        return NULL;

    copy = malloc(sizeof(char) * len);
    if (!copy) {
        fprintf(stderr, "Memory allocation error!\n");
        exit(EXIT_FAILURE);
    }

    memcpy(copy, blob, len);
    return copy;
}

/** @brief Get user count
 *
 *  Returns the number of users
 *  registerd in the CXSS database.
 *
 *  @param dbcontext    Database context handle
 *  @return             User count
 */
int
cxss_get_user_count(DB_Context_t dbcontext)
{
    sqlite3_reset(dbcontext->get_user_count_stmt);

    /* Execute query */
    if (sqlite3_step(dbcontext->get_user_count_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Could not get count!\n");
        return -1;
    }
    
    return sqlite3_column_int(dbcontext->get_user_count_stmt, 0);
}


/** @brief Get resource count per user
 *
 *  Returns the number of resources registered
 *  for a given Centrallix user.
 *
 *  @param dbcontext    Database context handle
 *  @param cxss_userid  Centrallix user identity
 *  @return             Resource count
 */
int
cxss_get_user_resc_count(DB_Context_t dbcontext, const char *cxss_userid)
{
    sqlite3_reset(dbcontext->get_user_resc_count_stmt);

    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->get_user_resc_count_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n",
                        sqlite3_errmsg(dbcontext->db));
        return -1;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->get_user_resc_count_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Could not get count!\n");
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
 *  @param cxss_userid  Centrallix user identity
 *  @return             True if found, false if NOT found
 */
bool
cxss_db_contains_user(DB_Context_t dbcontext, const char *cxss_userid)
{
    size_t count;

    sqlite3_reset(dbcontext->is_user_in_db_stmt);
    
    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->is_user_in_db_stmt, 1,
                          cxss_userid, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n", 
                        sqlite3_errmsg(dbcontext->db));
        return -1;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_user_in_db_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Failure!\n");
        exit(EXIT_FAILURE);
    }

    /* Retrieve result */
    count = sqlite3_column_int(dbcontext->is_user_in_db_stmt, 0);

    if (count > 0)
        return true;
    else 
        return false;
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
    size_t count;

    sqlite3_reset(dbcontext->is_user_in_db_stmt);
    
    /* Bind data with sqlite3 stmt */
    if (sqlite3_bind_text(dbcontext->is_resc_in_db_stmt, 1,
                          resource_id, -1, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind stmt with value: %s\n", 
                        sqlite3_errmsg(dbcontext->db));
        return -1;
    }

    /* Execute query */
    if (sqlite3_step(dbcontext->is_resc_in_db_stmt) != SQLITE_ROW) {
        fprintf(stderr, "Failure!\n");
        exit(EXIT_FAILURE);
    }

    /* Retrieve result */
    count = sqlite3_column_int(dbcontext->is_resc_in_db_stmt, 0);

    if (count > 0)
        return true;
    else 
        return false;
}


