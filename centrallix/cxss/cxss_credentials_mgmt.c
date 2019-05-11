#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/bio.h>
#include "cxss_credentials_mgmt.h"
#include "cxss_credentials_db.h"
#include "cxss_encryption.h"

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
    
    cxss_initialize_csprng();
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
    return cxss_close_credentials_database(dbcontext);
}

/** @brief Add user to CXSS
 *
 *  Add user to CXSS
 *
 *  @param cxss_userid          Centrallix user identity
 *  @param publickey            User public key
 *  @param keylength            Length of user public key
 *  @return                     Status code   
 */
int
cxss_adduser(const char *cxss_userid, const char *publickey, size_t keylength)
{
    CXSS_UserData UserData;
    char *current_timestamp;

    /* Check if user is already in database */
    if (cxss_db_contains_user(dbcontext, cxss_userid)) {
        fprintf(stderr, "User is already in database!\n");
        return -1;
    }

    /* Get current timestamp */
    current_timestamp = get_timestamp();
    
    /* Build struct */
    UserData.CXSS_UserID = (char*)cxss_userid;
    UserData.PublicKey = (char*)publickey;
    UserData.KeyLength = keylength;
    UserData.DateCreated = current_timestamp;
    UserData.DateLastUpdated = current_timestamp;

    return cxss_insert_user(dbcontext, &UserData);
}

int
cxss_adduser_auth(const char *cxss_userid, 
                  const char *encryption_key, size_t encryption_key_len,
                  const char *salt, size_t salt_len,
                  const char *privatekey, size_t privatekey_len)
{
    CXSS_UserAuth UserAuth;
    char *current_timestamp;
    char iv[16]; /* 128-bit iv */
    char *encrypted_privatekey;
    size_t encr_privatekey_len;

    /* Generate initialization vector */
    if (cxss_generate_128bit_iv(iv) < 0) {
        fprintf(stderr, "Failed to generate IV\n");
        return -1;
    }

    /* Allocate buffer for encrypted password */
    encr_privatekey_len = cxss_aes256_ciphertext_length(privatekey_len);
    encrypted_privatekey = malloc(sizeof(char) * encr_privatekey_len);
    if (!encrypted_privatekey) {
        fprintf(stderr, "Memory allocation error!\n");
        return -1;
    }

    /* Encrypt password */
    encr_privatekey_len = cxss_encrypt_aes256(privatekey, privatekey_len, 
                                              encryption_key, iv, 
                                              encrypted_privatekey);
    if (encr_privatekey_len < 0) {
        fprintf(stderr, "Error while encrypting pwd\n");
        return -1;
    }

    /* Generate timestamp */
    current_timestamp = get_timestamp();

    /* Build struct */
    UserAuth.CXSS_UserID = (char*)cxss_userid;
    UserAuth.PrivateKey = (char*)encrypted_privatekey;
    UserAuth.KeyLength = encr_privatekey_len;
    UserAuth.Salt = (char*)salt;
    UserAuth.SaltLength = 8;
    UserAuth.UserIV = iv;
    UserAuth.IVLength = 16;
    UserAuth.AuthClass = "privatekey"; // Arbitrary, so far
    UserAuth.RemovalFlag = 0;
    UserAuth.DateCreated = current_timestamp;
    UserAuth.DateLastUpdated = current_timestamp;

    /* Store entry in database */
    cxss_insert_user_auth(dbcontext, &UserAuth); 

    /* Scrub password from RAM */
    memset(encrypted_privatekey, 0, encr_privatekey_len);
    free(encrypted_privatekey);

    return 0;
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
                              size_t ecryption_key_len)
{
    char *plaintext;
    CXSS_UserAuth UserAuth;
    
    /* Retrieve data from db */
    cxss_retrieve_user_auth(dbcontext, cxss_userid, &UserAuth);
 
    /* Decrypt */
    plaintext = malloc(sizeof(char) * UserAuth.KeyLength);
    if (!plaintext) return NULL;

    cxss_decrypt_aes256(UserAuth.PrivateKey, UserAuth.KeyLength, encryption_key,
                        UserAuth.UserIV, plaintext);
                        
    cxss_free_userauth(&UserAuth);  
    return plaintext;
}

/** @brief Add resource to CXSS
 *
 */
int
cxss_add_resource(const char *cxss_userid, const char *resource_id,
                  const char *resource_username, size_t username_len,
                  const char *resource_password, size_t password_len)
{
    CXSS_UserResc UserResc;
    
    memset(&UserResc, 0, sizeof(CXSS_UserResc));

    UserResc.CXSS_UserID = (char*)cxss_userid;
    UserResc.ResourceID = (char*)resource_id;
    UserResc.ResourceUsername = (char*)resource_username;
    UserResc.UsernameLength = username_len;
    UserResc.ResourcePassword = (char*)resource_password;
    UserResc.PasswordLength = password_len;

    /* Insert */
    if (cxss_insert_user_resc(dbcontext, &UserResc) < 0) {
        fprintf(stderr, "Failed to insert user resource\n");
        return -1;
    }
    
    return 0;
}

int 
cxss_get_resource(const char *cxss_userid, const char *resource_id)
{
    CXSS_UserResc UserResc;
    
    if (cxss_retrieve_user_resc(dbcontext, cxss_userid, resource_id,
                                &UserResc) < 0) {
        fprintf(stderr, "Failed to retrieve resource!\n");
        return -1;
    }

    printf("%s\n", UserResc.ResourcePassword);

    cxss_free_userresc(&UserResc);
    return 0;
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

