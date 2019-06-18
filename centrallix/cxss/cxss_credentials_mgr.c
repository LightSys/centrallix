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
cxssCredentialsManagerInit(void)
{   
    cxssCryptoInit();
    dbcontext = cxssCredentialsDatabaseInit("cxss.db");
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
cxssCredentialsManagerClose(void)
{
    cxssCryptoCleanup();
    cxssCredentialsDatabaseClose(dbcontext);
}

/** @brief Add user to CXSS
 *
 *  @param cxss_userid          Centrallix User ID
 *  @param pb_userk ey          Password-based user encryption key (used to encrypt private key)
 *  @param keylength            Length of password-based user encryption key
 *  @param salt                 User salt
 *  @param salt_len             Length of user salt
 *  @return                     Status code   
 */
int
cxssAddUser(const char *cxss_userid, const char *pb_userkey, size_t pb_userkey_len, 
            const char *salt, size_t salt_len)
{
    CXSS_UserData UserData = {};
    CXSS_UserAuth UserAuth = {};
    char *privatekey = NULL;
    char *publickey = NULL;
    char *encrypted_privatekey = NULL;
    char *current_timestamp = cxssGetTimestamp();
    char iv[16]; // 128-bit iv
    int privatekey_len = 0;
    int publickey_len = 0;
    int encr_privatekey_len;

    /* Generate RSA key pair */
    if (cxssGenerateRSA4096bitKeypair(&privatekey, &privatekey_len, 
                                      &publickey, &publickey_len) < 0) {
        mssError(0, "CXSS", "Failed to generate RSA keypair\n");
        goto error;
    }

    /* Generate initialization vector */
    if (cxssGenerate128bitIV(iv) < 0) {
        mssError(0, "CXSS", "Failed to generate IV\n");
        goto error;
    }

    /* Encrypt private key */
    if (cxssEncryptAES256(privatekey, privatekey_len, pb_userkey, 
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

    if (cxssInsertUserData(dbcontext, &UserData) < 0) {
        mssError(0, "CXSS", "Failed to insert user into db\n");
        goto error;
    }
    if (cxssInsertUserAuth(dbcontext, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to insert user auth into db\n");
        goto error;
    }

    free(encrypted_privatekey);
    cxssDestroyKey(privatekey, privatekey_len);
    cxssDestroyKey(publickey, publickey_len);
    return CXSS_MGR_SUCCESS;

error:
    free(encrypted_privatekey);
    cxssDestroyKey(privatekey, privatekey_len);
    cxssDestroyKey(publickey, publickey_len);
    return CXSS_MGR_INSERT_ERROR;
}

/** @brief Retrieve and decrypt user private key
 *
 *  @param cxss_userid          Centrallix User ID
 *  @param pb_userkey           Password-based user key used to encrypt private key
 *  @param pb_userkey_len       Length of password-based user encryption key
 *  @param privatekey           Pointer to pointer to a buffer to store the retrieved private key
 *  @param privatekey_len       Pointer to a variable to store the length of the retrieved private key
 *  @return                     Status code
 */
int
cxssRetrieveUserPrivateKey(const char *cxss_userid, const char *pb_userkey, size_t pb_userkey_len,
                           char **privatekey, int *privatekey_len)
{
    CXSS_UserAuth UserAuth = {};
    
    /* Retrieve data from db */
    if (cxssRetrieveUserAuth(dbcontext, cxss_userid, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user auth\n");
        goto error;
    }
 
    /* Decrypt private key */
    if (cxssDecryptAES256(UserAuth.PrivateKey, UserAuth.KeyLength, pb_userkey, 
                          UserAuth.PrivateKeyIV, privatekey, privatekey_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt private key\n");
        goto error;
    }

    *privatekey_len = UserAuth.KeyLength;
    cxssFreeUserAuth(&UserAuth);  
    return CXSS_MGR_SUCCESS;

error:
    free(*privatekey);
    cxssFreeUserAuth(&UserAuth);
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** @brief Retrieve user public key
 *
 *  @param cxss_userid          CXSS User ID
 *  @param publickey            Pointer to pointer to a buffer to store the retrieved publickey
 *  @param publickey_len        Pointer to a variable to store the length of the retrieved publickey
 *  @return                     Status code
 */
int
cxssRetrieveUserPublicKey(const char *cxss_userid, char **publickey, int *publickey_len)
{
    CXSS_UserData UserData = {};

    /* Retrieve data from db */
    if (cxssRetrieveUserData(dbcontext, cxss_userid, &UserData) < 0) {
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

    cxssFreeUserData(&UserData);  
    return CXSS_MGR_SUCCESS;

error:
    cxssFreeUserData(&UserData);
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
cxssAddResource(const char *cxss_userid, const char *resource_id, const char *auth_class,
                const char *resource_username, size_t username_len,
                const char *resource_password, size_t password_len)
{
    CXSS_UserResc UserResc = {};
    char *encrypted_username = NULL;
    char *encrypted_password = NULL;
    char *publickey = NULL;
    char *current_timestamp = cxssGetTimestamp();
    char encrypted_rand_key[512];
    char rand_key[32];
    char username_iv[16];
    char authdata_iv[16];
    int encr_username_len;
    int encr_password_len;
    int publickey_len;
    int encrypted_rand_key_len;

    /* Retrieve user publickey */
    if (cxssRetrieveUserPublicKey(cxss_userid, &publickey, &publickey_len) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user public key\n");
        goto error;
    }

    /* Generate random AES key and AES IVs */
    if (cxssGenerate256bitRandomKey(rand_key) < 0) {
        mssError(0, "CXSS", "Failed to generate key\n");
        goto error;
    }
    if (cxssGenerate128bitIV(username_iv) < 0) {
        mssError(0, "CXSS", "Failed to generate iv\n");
        goto error;
    }
    if (cxssGenerate128bitIV(authdata_iv) < 0) {
        mssError(0, "CXSS", "Failed to generate iv\n");
        goto error;
    }

    /* Encrypt resource data with random key */
    if (cxssEncryptAES256(resource_username, username_len, rand_key, username_iv, 
                          &encrypted_username, &encr_username_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt resource username\n");
        goto error;
    }
    if (cxssEncryptAES256(resource_password, password_len, rand_key, authdata_iv,
                          &encrypted_password, &encr_password_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt resource password\n");
        goto error;
    }

    /* Encrypt random key with user's public key */
    if (cxssEncryptRSA(rand_key, sizeof(rand_key), publickey, publickey_len, 
                       encrypted_rand_key, &encrypted_rand_key_len) < 0) {
        mssError(0, "CXSS", "Failed to encrypt random key\n");
        goto error;
    }

    /* Erase plaintext random key from memory */
    memset(rand_key, 0, sizeof(rand_key));

    /* Build struct */
    UserResc.CXSS_UserID = cxss_userid;
    UserResc.ResourceID = resource_id;
    UserResc.AuthClass = auth_class;
    UserResc.AESKey = encrypted_rand_key;
    UserResc.ResourceUsername = encrypted_username;
    UserResc.ResourceAuthData = encrypted_password;
    UserResc.UsernameIV = username_iv;
    UserResc.AuthDataIV = authdata_iv;
    UserResc.AESKeyLength = encrypted_rand_key_len;
    UserResc.UsernameIVLength = sizeof(username_iv);
    UserResc.AuthDataIVLength = sizeof(authdata_iv);
    UserResc.UsernameLength = encr_username_len;
    UserResc.AuthDataLength = encr_password_len;
    UserResc.DateCreated = current_timestamp;
    UserResc.DateLastUpdated = current_timestamp;

    /* Insert */
    if (cxssInsertUserResc(dbcontext, &UserResc) < 0) {
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
 *  @param pb_userkey           Password-based user key used to encrypt private key
 *  @param pb_userkey_len       Length of password-based user encryption key
 *  @param resource_username    Pointer to pointer to a buffer used to store the decrypted resource username
 *  @param resource_authdata    Pointer to pointer to a buffer used to store the decrypted resource auth data
 *  @return                     Status code
 */
int 
cxssGetResource(const char *cxss_userid, const char *resource_id, const char *pb_userkey, 
                size_t pb_userkey_len, char **resource_username, char **resource_authdata)
{
    CXSS_UserAuth UserAuth = {};
    CXSS_UserResc UserResc = {};
    char *privatekey = NULL;
    char rand_key[32];
    int rand_key_len;
    int privatekey_len = 0;
    int username_len = 0;
    int authdata_len = 0;

    /* Retrieve auth and resource data from database */
    if (cxssRetrieveUserAuth(dbcontext, cxss_userid, &UserAuth) < 0) {
        mssError(0, "CXSS", "Failed to retrieve user auth\n");
        goto error;
    }   
    if (cxssRetrieveUserResc(dbcontext, cxss_userid, resource_id, &UserResc) < 0) {
        mssError(0, "CXSS", "Failed to retrieve resource!\n");
        goto error;
    }    

    /* Decrypt user's private key using user's password-based key */
    if (cxssDecryptAES256(UserAuth.PrivateKey, UserAuth.KeyLength,
                          pb_userkey, UserAuth.PrivateKeyIV, &privatekey, &privatekey_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt private key\n");
        goto error;
    } 

    /* Decrypt AES key using user's private key */ 
    if (cxssDecryptRSA(UserResc.AESKey, UserResc.AESKeyLength,
                       privatekey, privatekey_len, rand_key, &rand_key_len) < 0) {
        mssError(0, "CXSS", "Failed to decrypt AES key\n");
        goto error;
    }

    /* Decrypt resource username/authdata using AES key */
    if (cxssDecryptAES256(UserResc.ResourceUsername, UserResc.UsernameLength,
                          rand_key, UserResc.UsernameIV, resource_username, &username_len) < 0) {
        mssError(0, "CXSS", "Error while decrypting username\n");
        goto error;
    }
    if (cxssDecryptAES256(UserResc.ResourceAuthData, UserResc.AuthDataLength,
                          rand_key, UserResc.AuthDataIV, resource_authdata, &authdata_len) < 0) {
        mssError(0, "CXSS", "Error while decrypting auth data\n");
        goto error;
    }

    cxssFreeUserAuth(&UserAuth);
    cxssFreeUserResc(&UserResc);
    cxssDestroyKey(privatekey, privatekey_len);
    return CXSS_MGR_SUCCESS;

error:
    cxssFreeUserAuth(&UserAuth);
    cxssFreeUserResc(&UserResc);
    cxssDestroyKey(privatekey, privatekey_len);
    cxssDestroyKey(*resource_username, username_len);
    cxssDestroyKey(*resource_authdata, authdata_len);    
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
    if (cxssDeleteUserData(dbcontext, cxss_userid) < 0) {
        mssError(0, "CXSS", "Failed to delete user data\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    if (cxssDeleteAllUserAuth(dbcontext, cxss_userid) < 0) {
        mssError(0, "CXSS", "Failed to delete user auth data\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    if (cxssDeleteAllUserResc(dbcontext, cxss_userid) < 0) {
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
cxssDeleteResource(const char *cxss_userid, const char *resource_id)
{
    if (cxssDeleteUserResc(dbcontext, cxss_userid, resource_id) < 0) {
        mssError(0, "CXSS", "Failed to delete resource\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

