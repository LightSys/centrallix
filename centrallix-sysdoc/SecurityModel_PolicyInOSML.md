### **Title:**	Policy in OSML  
### **Author:**	Noah Board  
### **Date:**	06-Jun-2022  
  
---

# Overview
This document covers how security policies are enforced in the OSML. More specifically, this relates primarily to the `cxssAuthorize` function found in centrallix/cxss/cxss_policy.c. However, the function is intended to be used in functions which interact with the OSML, including reads, writes, etc. Terms used in this document are largely defined in centrallix-sysdoc/SecurityModel_New.txt. Currently, the policy system has several limitations, so please keep these in mind when attempting to use or refine the code. 

# cxssAuthorize 
Found in centrallix/cxss/cxss_policy  
## Parameters
- **domain:** the affected object’s domain, as found in the object spec
- **type:** the type, as found in the object’s spec
- **path:** the object’s path as found in the object spec
- **attr:** the object’s attribute, as found in the object spec
- **access_type:** the access type request by the user (e.g. read, write, observe, etc.)
- **log_mode:** how the logging system should be handled. This can be disabled for testing, or used to ignore failed attempts, and other similar actions. Note that this is currently **not** used by `cxssAuthorize`.  
## Overview
The function essentially loops through each policy until a relevant policy is found, and then checks each rule for a match.  
In order to allow actions to be performed while the system is setting up and the main policy has not been loaded, if the main policy is NULL, which should only occur during setup, and the caller has the system:seckernel endorsement, any action will be allowed. Without this endorsement, all actions are denied when the main policy is NULL. 
If the policy mode is set to disable, all actions will be allowed, provided that the main policy was loaded.   
If none of the allow or deny conditions were met by the above descriptions, then iterate through rules found in the main and sub policies. The search is conducted in a breadth first manner using a queue of policies. To start, the main policy is placed on the queue. The code then repeatedly removes the policy from the front of the queue. If the policy is applicable, as determined by the domain of the policy, each rule is checked in the policy using `cxssIsRuleMatch`. The first rule to match has the corresponding action taken, and the function exits. If policy is not applicable or no applicable rules were found in the policy, each sub-policy is added to the back of the queue. The loops exits if a rule is matched or no more policies are found.   
The function then performs some basic cleanup. If no match was found, the default action is set. If the policy mode is set to warn, the function will return allow. Note that currently no warnings are generated.   
## Limitations
- The function currently ignores the log mode
- Due to limitations with the policy structure used by Centrallix, which does not include default actions, default actions are assumed to always be deny, regardless of how the actual policy file is set. 
- Due to limitations with rule structures, rule exceptions are inaccessible and therefore ignored. 
- The security system only determines identity based on usernames and endorsements, while roles and groups are both ignored. 
- No warnings are generated when the policy mode is set to warn; the function silently allows all actions after performing checks against the security. 

# cxssIsRuleMatch
## Parameters
- **domain:** same as in `cxssAuthorize`
- **type:** same
- **path:** same
- **attr:** same
- **access_type:** same
- **rule:** the rule that needs to be checked against the object and current context
## Overview  
The function starts with the assumption that the rule is a match and checks various parameters to see if the assumption is true.   
First, the rule’s object spec is parsed into the `appName` (domain), `objType` (type), `objName` (path), and `attrName` (attribute). Then the function begins checking against the parameters passed to `cxssAuthorize`. Blanks in the rule are considered wildcards. The object path is interesting in that `objName` is converted in regex which is used to check against the object’s path in order to facilitate quick and efficient checks. The function `mssUserName` is used to retrieve the current user. `cxssHasEndorsement` is used to check if the user has a specific endorsement or not.   
Errors will be generated if the object’s spec exceeding a max size or if the user attempts to perform an action with an access type of 0.  
## Limitations
- the function could probably be sped up by using nested if’s; only one check needs to fail to return false, but every check will be performed no matter what. If nested if’s are used, consider placing the fastest or most likely to fail checks first, with slower and less likely to succeed checks placed toward the end. 
- The subject is checked solely by comparing against the username. 
- Endorsement checks are only performed with the full domain.
- Attributes assume there is only a single attribute. This may or may not be a bad assumption. 

# Testing
