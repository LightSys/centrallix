#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 00: Basic Init";

    
    /** Basic test of init and release */
    char * PATH = "/home/devel/test.db";
    CXSS_DB_Context_t dbCon = cxssCredentialsDatabaseInit(PATH);

    assert(dbCon != NULL);

    cxssCredentialsDatabaseClose(dbCon);

    return 0;
    }
