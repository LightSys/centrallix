#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <openssl/bio.h>
#include "cxss/util.h"
#include "cxss/credentials_mgr.h"
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"
#include <assert.h>

/* Database context */
static CXSS_DB_Context_t dbcontext = NULL;

/** @brief Initialize credentials manager
 *
 *  This function should be called before making any other function 
 *  calls via the credentials manager API. It takes care of connecting 
 *  to and setting up the credentials database (cxss_credentials_db.c)
 *  and initializing the cryptography module (cxss_crypto.c).
 *
 *  @return     Status code
 */ 
int 
cxss_initCredentialsManager(void)
{   
    cxss_initializeCrypto();
    dbcontext = cxss_initCredentialsDatabase("test.db");
    if (!dbcontext) {
        return CXSS_MGR_INIT_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

/** @brief Close credentials manager
 *
 *  This function should be called when closing the credentials manager.
 *  It takes care of closing the connection to the credentials database,
 *  deallocating the database context and any crypto data structures.
 *
 *  @return     void
 */
void
cxss_closeCredentialsManager(void)
{
    cxss_cleanupCrypto();
    cxss_closeCredentialsDatabase(dbcontext);
}

/** @brief Add user to CXSS
 *
 *  @param cxss_userid          Centrallix User ID
 *  @param encryption_key       Password-based user encryption key
 *  @param keylength            Length of user encryption key
 *  @param salt                 User salt
 *  @param salt_len             Length of user salt
 *  @return                     Status code   
 */
int
cxss_addUser(const char *cxss_userid, const char *encryption_key, size_t encryption_key_len, 
             const char *salt, size_t salt_len)
{
    CXSS_UserData UserData = {};
    CXSS_UserAuth UserAuth = {};
    char *privatekey = NULL;
    char *publickey = NULL;
    char *encrypted_privatekey = NULL;
    char *current_timestamp = get_timestamp();
    char iv[16]; // 128-bit iv
    int privatekey_len, publickey_len;
    int encr_privatekey_len;

    /* Generate RSA key pair */
    if (cxss_generateRSA4096bitKeypair(&privatekey, &privatekey_len, 
                                       &publickey, &publickey_len) < 0) {
        mssError(0, "CXSS", "Failed to generate RSA keypair\n");
        goto error;
    }

    /* Generate initialization vector */
    if (cxss_generate128bitIV(iv) < 0) {
        mssError(0, "CXSS", "Failed to generate IV\n");
        goto error;
    }

    /* Encrypt private key */
    if (cxss_encryptAES256(privatekey, privatekey_len, encryption_key, 
                           iv, &encrypted_privatekey, &encr_privatekey_len) < 0) {
        mssError(0, "CXSS", "Error while encrypting private key\n");        
        goto error;
    }

    /* Build UserData/UserAuth structs */ 
    UserData.CXSS_UserID = cxss_userid;
    UserData.PublicKey = publickey;
    UserData.DateCreated = current_timestamp;
    UserData.DateLastUpdated = current_timestamp;
    UserData.KeyLength = publickey_len;
    UserAuth.CXSS_UserID = cxss_userid;
    UserAuth.PrivateKey = encrypted_privatekey;
    UserAuth.PrivateKeyIV = iv;
    UserAuth.Salt = salt;
    UserAuth.DateCreated = current_timestamp;
    UserAuth.DateLastUpdated = current_timestamp;
    UserAuth.RemovalFlag = false;
    UserAuth.SaltLength = salt_len;
    UserAuth.KeyLength = encr_privatekey_len;
    UserAuth.IVLength = sizeof(iv);

    if (cxss_insertUserData(dbcontext, &UserData) < 0) {
        mssError(0, "CXSS", "Failed to insert user into db\n");
        goto error;
    }
    if (cxss_insertUserAuth(dbcontext, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to insert user auth into db\n");
        goto error;
    }

    free(encrypted_privatekey);
    cxss_destroyRSAKeypair(privatekey, privatekey_len, publickey, publickey_len);
    return CXSS_MGR_SUCCESS;

error:
    free(encrypted_privatekey);
    free(privatekey);
    free(publickey);         
    return CXSS_MGR_INSERT_ERROR;
}

/** @brief Retrieve and decrypt user private key
 *
 *  @param cxss_userid          Centrallix User ID
 *  @param encryption_key       AES encryption key used to encrypt private key
 *  @param encryption_key_len   Length of encryption key
 *  @return                     Pointer to decrypted private key (must be freed)
 */
int
cxss_retrieveUserPrivateKey(const char *cxss_userid, const char *encryption_key, size_t ecryption_key_len,
                            char **privatekey, int *privatekey_len)
{
    CXSS_UserAuth UserAuth = {};
    
    /* Retrieve data from db */
    if (cxss_retrieveUserAuth(dbcontext, cxss_userid, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user auth\n");
        goto error;
    }
 
    /* Decrypt private key */
    if (cxss_decryptAES256(UserAuth.PrivateKey, UserAuth.KeyLength, encryption_key, 
                           UserAuth.PrivateKeyIV, privatekey, privatekey_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt private key\n");
        goto error;
    }

    *privatekey_len = UserAuth.KeyLength;                        
    cxss_freeUserAuth(&UserAuth);  
    return CXSS_MGR_SUCCESS;

error:
    free(*privatekey);
    cxss_freeUserAuth(&UserAuth);
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** @brief Retrieve user public key
 *
 *  @param cxss_userid          CXSS User ID
 *  @return                     Pointer to public key (must be freed)
 */
int
cxss_retrieveUserPublicKey(const char *cxss_userid, char **publickey, int *publickey_len)
{
    CXSS_UserData UserData = {};

    /* Retrieve data from db */
    if (cxss_retrieveUserData(dbcontext, cxss_userid, &UserData) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user data from db\n");
        goto error;    
    }
   
    /* Allocate buffer for public key */
    *publickey = malloc(UserData.KeyLength);
    if (!(*publickey)) {
        mssError(0, "CXSS", "Memory allocation error\n");
        goto error;
    }

    /* Copy key to buffer */
    memcpy(*publickey, UserData.PublicKey, UserData.KeyLength);
    *publickey_len = UserData.KeyLength;

    cxss_freeUserData(&UserData);  
    return CXSS_MGR_SUCCESS;

error:
    cxss_freeUserData(&UserData);
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** @brief Add resource to CXSS
 *  
 *  @param cxss_userid          Centrallix User ID
 *  @param resource_id          Resource ID
 *  @param resource_username    Resource Username
 *  @param username_len         Length of resource username
 *  @param resource_password    Resource Password
 *  @param password_len         Lenght of resource password
 *  @return                     Status code
 */
int
cxss_addResource(const char *cxss_userid, const char *resource_id, const char *auth_class,
                 const char *resource_username, size_t username_len,
                 const char *resource_password, size_t password_len)
{
    CXSS_UserResc UserResc = {};
    char *encrypted_username = NULL;
    char *encrypted_password = NULL;
    char *publickey = NULL;
    char *current_timestamp = get_timestamp();
    char encrypted_rand_key[512];
    char rand_key[32];
    char username_iv[16];
    char authdata_iv[16];
    int encr_username_len;
    int encr_password_len;
    int publickey_len;
    int encrypted_rand_key_len;

    /* Retrieve user publickey */
    if (cxss_retrieveUserPublicKey(cxss_userid, &publickey, &publickey_len) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user public key\n");
        goto error;
    }

    /* Generate random AES key and AES IVs */
    if (cxss_generate256bitRandomKey(rand_key) < 0) {
        mssError(0, "CXSS", "Failed to generate key\n");
        goto error;
    }
    if (cxss_generate128bitIV(username_iv) < 0) {
        mssError(0, "CXSS", "Failed to generate iv\n");
        goto error;
    }
    if (cxss_generate128bitIV(authdata_iv) < 0) {
        mssError(0, "CXSS", "Failed to generate iv\n");
        goto error;
    }

    /* Encrypt resource data with random key */
    if (cxss_encryptAES256(resource_username, username_len, rand_key, username_iv, 
                           &encrypted_username, &encr_username_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt resource username\n");
        goto error;
    }
    if (cxss_encryptAES256(resource_password, password_len, rand_key, authdata_iv,
                           &encrypted_password, &encr_password_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt resource password\n");
        goto error;
    }

    /* Encrypt random key with user's public key */
    if (cxss_encryptRSA(rand_key, sizeof(rand_key), publickey, publickey_len, 
                        encrypted_rand_key, &encrypted_rand_key_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt random key\n");
        goto error;
    }

    /* Build struct */
    UserResc.CXSS_UserID = cxss_userid;
    UserResc.ResourceID = resource_id;
    UserResc.AuthClass = auth_class;
    UserResc.AESKey = encrypted_rand_key;
    UserResc.ResourceUsername = encrypted_username;
    UserResc.ResourcePassword = encrypted_password;
    UserResc.UsernameIV = username_iv;
    UserResc.PasswordIV = authdata_iv;
    UserResc.AESKeyLength = encrypted_rand_key_len;
    UserResc.UsernameIVLength = sizeof(username_iv);
    UserResc.PasswordIVLength = sizeof(authdata_iv);
    UserResc.UsernameLength = encr_username_len;
    UserResc.PasswordLength = encr_password_len;
    UserResc.DateCreated = current_timestamp;
    UserResc.DateLastUpdated = current_timestamp;

    /* Insert */
    if (cxss_insertUserResc(dbcontext, &UserResc) < 0) {
        mssError(0, "CXSS", "Failed to insert user resource\n");
        goto error;
    }
    
    free(publickey);
    free(encrypted_username);
    free(encrypted_password);
    return CXSS_MGR_SUCCESS;

error:
    free(publickey);
    free(encrypted_username);
    free(encrypted_password);
    return CXSS_MGR_INSERT_ERROR;
}

/** @brief Get resource username/authdata from database
 *
 *  Get resource data from CXSS credentials database
 *  given CXSS user id and resource id.
 *
 *  @param cxss_userid          Centrallix User ID
 *  @param resource_id          Resource ID
 *  @return                     Status code
 */
int 
cxss_getResource(const char *cxss_userid, const char *resource_id, const char *user_key, 
                 size_t user_key_len, char **resource_username, char **resource_authdata)
{
    CXSS_UserAuth UserAuth = {};
    CXSS_UserResc UserResc = {};
    char *privatekey = NULL;
    char rand_key[32];
    int  privatekey_len;
    int  rand_key_len;
    int  ciphertext_len;

    /* Retrieve auth and resource data from database */
    if (cxss_retrieveUserAuth(dbcontext, cxss_userid, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user auth\n");
        goto error;
    }   
    if (cxss_retrieveUserResc(dbcontext, cxss_userid, resource_id, &UserResc) < 0) {
        mssError(0, "CXSS", "Failed to retrieve resource!\n");
        goto error;
    }    

    /* Decrypt user's private key using user's password-based key */
    if (cxss_decryptAES256(UserAuth.PrivateKey, UserAuth.KeyLength,
                           user_key, UserAuth.PrivateKeyIV, &privatekey, &privatekey_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt private key\n");
        goto error;
    } 

    /* Decrypt AES key using user's private key */ 
    if (cxss_decryptRSA(UserResc.AESKey, UserResc.AESKeyLength,
                        privatekey, privatekey_len, rand_key, &rand_key_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt AES key\n");
        goto error;
    }

    /* Decrypt resource username/authdata using AES key */
    if (cxss_decryptAES256(UserResc.ResourceUsername, UserResc.UsernameLength,
                           rand_key, UserResc.UsernameIV, resource_username, &ciphertext_len) < 0) {
        mssError(0, "CXSS", "Error while decrypting username\n");
        goto error;
    }
    if (cxss_decryptAES256(UserResc.ResourcePassword, UserResc.PasswordLength,
                           rand_key, UserResc.PasswordIV, resource_authdata, &ciphertext_len) < 0) {
        mssError(0, "CXSS", "Error while decrypting auth data\n");
        goto error;
    }

    free(privatekey); 
    cxss_freeUserAuth(&UserAuth);
    cxss_freeUserResc(&UserResc);
    return CXSS_MGR_SUCCESS;

error:
    free(privatekey);
    free(*resource_username);
    free(*resource_authdata);
    cxss_freeUserAuth(&UserAuth);
    cxss_freeUserResc(&UserResc);    
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** Remove user from CXSS
 *
 *  @param cxss_userid  Centrallix User ID
 *  @return             Status code
 */
int
cxss_deleteUser(const char *cxss_userid)
{
    if (cxss_deleteUserData(dbcontext, cxss_userid) < 0) {
        mssError(0, "CXSS", "Failed to delete user data\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    if (cxss_deleteAllUserAuth(dbcontext, cxss_userid) < 0) {
        mssError(0, "CXSS", "Failed to delete user auth data\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    if (cxss_deleteAllUserResc(dbcontext, cxss_userid) < 0) {
        mssError(0, "CXSS", "Failed to delete user resources\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

/** Delete a user's resource from CXSS
 *
 *  @param cxss_userid  Centrallix User ID
 *  @param resource_id  Resource ID
 *  @return             Status code
 */
int
cxss_deleteResource(const char *cxss_userid, const char *resource_id)
{
    if (cxss_deleteUserResc(dbcontext, cxss_userid, resource_id) < 0) {
        mssError(0, "CXSS", "Failed to delete resource\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

