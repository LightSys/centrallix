#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 02: Basic auth table tests";

    /** Set up DB. **/
    char * PATH = "./tests/tempFiles/test.db";

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
    char *pubKey, *privKey;
    size_t pubLen, privLen;
    char ivBuf[16];
    
    /* generate valid inputs for key, iv, and their lengths */
    cxss_internal_InitEntropy(1280); 
    cxssCryptoInit(); 
    int result = cxssGenerateRSA4096bitKeypair(&privKey, &privLen, &pubKey, &pubLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate128bitIV(ivBuf);
    assert(result == CXSS_CRYPTO_SUCCESS);
    cxssCryptoCleanup();

    CXSS_UserAuth data;
    data.CXSS_UserID = "2";
    data.AuthClass = "something";
    data.PrivateKey = privKey;
    data.PrivateKeyIV = ivBuf;
    data.DateCreated = "03/02/2001";
    data.DateLastUpdated = "1/2/03";
    data.RemovalFlag = 0;
    data.KeyLength = strlen(data.PrivateKey);
    data.IVLength = 16;

    result = cxssInsertUserAuth(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);
    assert(data.PK_UserAuth == 1); /* should have been set by call */

    /*** Retrieve ***/

    /** retrieve: **/
    CXSS_UserAuth retData;
    result = cxssRetrieveUserAuth(dbCon, data.PK_UserAuth, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(data.PK_UserAuth == retData.PK_UserAuth);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.AuthClass, retData.AuthClass) == 0);
    assert(memcmp(data.PrivateKey, retData.PrivateKey, data.KeyLength) == 0);
    assert(memcmp(data.PrivateKeyIV, retData.PrivateKeyIV, 16) == 0);        
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);  
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.RemovalFlag == retData.RemovalFlag);  
    assert(data.KeyLength == retData.KeyLength); 
    assert(data.IVLength == retData.IVLength); 

    /** retrive item not in DB **/
    CXSS_UserAuth noData;
    result = cxssRetrieveUserAuth(dbCon, 2, &noData);
    assert(result == CXSS_DB_QUERY_ERROR);

    
    /*** Update ***/

    /** update the entry and test results **/
    data.CXSS_UserID = "3";
    data.AuthClass = "another thing";
    privKey[0] = -privKey[0]; /* points to same mem, so this is ok */
    ivBuf[0]   = -ivBuf[0];
    data.DateCreated = "02/20/2002";
    data.DateLastUpdated = "2/2/22";

    result = cxssUpdateUserAuth(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    /* now retrieve and check output */
    result = cxssRetrieveUserAuth(dbCon, data.PK_UserAuth, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(data.PK_UserAuth == retData.PK_UserAuth);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.AuthClass, retData.AuthClass) == 0);
    assert(memcmp(data.PrivateKey, retData.PrivateKey, data.KeyLength) == 0);
    assert(memcmp(data.PrivateKeyIV, retData.PrivateKeyIV, 16) == 0);        
    assert(strcmp(data.DateCreated, retData.DateCreated) != 0);  /* date created is never updated */ 
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.RemovalFlag == retData.RemovalFlag);  
    assert(data.KeyLength == retData.KeyLength); 
    assert(data.IVLength == retData.IVLength); 

    /** attempt to update nonexistent entry **/
    data.PK_UserAuth = 2;
    result = cxssUpdateUserAuth(dbCon, &data);
    assert(result == CXSS_DB_NOENT_ERROR);
    data.PK_UserAuth = 1; /* fix before next test*/


    /*** Delete ***/

    /** delete the entry and confirm is gone **/
    result = cxssDeleteUserAuth(dbCon, data.PK_UserAuth);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserAuth(dbCon, data.PK_UserAuth, &retData);
    assert(result == CXSS_DB_QUERY_ERROR);

    /** confirm duplicate delete fails **/
    result = cxssDeleteUserAuth(dbCon, data.PK_UserAuth);
    assert(result == CXSS_DB_NOENT_ERROR);

    cxssCredentialsDatabaseClose(dbCon);

    return 0;
    }
