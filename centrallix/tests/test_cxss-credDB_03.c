#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 03: resource table basic tests ";

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

    /** generate valid inputs for key, salt, iv, and their lengths **/
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


    /** insert one entry **/
    CXSS_UserResc data;
    data.CXSS_UserID = "1";
    data.ResourceID = "1";
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

    result = cxssInsertUserResc(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    
    /*** Retrieve ***/

    /** retrieve: **/
    CXSS_UserResc retData;
    result = cxssRetrieveUserResc(dbCon, data.CXSS_UserID, data.ResourceID, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.ResourceID, retData.ResourceID) == 0);    
    assert(memcmp(data.AESKey, retData.AESKey, 32) == 0);  
    assert(memcmp(data.UsernameIV, retData.UsernameIV, 16) == 0);
    assert(memcmp(data.AuthDataIV, retData.AuthDataIV, 16) == 0);    
    assert(strcmp(data.ResourceUsername, retData.ResourceUsername) == 0);  
    assert(strcmp(data.ResourceAuthData, retData.ResourceAuthData) == 0);  
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);    
    assert(data.AESKeyLength == retData.AESKeyLength);  
    assert(data.UsernameIVLength == retData.UsernameIVLength);  
    assert(data.AuthDataIVLength == retData.AuthDataIVLength);
    assert(data.UsernameLength == retData.UsernameLength);  
    assert(data.AuthDataLength == retData.AuthDataLength);  

    /** retrive item not in DB **/
    CXSS_UserResc noData;
    result = cxssRetrieveUserResc(dbCon, "2", "1", &noData);
    assert(result == CXSS_DB_QUERY_ERROR);


    /*** Update ***/

    /** update one field **/
    data.DateLastUpdated = "2/2/22"; 
    result = cxssUpdateUserResc(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserResc(dbCon, data.CXSS_UserID, data.ResourceID, &retData);
    assert(result == CXSS_DB_SUCCESS);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.ResourceID, retData.ResourceID) == 0);    
    assert(memcmp(data.AESKey, retData.AESKey, 32) == 0);  
    assert(memcmp(data.UsernameIV, retData.UsernameIV, 16) == 0);
    assert(memcmp(data.AuthDataIV, retData.AuthDataIV, 16) == 0);    
    assert(strcmp(data.ResourceUsername, retData.ResourceUsername) == 0);  
    assert(strcmp(data.ResourceAuthData, retData.ResourceAuthData) == 0);  
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);    
    assert(data.AESKeyLength == retData.AESKeyLength);  
    assert(data.UsernameIVLength == retData.UsernameIVLength);  
    assert(data.AuthDataIVLength == retData.AuthDataIVLength);
    assert(data.UsernameLength == retData.UsernameLength);  
    assert(data.AuthDataLength == retData.AuthDataLength);  

    /** update all fields **/
    data.CXSS_UserID = "1";
    data.ResourceID = "1";
    key[0] = -key[0]; /* just chaninging one piece is enough */
    uNameIV[0] = -uNameIV[0];
    aDataIV[0] = -aDataIV[0];
    data.ResourceUsername = "another name";
    data.ResourceAuthData = "more, different, auth data";
    data.DateCreated = "02/02/2020";
    data.DateLastUpdated = "01/02/2010";
    data.AESKeyLength = 32;
    data.UsernameIVLength = 16;
    data.AuthDataIVLength = 16;
    data.UsernameLength = strlen(data.ResourceUsername);
    data.AuthDataLength = strlen(data.ResourceAuthData);

    result = cxssUpdateUserResc(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserResc(dbCon, data.CXSS_UserID, data.ResourceID, &retData);
    assert(result == CXSS_DB_SUCCESS);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.ResourceID, retData.ResourceID) == 0);    
    assert(memcmp(data.AESKey, retData.AESKey, 32) == 0);  
    assert(memcmp(data.UsernameIV, retData.UsernameIV, 16) == 0);
    assert(memcmp(data.AuthDataIV, retData.AuthDataIV, 16) == 0);    
    assert(strcmp(data.ResourceUsername, retData.ResourceUsername) == 0);  
    assert(strcmp(data.ResourceAuthData, retData.ResourceAuthData) == 0);  
    assert(strcmp(data.DateCreated, retData.DateCreated) != 0); /* cannot update date created class */ 
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);    
    assert(data.AESKeyLength == retData.AESKeyLength);  
    assert(data.UsernameIVLength == retData.UsernameIVLength);  
    assert(data.AuthDataIVLength == retData.AuthDataIVLength);
    assert(data.UsernameLength == retData.UsernameLength);  
    assert(data.AuthDataLength == retData.AuthDataLength);  

    /** update item not in db **/
    data.CXSS_UserID = "2";
    data.DateLastUpdated = "notADate";
    result = cxssUpdateUserResc(dbCon, &data);
    assert(result == CXSS_DB_NOENT_ERROR);

    result = cxssRetrieveUserResc(dbCon, data.CXSS_UserID, data.ResourceID, &noData);
    assert(result == CXSS_DB_QUERY_ERROR); /* Should not have created anything new */

    result = cxssRetrieveUserResc(dbCon, "1", data.ResourceID, &retData);
    assert(result == CXSS_DB_SUCCESS);
    
    /* these should not match */
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) != 0); 
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) != 0);     
    data.CXSS_UserID = "1";

    /** Delete **/

    /** delete entry **/ 
    result = cxssDeleteUserResc(dbCon,  data.CXSS_UserID,  data.ResourceID);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserResc(dbCon,  data.CXSS_UserID, data.ResourceID, &noData); /* attempt to retrieve */
    assert(result == CXSS_DB_QUERY_ERROR);

    /** attempt to delete item not there **/
    result = cxssDeleteUserResc(dbCon, data.CXSS_UserID, data.ResourceID);
    assert(result == CXSS_DB_NOENT_ERROR); 


    cxssCredentialsDatabaseClose(dbCon);
    return 0;
    }
