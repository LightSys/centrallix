#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/bio.h>
#include "cxss_credentials_mgmt.h"
#include "cxss_credentials_db.h"
#include "cxss_encryption.h"
#include <assert.h>
static DB_Context_t dbcontext = NULL;

/** @brief Initialize CXSS credentials mgmt
 *
 *  Initialize the CXSS credentials mgmt
 *  system (load database, etc...).
 *
 *  @return     Status code
 */
int 
cxss_init_credentials_mgmt(void)
{   
    printf("%s\n", "Initializing database...");
    
    cxss_initialize_crypto();
    dbcontext = cxss_init_credentials_database("test.db");

    if (!dbcontext) return -1;
}

/** @brief Close CXSS credentials mgmt
 *
 *  Close the CXSS credentials mgmt
 *  system (close database, free data structures, etc...)
 *
 *  @return     Status code
 */
int
cxss_close_credentials_mgmt(void)
{
    cxss_cleanup_crypto();
    return cxss_close_credentials_database(dbcontext);
}

/** @brief Add user to CXSS
 *
 *  Add user to CXSS
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param encryption_key       Password-based user encryption key
 *  @param keylength            Length of user encryption key
 *  @param salt                 User salt
 *  @param salt_len             Length of user salt
 *  @return                     Status code   
 */
int
cxss_adduser(const char *cxss_userid, const char *encryption_key, 
             size_t encryption_key_len, const char *salt, size_t salt_len)
{
    CXSS_UserData UserData;
    CXSS_UserAuth UserAuth;
    char *privatekey = NULL, *publickey = NULL;
    char *encrypted_privatekey = NULL;
    char iv[16]; // 128-bit iv
    char *current_timestamp;
    int privatekey_len, publickey_len;
    int encr_privatekey_len;
    int predicted_encr_len; 

    /* Check if user is already in database */
    if (cxss_db_contains_user(dbcontext, cxss_userid)) {
        fprintf(stderr, "User is already in database!\n");
        goto error;
    }

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

    /* Allocate buffer for encrypted private key */
    predicted_encr_len = cxss_aes256_ciphertext_length(privatekey_len);
    encrypted_privatekey = malloc(sizeof(char) * predicted_encr_len);
    if (!encrypted_privatekey) {
        fprintf(stderr, "Memory allocation error!\n");
        goto error;
    }

    /* Encrypt private key */
    encr_privatekey_len = cxss_encrypt_aes256(privatekey, privatekey_len, 
                                              encryption_key, iv, 
                                              encrypted_privatekey);
 
    if (encr_privatekey_len != predicted_encr_len) {
        fprintf(stderr, "Error while encrypting with user key\n");
        goto error;
    }

    /* Get current timestamp */
    current_timestamp = get_timestamp();
    
    UserData.CXSS_UserID = (char*)cxss_userid;
    UserData.PublicKey = publickey;
    UserData.KeyLength = publickey_len;
    UserData.DateCreated = current_timestamp;
    UserData.DateLastUpdated = current_timestamp;
    UserAuth.CXSS_UserID = (char*)cxss_userid;
    UserAuth.PrivateKey = encrypted_privatekey;
    UserAuth.KeyLength = encr_privatekey_len;
    UserAuth.Salt = (char*)salt;
    UserAuth.SaltLength = 8;
    UserAuth.UserIV = iv;
    UserAuth.IVLength = 16;
    UserAuth.AuthClass = "privatekey"; // Arbitrary, so far
    UserAuth.RemovalFlag = 0;
    UserAuth.DateCreated = current_timestamp;
    UserAuth.DateLastUpdated = current_timestamp;

    if (cxss_insert_user(dbcontext, &UserData) < 0) {
        fprintf(stderr, "Failed to insert user into db\n");
        goto error;
    }

    if (cxss_insert_user_auth(dbcontext, &UserAuth) < 0) {
        fprintf(stderr, "Failed to insert user into db\n");
        goto error;
    }

    /* Erase private key from RAM */
    memset(privatekey, 0, sizeof(privatekey));

    cxss_free_rsa_keypair(privatekey, publickey);
    free(encrypted_privatekey);
    return 0;

error:
    cxss_free_rsa_keypair(privatekey, publickey);
    free(encrypted_privatekey);
    return -1;
}

/** @brief Retrieve user private key
 *
 *  Retrieve and decrypt user private key
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param encryption_key       AES encryption key used to 
 *                              encrypt the private key
 *  @param encryption_key_len   Length of encryption key
 *  @return                     Pointer to decrypted private key (must be freed)
 */
char *
cxss_retrieve_user_privatekey(const char *cxss_userid, 
                              const char *encryption_key, 
                              size_t ecryption_key_len,
                              int *privatekey_len)
{
    char *plaintext;
    CXSS_UserAuth UserAuth;
    
    /* Retrieve data from db */
    if (cxss_retrieve_user_auth(dbcontext, cxss_userid, &UserAuth) < 0) {
        fprintf(stderr, "Failed to retrieve user auth\n");
        return NULL;
    }
 
    /* Decrypt */
    plaintext = malloc(sizeof(char) * UserAuth.KeyLength);
    if (!plaintext) return NULL;

    if (cxss_decrypt_aes256(UserAuth.PrivateKey, UserAuth.KeyLength, 
                            encryption_key, UserAuth.UserIV, plaintext) < 0) {
        fprintf(stderr, "Failed to decrypt private key\n");
        cxss_free_userauth(&UserAuth);
        free(plaintext);
        return NULL;
    }
    *privatekey_len = UserAuth.KeyLength;
                        
    cxss_free_userauth(&UserAuth);  
    return plaintext;
}

/** @brief Retrieve user public key
 *
 *  Retrieve user public key
 *
 *  @param cxss_userid          Centrallix user identity
 *  @return                     Pointer to public key (must be freed)
 */
char *
cxss_retrieve_user_publickey(const char *cxss_userid, int *publickey_len)
{
    char *publickey;
    CXSS_UserData UserData;

    /* Retrieve data from db */
    cxss_retrieve_user(dbcontext, cxss_userid, &UserData);
    
    publickey = malloc(UserData.KeyLength);
    if (!publickey) {
        fprintf(stderr, "Memory allocation error!\n");
        return NULL;
    }
    memcpy(publickey, UserData.PublicKey, UserData.KeyLength);
    *publickey_len = UserData.KeyLength;

    cxss_free_userdata(&UserData);  
    return publickey;
}

/** @brief Add resource to CXSS
 *  
 *  Insert resource into CXSS credentials database
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param resource_id          Resource ID
 *  @param resource_username    Resource Username
 *  @param username_len         Length of resource username
 *  @param resource_password    Resource Password
 *  @param password_len         Lenght of resource password
 *  @return                     Status code
 */
int
cxss_add_resource(const char *cxss_userid, const char *resource_id,
                  const char *resource_username, size_t username_len,
                  const char *resource_password, size_t password_len)
{
    CXSS_UserResc UserResc;
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

    /* Check if resource is already in database */
    if (cxss_db_contains_resc(dbcontext, resource_id)) {
        fprintf(stderr, "Database already contains resource\n");
        goto error;
    }

    /* Retrieve user publickey */
    publickey = cxss_retrieve_user_publickey(cxss_userid, &publickey_len);
    if (!publickey) {
        fprintf(stderr, "Failed to retrieve public key\n");
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

    encr_username_len = cxss_aes256_ciphertext_length(username_len);
    encr_password_len = cxss_aes256_ciphertext_length(password_len);
    
    encrypted_username = malloc(encr_username_len);
    encrypted_password = malloc(encr_password_len);
    if (!encrypted_username || !encrypted_password) {
        fprintf(stderr, "Memory allocation error\n");
        goto error;
    }

    /* Encrypt resource data with random key */
    if (cxss_encrypt_aes256(resource_username, username_len, key, uname_iv, 
                            encrypted_username) < 0) {
        fprintf(stderr, "Failed to encrypt resource username\n");
        goto error;
    }
    if (cxss_encrypt_aes256(resource_password, password_len, key, pwd_iv,
                            encrypted_password) < 0) {
        fprintf(stderr, "Failed to encrypt resource password\n");
        goto error;
    }

    /* Encrypt random key with user's public key */
    encrypted_rand_key_len = cxss_encrypt_rsa(key, sizeof(key),
                                              publickey, publickey_len, 
                                              encrypted_rand_key);
    if (encrypted_rand_key_len < 0) {
        fprintf(stderr, "Failed to encrypt random key\n");
        goto error;
    }

    /* Build struct */
    memset(&UserResc, 0, sizeof(CXSS_UserResc));
    UserResc.CXSS_UserID = (char*)cxss_userid;
    UserResc.ResourceID = (char*)resource_id;
    UserResc.AESKey = encrypted_rand_key;
    UserResc.AESKeyLength = encrypted_rand_key_len;
    UserResc.UsernameIV = uname_iv;
    UserResc.UsernameIVLength = sizeof(uname_iv);
    UserResc.PasswordIV = pwd_iv;
    UserResc.PasswordIVLength = sizeof(pwd_iv);
    UserResc.ResourceUsername = encrypted_username;
    UserResc.UsernameLength = encr_username_len;
    UserResc.ResourcePassword = encrypted_password;
    UserResc.PasswordLength = encr_password_len;

    /* Insert */
    if (cxss_insert_user_resc(dbcontext, &UserResc) < 0) {
        fprintf(stderr, "Failed to insert user resource\n");
        goto error;
    }
    
    free(publickey);
    free(encrypted_username);
    free(encrypted_password);
    return 0;

error:
    free(publickey);
    free(encrypted_username);
    free(encrypted_password);
    return -1;
}

/** @brief Get resource data from database
 *
 *  Get resource data from CXSS credentials database
 *  given CXSS user id and resource id.
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param resource_id          Resource ID
 *  @return                     Status code
 */
int 
cxss_get_resource(const char *cxss_userid, const char *resource_id,
                  const char *user_key, size_t user_key_len)
{
    CXSS_UserAuth UserAuth;
    CXSS_UserResc UserResc;
    char aeskey[32];
    char ciphertext[512];
    char *privatekey = NULL;
    int  privatekey_len;
    int  aeskey_len;
    int  ciphertext_len;

    memset(&UserAuth, 0, sizeof(CXSS_UserAuth));
    if (cxss_retrieve_user_auth(dbcontext, cxss_userid, &UserAuth) < 0) {
        fprintf(stderr, "Failed to retrieve user auth\n");
        goto free_all;
    }   
    memset(&UserResc, 0, sizeof(CXSS_UserResc));    
    if (cxss_retrieve_user_resc(dbcontext, cxss_userid, resource_id,
                                &UserResc) < 0) {
        fprintf(stderr, "Failed to retrieve resource!\n");
        goto free_all;
    }

    /* Alloc private key buffer */
    privatekey = malloc(cxss_aes256_ciphertext_length(UserAuth.KeyLength));
    if (!privatekey) {
        fprintf(stderr, "Memory allocation error\n");
        goto free_all;
    }

    /* Decrypt private key */
    privatekey_len = cxss_decrypt_aes256(UserAuth.PrivateKey, 
                                         UserAuth.KeyLength,
                                         user_key, UserAuth.UserIV,
                                         privatekey);
    if (privatekey_len < 0) {
        fprintf(stderr, "Failed to decrypt private key\n");
        goto free_all;
    } 

    printf("LENGTH: %ld\n", UserResc.AESKeyLength);
     
    /* Decrypt AES key */ 
    aeskey_len = cxss_decrypt_rsa(UserResc.AESKey, 
                                  UserResc.AESKeyLength, 
                                  privatekey, privatekey_len, aeskey);
    if (aeskey_len < 0) {
        fprintf(stderr, "Failed to decrypt AES key\n");
        goto free_all;
    }

    /* Decrypt username & passowrd */ 
    ciphertext_len =  cxss_decrypt_aes256(UserResc.ResourceUsername,
                                          UserResc.UsernameLength,
                                          aeskey, UserResc.UsernameIV,
                                          ciphertext);
    if (ciphertext_len < 0) {
        fprintf(stderr, "Failed to decrypt resource username\n");
        goto free_all;
    }
    printf("USERNAME: %s\n", ciphertext);

    ciphertext_len =  cxss_decrypt_aes256(UserResc.ResourcePassword,
                                          UserResc.PasswordLength,
                                          aeskey, UserResc.PasswordIV,
                                          ciphertext);
    if (ciphertext_len < 0) {
        fprintf(stderr, "Failed to decrypt resource password\n");
        goto free_all;
    }
    printf("PASSWORD: %s\n", ciphertext);

    free(privatekey); 
    cxss_free_userauth(&UserAuth);
    cxss_free_userresc(&UserResc);
    return 0;

free_all:
    free(privatekey);
    cxss_free_userauth(&UserAuth);
    cxss_free_userresc(&UserResc);    
    return -1;
}


/** @brief Get current timestamp
 *
 *  This function returns the current timestamp
 *  stored in a static buffer. Hence, the buffer
 *  does not need to be freed.
 *
 *  @return     timestamp
 */    
static char *
get_timestamp(void)
{
    time_t current_time;
    char *timestamp;

    current_time = time(NULL);
    if (current_time == (time_t)-1) {
        fprintf(stderr, "Failed to retrieve system time\n");
        exit(EXIT_FAILURE);
    }

    timestamp = ctime(&current_time);
    if (!timestamp) {
        fprintf(stderr, "Failed to generate timestamp\n");
        exit(EXIT_FAILURE);
    }

    /* Remove pesky newline */
    for (char *ch = timestamp; *ch != '\0'; ch++) {
        if (*ch == '\n') {
            *ch = '\0';
            break;
        }
    }

    return timestamp;
}

