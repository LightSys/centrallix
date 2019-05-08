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

int 
cxss_init_credentials_mgmt(void)
{   
    printf("%s\n", "Initializing database...");
    
    cxss_initialize_csprng();
    dbcontext = cxss_init_credentials_database("test.db");

    if (!dbcontext) return -1;
}

int
cxss_close_credentials_mgmt(void)
{
    return cxss_close_credentials_database(dbcontext);
}

int
cxss_adduser(const char *cxss_userid, const char *publickey, size_t keylength)
{
    char *timestamp = get_timestamp();
    
    return cxss_insert_user(dbcontext, cxss_userid, publickey, keylength, 
                            timestamp, timestamp);
}

int
cxss_adduser_auth(const char *cxss_userid, 
                  const char *encryption_key, size_t encryption_key_len,
                  const char *salt, size_t salt_len,
                  const char *privatekey, size_t privatekey_len)
{
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

    /* Store entry in database */
    current_timestamp = get_timestamp();
    cxss_insert_user_auth(dbcontext, cxss_userid, 
                          encrypted_privatekey, encr_privatekey_len, 
                          salt, 8, iv, 16, "privatekey", 0, 
                          current_timestamp, current_timestamp); 


    /* Scrub password from RAM */
    memset(encrypted_privatekey, 0, encr_privatekey_len);
    free(encrypted_privatekey);

    return 0;
}

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

