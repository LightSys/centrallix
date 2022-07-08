#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 06: Delete All Resc";

    int n = 100; /* WARNING: if set to higher than 255, things will break */

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


    /* generate valid inputs for key, salt, iv, and their lengths */
    char resc[2];
    resc[1] = '\0';
    char key[32];
    char uNameIV[16];
    char aDataIV[16];
    cxss_internal_InitEntropy(1280); 
    cxssCryptoInit(); 
    int result = cxssGenerate256bitRandomKey(&key);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate64bitSalt(uNameIV);
    assert(result == CXSS_CRYPTO_SUCCESS);
    result = cxssGenerate128bitIV(aDataIV);
    assert(result == CXSS_CRYPTO_SUCCESS);
    cxssCryptoCleanup();


    /** insert n entries **/
    CXSS_UserResc data, temp;
    data.CXSS_UserID = "1";
    data.ResourceID = resc;
    data.AESKey = key; 
    data.UsernameIV = uNameIV;
    data.AuthDataIV = aDataIV;
    data.ResourceUsername = "aUsername";
    data.ResourceAuthData = "some auth data";
    data.DateCreated = "7/5/2022";
    data.DateLastUpdated = "7/4/2022";
    data.AESKeyLength = 32;
    data.UsernameIVLength = 16;
    data.AuthDataIVLength = 16;
    data.UsernameLength = strlen(data.ResourceUsername);
    data.AuthDataLength = strlen(data.ResourceAuthData);

    assert(result == CXSS_DB_SUCCESS);
    for(int i = 0 ; i < n ; i++)
        {
        resc[0] = (char) i;
        result = cxssInsertUserResc(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserResc(dbCon, "2");
    assert(result == CXSS_DB_NOENT_ERROR);

    /* delete all and confirm all were deleted */
    result = cxssDeleteAllUserResc(dbCon, "1");
    assert(result == CXSS_DB_SUCCESS);

    for(int i = 0 ; i < n ; i++)
        {
        resc[0] = (char) i;
        result = cxssRetrieveUserResc(dbCon, "1", resc, &temp);
        assert(result == CXSS_DB_QUERY_ERROR);
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserResc(dbCon, "1");
    assert(result == CXSS_DB_NOENT_ERROR);

    /** Delete by user **/
    for(int i = 0 ; i < n ; i++)
        {
        resc[0] = (char) i;
        data.CXSS_UserID = (i < n/2)? "1":"2";
        result = cxssInsertUserResc(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    /* delete all from user and confirm */
    result = cxssDeleteAllUserResc(dbCon, "1");
    assert(result == CXSS_DB_SUCCESS);

    for(int i = 0 ; i < n ; i++)
        {
        char* id = (i < n/2)? "1":"2";
        resc[0] = (char) i;
        result = cxssRetrieveUserResc(dbCon, id, resc, &temp);
        assert(result == ((i < n/2)? CXSS_DB_QUERY_ERROR : CXSS_DB_SUCCESS));
        }
    /* test deleting nothing */
    result = cxssDeleteAllUserResc(dbCon, "1");
    assert(result == CXSS_DB_NOENT_ERROR);

    cxssCredentialsDatabaseClose(dbCon);
    return 0;
    }
