#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 02: Test auth table";

    /** Set up DB. **/
    char * PATH = "/home/devel/test.db";

    /** Reset DB file if needed **/
    if(access(PATH, 0) == 0)
        {
        assert(remove(PATH) == 0);
        }
    assert(access(PATH, 0) != 0);

    CXSS_DB_Context_t dbCon = cxssCredentialsDatabaseInit(PATH);
    assert(dbCon != NULL);
    
    
    /*** Insert ***/

    /** insert one entry **/
    char* pubKey;
    size_t pubLen;
    char saltBuf[8];
    char ivBuf[16];

    CXSS_UserAuth data;
    data.PrivateKeyIV = &ivBuf;
    data.Salt = &saltBuf;

    /** generate valid inputs for key, salt, iv, and their lengths **/
    cxss_internal_InitEntropy(1280); 
    cxssCryptoInit(); 
    int result = cxssGenerateRSA4096bitKeypair(&data.PrivateKey, &data.KeyLength, &pubKey, &pubLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate64bitSalt(saltBuf);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate128bitIV(ivBuf);
    assert(result == CXSS_CRYPTO_SUCCESS);
    cxssCryptoCleanup();

    data.CXSS_UserID = "1";
    data.DateCreated = "01/02/2003";
    data.DateLastUpdated = "7/4/2022";
    data.RemovalFlag = 0;
    data.SaltLength = 8;
    data.IVLength = 16;

    result = cxssInsertUserAuth(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);


    /*** Retrieve ***/

    /** retrieve: **/
    CXSS_UserAuth retData;
    result = cxssRetrieveUserAuth(dbCon, data.CXSS_UserID, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(memcmp(data.Salt, retData.Salt, 8) == 0);
    assert(memcmp(data.PrivateKey, retData.PrivateKey, data.KeyLength) == 0);
    assert(memcmp(data.PrivateKeyIV, retData.PrivateKeyIV, 16) == 0);        
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);  
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.RemovalFlag == retData.RemovalFlag);  
    assert(data.KeyLength == retData.KeyLength); 
    assert(data.SaltLength == retData.SaltLength); 
    assert(data.IVLength == retData.IVLength); 

    /** retrive item not in DB **/
    CXSS_UserAuth noData;
    result = cxssRetrieveUserAuth(dbCon, "2", &noData);
    assert(result == CXSS_DB_QUERY_ERROR);


    /** NOTE: cannot perform updates on the auth table **/


    /*** Delete ***/

    /** delete entry (only way to do it is to delete all of the user's records) **/ 
    result = cxssDeleteAllUserAuth(dbCon, "1");
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserAuth(dbCon, "1", &noData); /* attempt to retrieve */
    assert(result == CXSS_DB_QUERY_ERROR);

    /** attempt to delete item not there **/
    result = cxssDeleteAllUserAuth(dbCon, "2");
    assert(result == CXSS_DB_NOENT_ERROR); 

    /** create multiple entries for deleting **/
    result = cxssInsertUserAuth(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);
    
    /** steal values from data **/
    CXSS_UserAuth data2;
    data2.PrivateKeyIV = &ivBuf;
    data2.Salt = &saltBuf;
    data2.PrivateKey = data.PrivateKey;
    data2.KeyLength = data.KeyLength;
    data2.CXSS_UserID = "1";
    data2.DateCreated = "a diff value";
    data2.DateLastUpdated = "a diff value";
    data2.RemovalFlag = 1;
    data2.SaltLength = 8;
    data2.IVLength = 16;

    result = cxssInsertUserAuth(dbCon, &data2);
    assert(result == CXSS_DB_SUCCESS);

    /** check retreival (should just be data again) **/
    result = cxssRetrieveUserAuth(dbCon, data.CXSS_UserID, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(memcmp(data.Salt, retData.Salt, 8) == 0);
    assert(memcmp(data.PrivateKey, retData.PrivateKey, data.KeyLength) == 0);
    assert(memcmp(data.PrivateKeyIV, retData.PrivateKeyIV, 16) == 0);        
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);  
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.RemovalFlag == retData.RemovalFlag);  
    assert(data.KeyLength == retData.KeyLength); 
    assert(data.SaltLength == retData.SaltLength); 
    assert(data.IVLength == retData.IVLength); 

    /** now delete both **/
    result = cxssDeleteAllUserAuth(dbCon, "1");
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserAuth(dbCon, "1", &noData); /* attempt to retrieve */
    assert(result == CXSS_DB_QUERY_ERROR);

    cxssCredentialsDatabaseClose(dbCon);

    return 0;
    }
