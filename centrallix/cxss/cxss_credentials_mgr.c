#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <openssl/bio.h>
#include "cxss_util.h"
#include "cxss_credentials_mgr.h"
#include "cxss_credentials_db.h"
#include "cxss_encryption.h"
#include <assert.h>

static CXSS_DB_Context_t dbcontext = NULL;

/** @brief Init CXSS credentials mgr
 *
 *  @return     Status code
 */
int 
cxss_init_credentials_mgmt(void)
{   
    cxss_initialize_crypto();
    dbcontext = cxss_init_credentials_database("test.db");
    if (!dbcontext) {
        return CXSS_MGR_INIT_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

/** @brief Close CXSS credentials mgr
 *
 *  Close the CXSS credentials manager
 *  (close database, free data structures, etc...)
 *
 *  @return     Status code
 */
int
cxss_close_credentials_mgmt(void)
{
    cxss_cleanup_crypto();
    if (cxss_close_credentials_database(dbcontext) < 0) {
        return CXSS_MGR_CLOSE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

/** @brief Add user to CXSS
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param encryption_key       Password-based user encryption key
 *  @param keylength            Length of user encryption key
 *  @param salt                 User salt
 *  @param salt_len             Length of user salt
 *  @return                     Status code   
 */
int
cxss_adduser(const char *cxss_userid, const char *encryption_key, size_t encryption_key_len, 
             const char *salt, size_t salt_len)
{
    CXSS_UserData UserData = {};
    CXSS_UserAuth UserAuth = {};
    char *privatekey = NULL, *publickey = NULL;
    char *encrypted_privatekey = NULL;
    char iv[16]; // 128-bit iv
    char *current_timestamp = get_timestamp();
    int privatekey_len, publickey_len;
    int encr_privatekey_len;
    int predicted_encr_len; 

    /* Generate RSA key pair */
    if (cxss_generate_rsa_4096bit_keypair(&privatekey, &privatekey_len, 
                                          &publickey, &publickey_len) < 0) {
        fprintf(stderr, "Failed to generate RSA keypair\n");
        goto error;
    }

    /* Generate initialization vector */
    if (cxss_generate_128bit_iv(iv) < 0) {
        fprintf(stderr, "Failed to generate IV\n");
        goto error;
    }

    /* Encrypt private key */
    if (cxss_encrypt_aes256(privatekey, privatekey_len, encryption_key, 
                            iv, &encrypted_privatekey, &encr_privatekey_len) < 0) {
        fprintf(stderr, "Error while encrypting private key\n");        
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

    if (cxss_insert_userdata(dbcontext, &UserData) < 0) {
        fprintf(stderr, "Failed to insert user into db\n");
        goto error;
    }
    if (cxss_insert_userauth(dbcontext, &UserAuth) < 0) {
        fprintf(stderr, "Failed to insert user auth into db\n");
        goto error;
    }

    free(encrypted_privatekey);
    cxss_destroy_rsa_keypair(privatekey, privatekey_len, publickey, publickey_len);
    return CXSS_MGR_SUCCESS;

error:
    free(encrypted_privatekey);
    free(privatekey);
    free(publickey);         
    return CXSS_MGR_INSERT_ERROR;
}

/** @brief Retrieve and decrypt user private key
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param encryption_key       AES encryption key used to encrypt private key
 *  @param encryption_key_len   Length of encryption key
 *  @return                     Pointer to decrypted private key (must be freed)
 */
int
cxss_retrieve_user_privatekey(const char *cxss_userid, const char *encryption_key, size_t ecryption_key_len,
                              char **privatekey, int *privatekey_len)
{
    CXSS_UserAuth UserAuth = {};
    
    /* Retrieve data from db */
    if (cxss_retrieve_userauth(dbcontext, cxss_userid, &UserAuth) < 0) {
        fprintf(stderr, "Failed to retrieve user auth\n");
        goto error;
    }
 
    /* Decrypt private key */
    if (cxss_decrypt_aes256(UserAuth.PrivateKey, UserAuth.KeyLength, encryption_key, 
                            UserAuth.PrivateKeyIV, privatekey, privatekey_len) < 0) {
        fprintf(stderr, "Failed to decrypt private key\n");
        goto error;
    }

    *privatekey_len = UserAuth.KeyLength;                        
    cxss_free_userauth(&UserAuth);  
    return CXSS_MGR_SUCCESS;

error:
    free(*privatekey);
    cxss_free_userauth(&UserAuth);
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** @brief Retrieve user public key
 *
 *  @param cxss_userid          CXSS user identity
 *  @return                     Pointer to public key (must be freed)
 */
int
cxss_retrieve_user_publickey(const char *cxss_userid, char **publickey, int *publickey_len)
{
    CXSS_UserData UserData = {};

    /* Retrieve data from db */
    if (cxss_retrieve_userdata(dbcontext, cxss_userid, &UserData) < 0) {
        fprintf(stderr, "Failed to retrieve user data from db\n");
        goto error;    
    }
   
    /* Allocate buffer for public key */
    *publickey = malloc(UserData.KeyLength);
    if (!(*publickey)) {
        fprintf(stderr, "Memory allocation error!\n");
        goto error;
    }

    /* Copy key to buffer */
    memcpy(*publickey, UserData.PublicKey, UserData.KeyLength);
    *publickey_len = UserData.KeyLength;

    cxss_free_userdata(&UserData);  
    return CXSS_MGR_SUCCESS;

error:
    cxss_free_userdata(&UserData);
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** @brief Add resource to CXSS
 *  
 *  @param cxss_userid          Centrallix user identity
 *  @param resource_id          Centrallix resource identity
 *  @param resource_username    Resource Username
 *  @param username_len         Length of resource username
 *  @param resource_password    Resource Password
 *  @param password_len         Lenght of resource password
 *  @return                     Status code
 */
int
cxss_add_resource(const char *cxss_userid, const char *resource_id, const char *auth_class,
                  const char *resource_username, size_t username_len,
                  const char *resource_password, size_t password_len)
{
    CXSS_UserResc UserResc = {};
    char *encrypted_username = NULL;
    char *encrypted_password = NULL;
    int encr_username_len;
    int encr_password_len;
    char *publickey = NULL;
    int publickey_len;
    char encrypted_rand_key[512];
    int encrypted_rand_key_len;
    char key[32];
    char uname_iv[16];
    char pwd_iv[16];
    char *current_timestamp = get_timestamp();

    /* Retrieve user publickey */
    if (cxss_retrieve_user_publickey(cxss_userid, &publickey, &publickey_len) < 0) {
        fprintf(stderr, "Failed to retrieve user public key\n");
        goto error;
    }

    /* Generate random AES key and AES IVs */
    if (cxss_generate_256bit_rand_key(key) < 0) {
        fprintf(stderr, "Failed to generate key\n");
        goto error;
    }
    if (cxss_generate_128bit_iv(uname_iv) < 0) {
        fprintf(stderr, "Failed to generate iv\n");
        goto error;
    }
    if (cxss_generate_128bit_iv(pwd_iv) < 0) {
        fprintf(stderr, "Failed to generate iv\n");
        goto error;
    }

    /* Encrypt resource data with random key */
    if (cxss_encrypt_aes256(resource_username, username_len, key, uname_iv, 
                            &encrypted_username, &encr_username_len) < 0) {
        fprintf(stderr, "Failed to encrypt resource username\n");
        goto error;
    }
    if (cxss_encrypt_aes256(resource_password, password_len, key, pwd_iv,
                            &encrypted_password, &encr_password_len) < 0) {
        fprintf(stderr, "Failed to encrypt resource password\n");
        goto error;
    }

    /* Encrypt random key with user's public key */
    encrypted_rand_key_len = cxss_encrypt_rsa(key, sizeof(key), publickey, publickey_len, 
                                              encrypted_rand_key);
    if (encrypted_rand_key_len < 0) {
        fprintf(stderr, "Failed to encrypt random key\n");
        goto error;
    }

    /* Build struct */
    UserResc.CXSS_UserID = cxss_userid;
    UserResc.ResourceID = resource_id;
    UserResc.AuthClass = auth_class;
    UserResc.AESKey = encrypted_rand_key;
    UserResc.ResourceUsername = encrypted_username;
    UserResc.ResourcePassword = encrypted_password;
    UserResc.UsernameIV = uname_iv;
    UserResc.PasswordIV = pwd_iv;
    UserResc.AESKeyLength = encrypted_rand_key_len;
    UserResc.UsernameIVLength = sizeof(uname_iv);
    UserResc.PasswordIVLength = sizeof(pwd_iv);
    UserResc.UsernameLength = encr_username_len;
    UserResc.PasswordLength = encr_password_len;
    UserResc.DateCreated = current_timestamp;
    UserResc.DateLastUpdated = current_timestamp;

    /* Insert */
    if (cxss_insert_userresc(dbcontext, &UserResc) < 0) {
        fprintf(stderr, "Failed to insert user resource\n");
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
 *  @param cxss_userid          Centrallix user identity
 *  @param resource_id          Centrallix resource identity
 *  @return                     Status code
 */
int 
cxss_get_resource(const char *cxss_userid, const char *resource_id, const char *user_key, 
                  size_t user_key_len, char **resource_username, char **resource_authdata)
{
    CXSS_UserAuth UserAuth = {};
    CXSS_UserResc UserResc = {};
    char aeskey[32];
    char *privatekey = NULL;
    int  privatekey_len;
    int  aeskey_len;
    int  ciphertext_len;

    /* Retrieve auth and resource data from database */
    if (cxss_retrieve_userauth(dbcontext, cxss_userid, &UserAuth) < 0) {
        fprintf(stderr, "Failed to retrieve user auth\n");
        goto error;
    }   
    if (cxss_retrieve_userresc(dbcontext, cxss_userid, resource_id, &UserResc) < 0) {
        fprintf(stderr, "Failed to retrieve resource!\n");
        goto error;
    }    

    /* Decrypt user's private key using user's password-based key */
    if (cxss_decrypt_aes256(UserAuth.PrivateKey, UserAuth.KeyLength,
                            user_key, UserAuth.PrivateKeyIV, &privatekey, &privatekey_len) < 0) {
        fprintf(stderr, "Failed to decrypt private key\n");
        goto error;
    } 

    /* Decrypt AES key using user's private key */ 
    aeskey_len = cxss_decrypt_rsa(UserResc.AESKey, UserResc.AESKeyLength, 
                                  privatekey, privatekey_len, aeskey);
    if (aeskey_len < 0) {
        fprintf(stderr, "Failed to decrypt AES key\n");
        goto error;
    }

    /* Decrypt resource username/authdata using AES key */
    if (cxss_decrypt_aes256(UserResc.ResourceUsername, UserResc.UsernameLength,
                            aeskey, UserResc.UsernameIV, resource_username, &ciphertext_len) < 0) {
        fprintf(stderr, "Error while decrypting username\n");
        goto error;
    }
    if (cxss_decrypt_aes256(UserResc.ResourcePassword, UserResc.PasswordLength,
                            aeskey, UserResc.PasswordIV, resource_authdata, &ciphertext_len) < 0) {
        fprintf(stderr, "Error while decrypting auth data\n");
        goto error;
    }

    free(privatekey); 
    cxss_free_userauth(&UserAuth);
    cxss_free_userresc(&UserResc);
    return CXSS_MGR_SUCCESS;

error:
    free(privatekey);
    free(*resource_username);
    free(*resource_authdata);
    cxss_free_userauth(&UserAuth);
    cxss_free_userresc(&UserResc);    
    return CXSS_MGR_RETRIEVE_ERROR;
}

/** Remove user from CXSS
 *
 *  @param cxss_userid  Centrallix user identity
 *  @return             Status code
 */
int
cxss_delete_user(const char *cxss_userid)
{
    if (cxss_delete_userdata(dbcontext, cxss_userid) < 0) {
        fprintf(stderr, "Failed to delete user data\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

/** Delete a user's resource from CXSS
 *
 *  @param cxss_userid  Centrallix user identity
 *  @param resource_id  Resource identity
 *  @return             Status code
 */
int
cxss_delete_resource(const char *cxss_userid, const char *resource_id)
{
    if (cxss_delete_userresc(dbcontext, cxss_userid, resource_id) < 0) {
        fprintf(stderr, "Failed to delete resource\n");
        return CXSS_MGR_DELETE_ERROR;
    }
    return CXSS_MGR_SUCCESS;
}

