#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "include/cxss/cxss.h"
//#include "include/cxss/policy.h"
long long
test(char** name)
    { 
    *name = "policy_04 access type match rule test";
    char* domain = "";
    char* type = ""; 
    char* path = ""; 
    char* attr = "";
    CxssPolRule ruleAll = {.MatchAccess = 0};

    /**check all possible access types agains all possible access rules**/
    int i;
    
    for(i = 0 ; i < 512 ; i++){
        int j;
        for(j = 0 ; j < 512 ; j++){
            CxssPolRule curRule = {.MatchAccess = j};
            /** if the rule.MatchAccess == 0, it matches all accesses **/
            if(i == 0){ //access type 0 is illegal
                assert(cxssIsRuleMatch(domain, type, path, attr, i, &curRule) == CXSS_MATCH_T_ERR);
            } else if(i == j || j == 0){
                assert(cxssIsRuleMatch(domain, type, path, attr, i, &curRule) == CXSS_MATCH_T_TRUE);
            } else {
                assert(cxssIsRuleMatch(domain, type, path, attr, i, &curRule) == CXSS_MATCH_T_FALSE);
            }
        }
    }
    return 0;
}
