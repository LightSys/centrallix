#include <assert.h>
#include <stdio.h>
#include "cxss/credentials_db.h"

long long
test(char** name)
    {
     *name = "CXSS Cred DB 01: Test user table";

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
    CXSS_UserData data;
    data.CXSS_UserID = "1";
    data.PublicKey = "not a real public key";
    data.DateCreated = "7/5/2022";
    data.DateLastUpdated = "7/4/2022";
    data.KeyLength = strlen(data.PublicKey);
    int result = cxssInsertUserData(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);


    /*** Retrieve ***/

    /** retrieve: **/
    CXSS_UserData retData;
    result = cxssRetrieveUserData(dbCon, data.CXSS_UserID, &retData);
    assert(result == CXSS_DB_SUCCESS);

    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.PublicKey, retData.PublicKey) == 0);    
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);  
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.KeyLength == retData.KeyLength);  

    /** retrive item not in DB **/
    CXSS_UserData noData;
    result = cxssRetrieveUserData(dbCon, "2", &noData);
    assert(result == CXSS_DB_QUERY_ERROR);


    /*** Update ***/

    /** update one field **/
    data.PublicKey = "yet another fake key."; 
    result = cxssUpdateUserData(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserData(dbCon, data.CXSS_UserID, &retData);
    assert(result == CXSS_DB_SUCCESS);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0); /* only this field changed */
    assert(strcmp(data.PublicKey, retData.PublicKey) == 0);    
    assert(strcmp(data.DateCreated, retData.DateCreated) == 0);  
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.KeyLength == retData.KeyLength);  

    /** update all fields **/
    data.PublicKey =    "-----BEGIN RSA PUBLIC KEY-----\n\
                MIICCgKCAgEAw4/Eqoi/sBB/O+gFne66/UAnico6oVncS1kn42iQ4aV/2zHha0ML\n\
                EozmpD97v6TPumdqQNdudkL2FATHouaegGLuz2Mj+njVcNLtaNcXmDsgw7Nzn8HV\n\
                k+XiPk3T2tq/LuqdrYjC9tgl0RAWsqIpx3k6MNhNGR63n5WpvTLPLF1m3upGWcx8\n\
                jUflMuy3fmw+Jd0nr0gbrENYDcqy9AayouCWtC7MVXSghISXYRqNQ6M1tSACMGjY\n\
                f1lsBQ/gieI7GZseitI5DP4uHzDWgPu8+nqIbLS/Ugf90H5cf16IB1GUIA77TjO4\n\
                J/c51xi9uDlCofPX2BkRxrehSt+nBUxKMR5qdbDA3+SsUUJfk4RLi4rdJOauBvNL\n\
                S0N4P+K4H45yLMe4mtFeBNxiaGgKmv2R6YTRrQmGVrqyve/9Bg99xM7xcVVCjCME\n\
                SbNMdr/bc0Pc0yJTdyY04cH0SbLs46/sQ4j22zc6bggdzKZQ9HchdJoSx+VbOf8G\n\
                ZlhzqJ9JEsH/+MRi9iGikMtCcZY4tFVC7iHVhJzCQ5lRB0MECtGVEPEcx2Oldfh9\n\
                3L3wuhMx+mNoeMJYXjdK9/qQSgcqGsJZEgJHG3uMfEKrHyJaNHfnlDCEsgUpHD6l\n\
                ITs60TTQbDAdLipygnF3MRPk0WMlx74HxbKRbJDzZc2v936KiFxqVh0CAwEAAQ==\n\
                -----END RSA PUBLIC KEY-----";
    data.DateCreated = "1/2/2034";
    data.DateLastUpdated = "4/3/2010";
    data.KeyLength =  strlen(data.PublicKey);

    result = cxssUpdateUserData(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserData(dbCon, data.CXSS_UserID, &retData);
    assert(result == CXSS_DB_SUCCESS);
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) == 0);
    assert(strcmp(data.PublicKey, retData.PublicKey) == 0);    
    assert(strcmp(data.DateCreated, retData.DateCreated) != 0); /* the date created cannot be updted. */
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) == 0);  
    assert(data.KeyLength == retData.KeyLength);  

    /** update item not in db **/
    data.CXSS_UserID = "2";
    data.PublicKey =    "back to fake";
    data.DateCreated = "not a date";
    data.DateLastUpdated = "this isn't either";
    data.KeyLength =  strlen(data.PublicKey);
    result = cxssUpdateUserData(dbCon, &data);
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserData(dbCon, data.CXSS_UserID, &noData);
    assert(result == CXSS_DB_QUERY_ERROR); /* Should not have created anything new*/

    result = cxssRetrieveUserData(dbCon, "1", &retData);
    assert(result == CXSS_DB_SUCCESS);
    /* these should not match */
    assert(strcmp(data.CXSS_UserID, retData.CXSS_UserID) != 0); 
    assert(strcmp(data.PublicKey, retData.PublicKey) != 0);    
    assert(strcmp(data.DateCreated, retData.DateCreated) != 0); /* the date created cannot be updated. */
    assert(strcmp(data.DateLastUpdated, retData.DateLastUpdated) != 0);  
    assert(data.KeyLength != retData.KeyLength);  
    

    /** Delete **/

    /** delete entry **/ 
    result = cxssDeleteUserData(dbCon, "1");
    assert(result == CXSS_DB_SUCCESS);

    result = cxssRetrieveUserData(dbCon, "1", &noData); /* attempt to retrieve */
    assert(result == CXSS_DB_QUERY_ERROR);

    /** attempt to delete item not there **/
    result = cxssDeleteUserData(dbCon, "2");
    assert(result == CXSS_DB_SUCCESS); /* deleting always works; does not check for change */


    cxssCredentialsDatabaseClose(dbCon);

    return 0;
    }
