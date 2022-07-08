#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 05: Retrieve All Auth";

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

    char class[16], create[16], update[16], test[16];
    CXSS_UserAuth data, temp;
    data.PK_UserAuth = 1;
    data.CXSS_UserID = "1";
    data.AuthClass = class;
    data.PrivateKey = privKey;
    data.PrivateKeyIV = ivBuf;
    data.DateCreated = create;
    data.DateLastUpdated = update;
    data.RemovalFlag = 0;
    data.KeyLength = strlen(data.PrivateKey);
    data.IVLength = 16;

    for(int i = 0 ; i < n ; i++)
        {
        /* provide unique, repeatable values */
        data.PK_UserAuth = i;
        sprintf(class, "something%d",i);
        sprintf(create, "1/%d/2022",i);
        sprintf(update, "1/2/20%d",i);

        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    CXSS_UserAuth_LLNode *head, *cur;
    result = cxssRetrieveUserAuthLL(dbCon, "1", NULL, &head);
    assert(result == CXSS_DB_SUCCESS);
    cur = head->next; /* get past sentinel */

    /* iterate through nodes and check results */
    int count = 0;
    while(cur != NULL)
        {
        /* generate old values again */
        sprintf(class, "something%d", count);
        sprintf(create, "1/%d/2022", count);
        sprintf(update, "1/2/20%d", count);

        assert(count == cur->UserAuth.PK_UserAuth);
        assert(strcmp(data.CXSS_UserID, cur->UserAuth.CXSS_UserID) == 0);
        assert(strcmp(class, cur->UserAuth.AuthClass) == 0);
        assert(memcmp(data.PrivateKey, cur->UserAuth.PrivateKey, data.KeyLength) == 0);
        assert(memcmp(data.PrivateKeyIV, cur->UserAuth.PrivateKeyIV, data.IVLength) == 0);
        assert(strcmp(create, cur->UserAuth.DateCreated) == 0);
        assert(strcmp(update, cur->UserAuth.DateLastUpdated) == 0);
        assert(data.RemovalFlag == cur->UserAuth.RemovalFlag);
        assert(data.KeyLength == cur->UserAuth.KeyLength);
        assert(data.IVLength == cur->UserAuth.IVLength);
        count++;
        cur = cur->next;
        }
    assert(count == n);

    /** test with user id **/
    data.CXSS_UserID = "2";
    for(int i = n ; i < 2*n ; i++)
        {
        data.PK_UserAuth = i;
        sprintf(class, "something%d", i);
        sprintf(create, "1/%d/2022", i);
        sprintf(update, "1/2/20%d", i);

        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    result = cxssRetrieveUserAuthLL(dbCon, "2", NULL, &head);
    assert(result == CXSS_DB_SUCCESS);
    cur = head->next; /* get past sentinel */

    /* iterate through nodes and check results */
    count = n;
    while(cur != NULL)
        {
        sprintf(class, "something%d", count);
        sprintf(create, "1/%d/2022", count);
        sprintf(update, "1/2/20%d", count);

        assert(count == cur->UserAuth.PK_UserAuth);
        assert(strcmp(data.CXSS_UserID, cur->UserAuth.CXSS_UserID) == 0);
        assert(strcmp(class, cur->UserAuth.AuthClass) == 0);
        assert(memcmp(data.PrivateKey, cur->UserAuth.PrivateKey, data.KeyLength) == 0);
        assert(memcmp(data.PrivateKeyIV, cur->UserAuth.PrivateKeyIV, data.IVLength) == 0);
        assert(strcmp(create, cur->UserAuth.DateCreated) == 0);
        assert(strcmp(update, cur->UserAuth.DateLastUpdated) == 0);
        assert(data.RemovalFlag == cur->UserAuth.RemovalFlag);
        assert(data.KeyLength == cur->UserAuth.KeyLength);
        assert(data.IVLength == cur->UserAuth.IVLength);
        count++;
        cur = cur->next;
        }
    assert(count == 2*n);

    /** test with user id and class **/
    data.CXSS_UserID = "2";
    data.AuthClass = "something else";
    for(int i = 2*n ; i < 3*n ; i++)
        {
        data.PK_UserAuth = i;
        sprintf(create, "1/%d/2022", i);
        sprintf(update, "1/2/20%d", i);

        result = cxssInsertUserAuth(dbCon, &data);
        assert(result == CXSS_DB_SUCCESS);
        }

    result = cxssRetrieveUserAuthLL(dbCon, "2", "something else", &head);
    assert(result == CXSS_DB_SUCCESS);
    cur = head->next; /* get past sentinel */

    /* iterate through nodes and check results */
    count = 2*n;
    while(cur != NULL)
        {
        sprintf(create, "1/%d/2022", count);
        sprintf(update, "1/2/20%d", count);

        assert(count == cur->UserAuth.PK_UserAuth);
        assert(strcmp(data.CXSS_UserID, cur->UserAuth.CXSS_UserID) == 0);
        assert(strcmp(data.AuthClass, cur->UserAuth.AuthClass) == 0);
        assert(memcmp(data.PrivateKey, cur->UserAuth.PrivateKey, data.KeyLength) == 0);
        assert(memcmp(data.PrivateKeyIV, cur->UserAuth.PrivateKeyIV, data.IVLength) == 0);
        assert(strcmp(create, cur->UserAuth.DateCreated) == 0);
        assert(strcmp(update, cur->UserAuth.DateLastUpdated) == 0);
        assert(data.RemovalFlag == cur->UserAuth.RemovalFlag);
        assert(data.KeyLength == cur->UserAuth.KeyLength);
        assert(data.IVLength == cur->UserAuth.IVLength);
        count++;
        cur = cur->next;
        }
    assert(count == 3*n);

    cxssCredentialsDatabaseClose(dbCon);
    return 0;
    }
