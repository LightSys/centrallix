#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "include/cxss/cxss.h"
//#include "include/cxss/policy.h"
long long
test(char** name)
    { 
    *name = "policy_03 match object/attribute identifiers";

    //set all of the input values to a constant string
    char* domain = "aa";
    char* type =   "bb"; 
    char* path =   "cc"; 
    char* attr =   "dd";
    int access_type = 1;

    //build every possible rule for match, blank, and no match
    char* aVals[3] = {"aa", "", "not_aa"};
    char* bVals[3] = {"bb", "", "not_bb"};
    char* cVals[3] = {"cc", "", "not_cc"};
    char* dVals[3] = {"dd", "", "not_dd"};

    /** check every possible set of matching, not matching, or blank **/
    int a;
    for(a = 0 ; a < 3 ; a++){
        int b;
        for(b = 0 ; b < 3 ; b++){
            int c;
            for(c = 0 ; c < 3 ; c++){
                int d;
                for(d = 0 ; d < 3 ; d++){
                    char total[OBJSYS_MAX_PATH + 1] = "";
                    strcat(total, aVals[a]);
                    strcat(total, ":");
                    strcat(total, bVals[b]);
                    strcat(total, ":");
                    strcat(total, cVals[c]);
                    strcat(total, ":");
                    strcat(total, dVals[d]); 

                    CxssPolRule rule = {.MatchAccess = 0};
                    strcpy(rule.MatchObject, total);

                    /** if all are correct or blank (any) then will match **/
                    if(a <= 1 && b <= 1 && c <= 1 && d <= 1){
                        assert(cxssIsRuleMatch(domain, type, path, attr, access_type, &rule) == CXSS_MATCH_T_TRUE);
                    }else {
                        assert(cxssIsRuleMatch(domain, type, path, attr, access_type, &rule) == CXSS_MATCH_T_FALSE);
                    } 
                }
            }
        }
    }

    return 0;
}
