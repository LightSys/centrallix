#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 04: Delete All Auth";

    int n = 100;

    /** Set up DB. **/
    char * PATH = "/home/devel/test.db";
    /* Reset DB file if needed */
    if(access(PATH, 0) == 0)
        {
        assert(remove(PATH) == 0);
        }
    assert(access(PATH, 0) != 0);
    CXSS_DB_Context_t dbCon = cxssCredentialsDatabaseInit(PATH);
    assert(dbCon != NULL);

    /* generate valid inputs for key, iv, and their lengths */
    char *pubKey, *privKey;
    size_t pubLen, privLen;
    char ivBuf[16];
    
    cxss_internal_InitEntropy(1280); 
    cxssCryptoInit(); 
    int result = cxssGenerateRSA4096bitKeypair(&privKey, &privLen, &pubKey, &pubLen);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate128bitIV(ivBuf);
    assert(result == CXSS_CRYPTO_SUCCESS);
    cxssCryptoCleanup();

    CXSS_UserAuth data, temp;
    data.PK_UserAuth = 1;
    data.CXSS_UserID = "1";
    data.AuthClass = "something";
    data.PrivateKey = privKey;
    data.PrivateKeyIV = ivBuf;
    data.DateCreated = "03/02/2001";
    data.DateLastUpdated = "1/2/03";
    data.RemovalFlag = 0;
    data.KeyLength = strlen(data.PrivateKey);
    data.IVLength = 16;

    for(int i = 0 ; i < n ; i++)
        {
        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    /* delete all and confirm all were deleted */
    result = cxssDeleteAllUserAuth(dbCon, "1", NULL);
    assert(result == CXSS_DB_SUCCESS);

    for(int i = 0 ; i < n ; i++)
        {
        result = cxssRetrieveUserAuth(dbCon, i, &temp);
        assert(result == CXSS_DB_QUERY_ERROR);
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserAuth(dbCon, "1", NULL);
    assert(result == CXSS_DB_NOENT_ERROR);

    /** Delete by user **/
    for(int i = 0 ; i < n ; i++)
        {
        data.CXSS_UserID = (i < n/2)? "1":"2";
        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    /* delete all from user and confirm */
    result = cxssDeleteAllUserAuth(dbCon, "1", NULL);
    assert(result == CXSS_DB_SUCCESS);

    for(int i = 0 ; i < n ; i++)
        {
        result = cxssRetrieveUserAuth(dbCon, i+1, &temp);
        assert(result == ((i < n/2)? CXSS_DB_QUERY_ERROR : CXSS_DB_SUCCESS));
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserAuth(dbCon, "1", NULL);
    assert(result == CXSS_DB_NOENT_ERROR);

    /** test delete by user and auth class **/
    for(int i = 0 ; i < n ; i++)
        {
        /* split users into 4 groups */
        data.CXSS_UserID = (i < n/2)? "1":"2";
        data.AuthClass = (i < n/4 || i > n*3/4)? "1":"2";
        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }
    result = cxssDeleteAllUserAuth(dbCon, "1", "1"); /* deletes first 4th */
    assert(result == CXSS_DB_SUCCESS);
    for(int i = 0 ; i < n ; i++)
        {
        result = cxssRetrieveUserAuth(dbCon, i+n+1, &temp);
        assert(result == ((i < n/4)? CXSS_DB_QUERY_ERROR : CXSS_DB_SUCCESS));
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserAuth(dbCon, "1", "1");
    assert(result == CXSS_DB_NOENT_ERROR);

    cxssCredentialsDatabaseClose(dbCon);
    return 0;
    }
