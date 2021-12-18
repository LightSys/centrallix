#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxlib/mtask.h"
#include "cxss/cxss.h"
#include "stparse.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2021 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module:	cxss (Centrallix Security Subsystem) Policy  	        */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	October 8, 2015                                         */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/*									*/
/*		The Policy module provides security policy		*/
/*		implementation.						*/
/************************************************************************/


/*** A couple of necessary forward declarations ***/
pCxssPolicy cxss_i_LoadPolicy(pStructInf policy_inf);
int cxss_i_ActivateIncludes(pCxssPolicy policy);


/*** cxss_i_FreePolicy - release memory used by a policy.  This is recursive.
 ***/
int
cxss_i_FreePolicy(pCxssPolicy policy)
    {
    int i;

	/** Clean up subpolicies **/
	for(i=0; i<policy->SubPolicies.nItems; i++)
	    cxss_i_FreePolicy((pCxssPolicy)policy->SubPolicies.Items[i]);

	/** Clean up authentications **/
	for(i=0; i<policy->Authentications.nItems; i++)
	    {
	    pCxssPolAuth auth = (pCxssPolAuth)policy->Authentications.Items[i];
	    if (auth->Criteria)
		expFreeExpression(auth->Criteria);
	    nmFree(auth, sizeof(CxssPolAuth));
	    }

	/** Clean up inclusions **/
	for(i=0; i<policy->Inclusions.nItems; i++)
	    {
	    nmFree(policy->Inclusions.Items[i], sizeof(CxssPolInclude));
	    }

	/** Clean up rules **/
	for(i=0; i<policy->Rules.nItems; i++)
	    {
	    nmFree(policy->Rules.Items[i], sizeof(CxssPolRule));
	    }

	/** Clean up the policy structure **/
	xaDeInit(&policy->SubPolicies);
	xaDeInit(&policy->Authentications);
	xaDeInit(&policy->Inclusions);
	xaDeInit(&policy->Rules);
	nmFree(policy, sizeof(CxssPolicy));

    return 0;
    }


/*** cxss_i_ValidatePolicy - ensure a policy meets requirements before we
 *** integrate it into security authorizations.
 ***
 *** Note that this is not recursive.  The caller's code structure is
 *** responsible for validating all levels of policies.
 ***
 *** policy->Parent *MUST* be set before calling Validate, if the policy is
 *** to be a subpolicy of another policy.  However, the policy should NOT
 *** be added to the parent until after validation passes.
 ***/
int
cxss_i_ValidatePolicy(pCxssPolicy policy, pCxssPolInclude include)
    {
    int len;
    int i;
    pCxssPolInclude subinclude;

	/** Has parent but include not specified? **/
	if ((policy->Parent && !include) || (!policy->Parent && include))
	    {
	    mssError(1, "CXSS", "Internal inconsistency when validating security policy");
	    return -1;
	    }

	/** Domain path - malformed? **/
	len = strlen(policy->DomainPath);
	if (strstr(policy->DomainPath, "//") || strstr(policy->DomainPath, "/./") || 
	    strstr(policy->DomainPath, "/../") || policy->DomainPath[0] != '/' ||
	    (len >= 2 && policy->DomainPath[len-2] == '/' && policy->DomainPath[len-1] == '.') ||
	    (len >= 3 && policy->DomainPath[len-3] == '/' && policy->DomainPath[len-2] == '.' && policy->DomainPath[len-1] == '.'))
	    {
	    mssError(1, "CXSS", "Malformed policy domain path '%s'", policy->DomainPath);
	    return -1;
	    }

	/** Main policy domain and domain path validation **/
	if (!policy->Parent)
	    {
	    if (strcmp(policy->Domain, "system") != 0)
		{
		mssError(1, "CXSS", "Main policy domain must be 'system'");
		return -1;
		}
	    if (strcmp(policy->DomainPath, "/") != 0)
		{
		mssError(1, "CXSS", "Main policy domain path must be '/'");
		return -1;
		}
	    }

	if (policy->Parent)
	    {
	    assert(include != NULL); /* guaranteed to pass, per check at top of function */

	    /** Subpolicy check domain and path **/
	    if (strcmp(policy->Domain, "system") == 0)
		{
		mssError(1, "CXSS", "Subpolicy domain must not be 'system'");
		return -1;
		}
	    if (include->Domain[0] && strcmp(include->Domain, policy->Domain) != 0)
		{
		mssError(1, "CXSS", "Subpolicy domain '%s' must match inclusion requirement '%s'", policy->Domain, include->Domain);
		return -1;
		}
	    for(i=0; i<policy->Inclusions.nItems; i++)
		{
		subinclude = (pCxssPolInclude)policy->Inclusions.Items[i];
		if (include->Domain[0] && strcmp(subinclude->Domain, include->Domain) != 0)
		    {
		    mssError(1, "CXSS", "Subpolicy inclusion domain '%s' must match parent inclusion domain '%s'", subinclude->Domain, include->Domain);
		    return -1;
		    }
		}
	    if (strcmp(policy->DomainPath, "/") == 0)
		{
		mssError(1, "CXSS", "Subpolicy domain path must not be '/'");
		return -1;
		}
	    if (strstr(policy->DomainPath, policy->Parent->DomainPath) != policy->DomainPath)
		{
		mssError(1, "CXSS", "Subpolicy domain path must be within parent policy's domain path");
		return -1;
		}

	    /** Subpolicy vs parent policy permissions and settings **/
	    if (policy->PolicyMode != policy->Parent->PolicyMode && !include->AllowMode)
		{
		mssError(1, "CXSS", "Subpolicy cannot override policy enforcement mode");
		return -1;
		}
	    if (policy->Inclusions.nItems > 0 && !include->AllowInclusion)
		{
		mssError(1, "CXSS", "Subpolicy cannot include other policies");
		return -1;
		}
	    if (policy->Authentications.nItems > 0 && !include->AllowSubjectlist)
		{
		mssError(1, "CXSS", "Subpolicy cannot specify new subjects / authentications");
		return -1;
		}
	    if (policy->Rules.nItems > 0 && !include->AllowRule)
		{
		mssError(1, "CXSS", "Subpolicy cannot specify new rules");
		return -1;
		}
	    }

    return 0;
    }


/*** cxss_i_LoadPolicyGeneric - load in a policy, either from an object or an OS file
 *** descriptor (objRead vs fdRead).
 ***/
pCxssPolicy
cxss_i_LoadPolicyGeneric(void* read_from, int (*read_fn)(), pCxssPolicy parent, pCxssPolInclude incl)
	{
    pStructInf policy_inf = NULL;
    pCxssPolicy policy = NULL;

	policy_inf = stParseMsgGeneric(read_from, read_fn, 0);
	if (!policy_inf)
	    goto error;

	/** Convert the policy from struct inf to the policy data structure **/
	policy = cxss_i_LoadPolicy(policy_inf);
	if (!policy)
	    goto error;
	policy->Parent = parent;
	if (cxss_i_ValidatePolicy(policy, incl) < 0)
	    goto error;

	/** Activate policy inclusions **/
	if (cxss_i_ActivateIncludes(policy) < 0)
	    goto error;

	return policy;

    error:
	if (policy_inf)
	    stFreeInf(policy_inf);
	if (policy)
	    cxss_i_FreePolicy(policy);
	return NULL;
    }


/*** cxss_i_ActivateOneInclude - load one specific path for an inclusion
 ***/
int
cxss_i_ActivateOneInclude(pCxssPolicy policy, pCxssPolInclude incl, pObjSession sess, char* path)
    {
    pObject obj = NULL;
    pCxssPolicy subpolicy = NULL;

	/** Open the path **/
	obj = objOpen(sess, path, OBJ_O_RDONLY, 0600, "system/file");
	if (!obj)
	    goto error;

	/** Load it **/
	subpolicy = cxss_i_LoadPolicyGeneric(obj, objRead, policy, incl);
	if (!subpolicy)
	    goto error;
	xaAddItem(&policy->SubPolicies, (void*)subpolicy);

	objClose(obj);
	return 0;

    error:
	if (obj)
	    objClose(obj);
	return -1;
    }


/*** cxss_i_ActivateIncludes - Go through sec-policy-inclusion objects in
 *** the policy and integrate any sub-policies that match those inclusions
 ***/
int
cxss_i_ActivateIncludes(pCxssPolicy policy)
    {
    int i,j;
    pCxssPolInclude incl;
    pObjSession sess = NULL;
    pXArray pathlist = NULL;
    pObject obj = NULL;
    pObjQuery qy = NULL;
    char* str;

	/** We'll be querying the OSML, so we need a session **/
	sess = objOpenSession("/");
	if (!sess)
	    goto error;

	/** Go through the inclusions **/
	for(i=0; i<policy->Inclusions.nItems; i++)
	    {
	    incl = (pCxssPolInclude)policy->Inclusions.Items[i];
	    if (incl->Path[0])
		{
		/** Wildcard path-based inclusion **/
		pathlist = xaNew(16);
		if (!pathlist)
		    goto error;
		if (objWildcardQuery(pathlist, sess, incl->Path) < 0)
		    goto error;
		for(j=0; j<pathlist->nItems; j++)
		    {
		    if (cxss_i_ActivateOneInclude(policy, incl, sess, pathlist->Items[j]) < 0)
			goto error;
		    nmSysFree(pathlist->Items[j]);
		    }
		xaFree(pathlist);
		pathlist = NULL;
		}
	    else
		{
		/** SQL-based inclusion **/
		qy = objMultiQuery(sess, incl->ProbeSQL, NULL, 0);
		if (!qy)
		    goto error;
		while((obj = objQueryFetch(qy, OBJ_O_RDONLY)) != NULL)
		    {
		    if (objGetAttrValue(obj, "path", DATA_T_STRING, POD(&str)) != 0)
			{
			mssError(1, "CXSS", "Policy inclusion probe_sql results must have 'path' attribute");
			goto error;
			}
		    if (cxss_i_ActivateOneInclude(policy, incl, sess, str) < 0)
			goto error;
		    objClose(obj);
		    obj = NULL;
		    }
		objQueryClose(qy);
		qy = NULL;
		}
	    }

	objCloseSession(sess);
	return 0;

    error:
	if (obj)
	    objClose(obj);
	if (qy)
	    objQueryClose(qy);
	if (sess)
	    objCloseSession(sess);
	if (pathlist)
	    {
	    for(i=0; i<pathlist->nItems; i++)
		{
		if (pathlist->Items[i])
		    nmSysFree(pathlist->Items[i]);
		}
	    xaFree(pathlist);
	    }
	return -1;
    }


/*** cxss_i_LoadAuth - load in subjectlists / authentication methods
 ***/
int
cxss_i_LoadAuth(pCxssPolicy policy, pStructInf policy_inf)
    {
    int i;
    pStructInf auth_inf;
    pCxssPolAuth auth = NULL;
    char* str;

	/** Loop through structure file data **/
	for (i=0; i<policy_inf->nSubInf; i++)
	    {
	    auth_inf = policy_inf->SubInf[i];
	    if ((auth_inf->Flags & ST_F_GROUP) && !strcmp(auth_inf->UsrType, "system/sec-policy-subjectlist"))
		{
		/** Create a subjectlist / authentication entry **/
		auth = (pCxssPolAuth)nmMalloc(sizeof(CxssPolAuth));
		if (!auth)
		    goto error;
		memset(auth, 0, sizeof(CxssPolAuth));
		if (stGetObjAttrValue(auth_inf, "ident_method", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(auth->IdentMethod, str, sizeof(auth->IdentMethod));
		else
		    {
		    mssError(1, "CXSS", "Subjectlist '%s' must supply ident_method", auth_inf->Name);
		    goto error;
		    }
		if (stGetObjAttrValue(auth_inf, "auth_method", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(auth->AuthMethod, str, sizeof(auth->AuthMethod));
		else
		    {
		    mssError(1, "CXSS", "Subjectlist '%s' must supply auth_method", auth_inf->Name);
		    goto error;
		    }
		if (stGetObjAttrValue(auth_inf, "identity", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(auth->Identity, str, sizeof(auth->Identity));
		else
		    {
		    if (!strcmp(auth->IdentMethod, "static"))
			{
			mssError(1, "CXSS", "Static subjectlist '%s' must supply identity", auth_inf->Name);
			goto error;
			}
		    }
		if (stGetObjAttrValue(auth_inf, "use_as_default", DATA_T_STRING, POD(&str)) == 0 && !strcasecmp(str, "yes"))
		    auth->IsDefault = true;
		if (stGetObjAttrValue(auth_inf, "add_endorsement", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(auth->AddEndorsement, str, sizeof(auth->AddEndorsement));
		if (stGetObjAttrValue(auth_inf, "criteria", DATA_T_STRING, POD(&str)) == 0)
		    {
		    auth->Criteria = expCompileExpression(str, NULL, MLX_F_ICASE | MLX_F_FILENAMES, 0);
		    if (!auth->Criteria)
			{
			mssError(0, "CXSS", "Invalid criteria for subjectlist '%s'", auth_inf->Name);
			goto error;
			}
		    }

		/** Add it to the policy **/
		xaAddItem(&policy->Authentications, (void*)auth);
		}
	    }

	return 0;

    error:
	if (auth)
	    nmFree(auth, sizeof(CxssPolAuth));
	return -1;
    }


/*** cxss_i_LoadIncludes - load in policy inclusion rules
 ***/
int
cxss_i_LoadIncludes(pCxssPolicy policy, pStructInf policy_inf)
    {
    int i;
    pStructInf incl_inf;
    pCxssPolInclude incl = NULL;
    char* str;

	/** Loop through structure file data **/
	for (i=0; i<policy_inf->nSubInf; i++)
	    {
	    incl_inf = policy_inf->SubInf[i];
	    if ((incl_inf->Flags & ST_F_GROUP) && !strcmp(incl_inf->UsrType, "system/sec-policy-inclusion"))
		{
		/** Create a policy inclusion **/
		incl = (pCxssPolInclude)nmMalloc(sizeof(CxssPolInclude));
		if (!incl)
		    goto error;
		memset(incl, 0, sizeof(CxssPolInclude));
		if (stGetObjAttrValue(incl_inf, "probe_sql", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(incl->ProbeSQL, str, sizeof(incl->ProbeSQL));
		if (stGetObjAttrValue(incl_inf, "path", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(incl->Path, str, sizeof(incl->Path));
		if (stGetObjAttrValue(incl_inf, "domain", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(incl->Domain, str, sizeof(incl->Domain));
		if (stGetObjAttrValue(incl_inf, "allow_inclusion", DATA_T_STRING, POD(&str)) == 0 && !strcasecmp(str, "yes"))
		    incl->AllowInclusion = true;
		if (stGetObjAttrValue(incl_inf, "allow_subjectlist", DATA_T_STRING, POD(&str)) == 0 && !strcasecmp(str, "yes"))
		    incl->AllowSubjectlist = true;
		if (stGetObjAttrValue(incl_inf, "allow_rule", DATA_T_STRING, POD(&str)) == 0 && !strcasecmp(str, "yes"))
		    incl->AllowRule = true;
		if (stGetObjAttrValue(incl_inf, "allow_mode", DATA_T_STRING, POD(&str)) == 0 && !strcasecmp(str, "yes"))
		    incl->AllowMode = true;

		/** Needs to have either path or probe sql **/
		if ((!incl->ProbeSQL[0] && !incl->Path[0]) || (incl->ProbeSQL[0] && incl->Path[0]))
		    {
		    mssError(1, "CXSS", "Inclusion '%s' must specify either probe_sql or path (but not both)", incl_inf->Name);
		    goto error;
		    }

		/** Add the inclusion to the policy **/
		xaAddItem(&policy->Inclusions, (void*)incl);
		}
	    }
	return 0;

    error:
	if (incl)
	    nmFree(incl, sizeof(CxssPolInclude));
	return -1;
    }


/*** cxss_i_LoadRules - load in policy authorization rules
 ***/
int
cxss_i_LoadRules(pCxssPolicy policy, pStructInf policy_inf)
    {
    int i,j;
    pStructInf rule_inf, subinf;
    pCxssPolRule rule = NULL;
    char* str;

	/** Loop through structure file data **/
	for (i=0; i<policy_inf->nSubInf; i++)
	    {
	    rule_inf = policy_inf->SubInf[i];
	    if ((rule_inf->Flags & ST_F_GROUP) && !strcmp(rule_inf->UsrType, "system/sec-policy-rule"))
		{
		/** Create a rule **/
		rule = (pCxssPolRule)nmMalloc(sizeof(CxssPolRule));
		if (!rule)
		    goto error;
		memset(rule, 0, sizeof(CxssPolRule));
		if (stGetObjAttrValue(rule_inf, "object", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(rule->MatchObject, str, sizeof(rule->MatchObject));
		if (stGetObjAttrValue(rule_inf, "subject", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(rule->MatchSubject, str, sizeof(rule->MatchSubject));
		if (stGetObjAttrValue(rule_inf, "endorsement", DATA_T_STRING, POD(&str)) == 0)
		    strtcpy(rule->MatchEndorsement, str, sizeof(rule->MatchEndorsement));
		if ((subinf = stLookup(rule_inf, "access")) != NULL)
		    {
		    for(j=0; ; j++)
			{
			str = NULL;
			if (stAttrValue(subinf, NULL, &str, j) < 0)
			    break;
			if (str)
			    {
			    if (!strcmp(str, "observe"))
				rule->MatchAccess |= CXSS_ACC_T_OBSERVE;
			    else if (!strcmp(str, "read"))
				rule->MatchAccess |= CXSS_ACC_T_READ;
			    else if (!strcmp(str, "write"))
				rule->MatchAccess |= CXSS_ACC_T_WRITE;
			    else if (!strcmp(str, "create"))
				rule->MatchAccess |= CXSS_ACC_T_CREATE;
			    else if (!strcmp(str, "delete"))
				rule->MatchAccess |= CXSS_ACC_T_DELETE;
			    else if (!strcmp(str, "exec"))
				rule->MatchAccess |= CXSS_ACC_T_EXEC;
			    else if (!strcmp(str, "noexec"))
				rule->MatchAccess |= CXSS_ACC_T_NOEXEC;
			    else if (!strcmp(str, "delegate"))
				rule->MatchAccess |= CXSS_ACC_T_DELEGATE;
			    else if (!strcmp(str, "endorse"))
				rule->MatchAccess |= CXSS_ACC_T_ENDORSE;
			    else
				{
				mssError(1, "CXSS", "Invalid access type '%s' for rule '%s'", str, rule_inf->Name);
				goto error;
				}
			    }
			else
			    {
			    break;
			    }
			}
		    }
		if ((subinf = stLookup(rule_inf, "action")) != NULL)
		    {
		    for(j=0; ; j++)
			{
			str = NULL;
			if (stAttrValue(subinf, NULL, &str, j) < 0)
			    break;
			if (str)
			    {
			    if (!strcmp(str, "allow"))
				rule->Action |= CXSS_ACT_T_ALLOW;
			    else if (!strcmp(str, "deny"))
				rule->Action |= CXSS_ACT_T_DENY;
			    else if (!strcmp(str, "default_allow"))
				rule->Action |= CXSS_ACT_T_DEFALLOW;
			    else if (!strcmp(str, "default_deny"))
				rule->Action |= CXSS_ACT_T_DEFDENY;
			    else if (!strcmp(str, "log"))
				rule->Action |= CXSS_ACT_T_LOG;
			    else
				{
				mssError(1, "CXSS", "Invalid action '%s' for rule '%s'", str, rule_inf->Name);
				goto error;
				}
			    }
			    else {
				break;
			    }
			}
		    }

		/** Add the rule to the policy **/
		xaAddItem(&policy->Rules, (void*)rule);
		}
	    }

	return 0;

    error:
	if (rule)
	    nmFree(rule, sizeof(CxssPolRule));
	return -1;
    }


/*** cxss_i_LoadPolicy - convert a policy object from struct inf form to
 *** the internal policy data structure form.
 ***/
pCxssPolicy
cxss_i_LoadPolicy(pStructInf policy_inf)
    {
    pCxssPolicy policy = NULL;
    char* str;

	/** Allocate the policy and set it up **/
	policy = (pCxssPolicy)nmMalloc(sizeof(CxssPolicy));
	if (!policy)
	    goto error;
	memset(policy, 0, sizeof(CxssPolicy));
	xaInit(&policy->SubPolicies, 16);
	xaInit(&policy->Authentications, 16);
	xaInit(&policy->Inclusions, 16);
	xaInit(&policy->Rules, 16);

	/** Enforcement mode **/
	if (stGetObjAttrValue(policy_inf, "mode", DATA_T_STRING, POD(&str)) == 0)
	    {
	    if (!strcmp(str, "disable"))
		policy->PolicyMode = CXSS_MODE_T_DISABLE;
	    else if (!strcmp(str, "warn"))
		policy->PolicyMode = CXSS_MODE_T_WARN;
	    else if (!strcmp(str, "enforce"))
		policy->PolicyMode = CXSS_MODE_T_ENFORCE;
	    else
		{
		mssError(1, "CXSS", "Invalid security policy mode '%s'", str);
		goto error;
		}
	    }
	else
	    {
	    policy->PolicyMode = CXSS_MODE_T_ENFORCE;
	    }

	/** Domain **/
	if (stGetObjAttrValue(policy_inf, "domain", DATA_T_STRING, POD(&str)) == 0)
	    strtcpy(policy->Domain, str, sizeof(policy->Domain));
	else
	    strcpy(policy->Domain, "system");

	/** Domain Path **/
	if (stGetObjAttrValue(policy_inf, "domain_path", DATA_T_STRING, POD(&str)) == 0)
	    strtcpy(policy->DomainPath, str, sizeof(policy->DomainPath));
	else
	    strcpy(policy->DomainPath, "/");

	/** Subjects/authentications **/
	if (cxss_i_LoadAuth(policy, policy_inf) < 0)
	    goto error;

	/** Inclusion data **/
	if (cxss_i_LoadIncludes(policy, policy_inf) < 0)
	    goto error;

	/** Rule data **/
	if (cxss_i_LoadRules(policy, policy_inf) < 0)
	    goto error;

	return policy;

    error:
	if (policy)
	    cxss_i_FreePolicy(policy);
	return NULL;
    }


/*** cxssLoadPolicy - load in a policy from a given underlying operating system
 *** file (not osml object pathname).  This bootstraps the policy system, and
 *** afterward all updates and subpolicy detections and updates are handled
 *** automatically.
 ***/
int
cxssLoadPolicy(char* os_path)
    {
    pFile policy_fd = NULL;
    pCxssPolicy policy = NULL;

	/** Already loaded? **/
	if (CXSS.Policy)
	    goto error;

	/** Try to load in the policy **/
	policy_fd = fdOpen(os_path, O_RDONLY, 0600);
	if (!policy_fd)
	    {
	    mssErrorErrno(1, "CXSS", "Could not open policy %s", os_path);
	    goto error;
	    }
	if ((policy = cxss_i_LoadPolicyGeneric(policy_fd, fdRead, NULL, NULL)) == NULL)
	    {
	    mssError(0, "CXSS", "Could not load policy %s", os_path);
	    goto error;
	    }
	strtcpy(policy->PolicyPath, os_path, sizeof(policy->PolicyPath));
	fdClose(policy_fd, 0);
	policy_fd = NULL;

	/** Activate the policy **/
	CXSS.Policy = policy;

	return 0;

    error:
	if (policy)
	    cxss_i_FreePolicy(policy);
	if (policy_fd)
	    fdClose(policy_fd, 0);
	return -1;
    }


/*** cxssPolicyInit - load in the config-specified policy
 ***/
int
cxssPolicyInit()
    {
    char* path = NULL;

	/** Lookup the specified path **/
	if (stAttrValue(stLookup(CxGlobals.ParsedConfig, "security_policy_file"), NULL, &path, 0) < 0 || !path)
	    {
	    mssError(1, "CXSS", "security_policy_file not specified in server configuration");
	    return -1;
	    }

	/** Load it **/
	if (cxssLoadPolicy(path) < 0)
	    return -1;

    return 0;
    }


/*** cxssAuthorizeSpec - given an object spec (domain:type:obj:attr), possibly
 *** containing blanks, and a given access type mask, determine whether the
 *** access is allowed.  Returns -1 on error, 0 on not allowed, and 1 on
 *** allowed.  The log_mode parameter is used to suppress logging if we're
 *** testing for access permissions but not actually about to do the access,
 *** or if for whatever reason a failed attempt should not be logged.
 ***/
int
cxssAuthorizeSpec(char* objectspec, int access_type, int log_mode)
    {
    char tmpspec[OBJSYS_MAX_PATH + 256];
    char *domain = "", *type = "", *path = "", *attr = "";
    char *colonptr;

	/** Make a local copy of the spec so we can adjust it **/
	if (strlen(objectspec) >= sizeof(tmpspec))
	    {
	    mssError(1,"CXSS","cxssAuthorizeSpec(): object spec too long.");
	    return -1;
	    }
	strtcpy(tmpspec, objectspec, sizeof(tmpspec));

	/** Break it up into its components. **/
	domain = tmpspec;
	colonptr = strchr(tmpspec, ':');
	if (colonptr)
	    {
	    type = colonptr+1;
	    *colonptr = '\0';
	    colonptr = strchr(type, ':');
	    if (colonptr)
		{
		path = colonptr+1;
		*colonptr = '\0';
		colonptr = strchr(path, ':');
		if (colonptr)
		    {
		    attr = colonptr+1;
		    *colonptr = '\0';
		    }
		}
	    }

    return cxssAuthorize(domain, type, path, attr, access_type, log_mode);
    }


/*** cxssAuthorize - like cxssAuthorizeSpec, but the various parts of the
 *** object spec are provided independently.
 ***/
 // - See SecurityModel_New.txt file for parm descriptions
 // - The rule information is already loaded in the policy data structure
 // (Inclusions / Subpolicies XArrays in policy.h)
 // - Endorsement information in cxss.h cxssHasEndorsement() call to check if current
 // subject has the specified endorsement
 // - Call to get current user (Subject) use mssUserName() in mtsession.h

 // check using usernames and endorsments only, not roles or groups
int
cxssAuthorize(char* domain, char* type, char* path, char* attr,
              int access_type, int log_mode)
    {
    /** Just a stub right now **/

	pCxssPolicy rootPolPtr = CXSS.Policy;

	printf("\n\ncxssAuth CALLED!\n\n");

	printf("Policy Mode:                  %d\n", rootPolPtr->PolicyMode);
	printf("Policy Path:                  %s\n", rootPolPtr->PolicyPath);
	printf("Domain Path:                  %s\n", rootPolPtr->DomainPath);
	printf("Domain:                       %s\n", rootPolPtr->Domain);
	printf("DateTime:                     %lld\n\n", rootPolPtr->ModifyDate.Value);

	// Q. root has no subpolicies?
	// A. 
	printf("Num SubPolicies:              %d\n", rootPolPtr->SubPolicies.nItems);
	printf("Num Inclusion Structs:        %d\n", rootPolPtr->Inclusions.nItems);
	printf("Num Rules:                    %d\n\n", rootPolPtr->Rules.nItems);

	pCxssPolRule rule_1_ptr = (pCxssPolRule) (xaGetItem(&(rootPolPtr->Rules), 0));
	printf("Rule 1 Object to Match:       %s\n", rule_1_ptr->MatchObject );
	printf("Rule 1 Subject to Match:      %s\n", rule_1_ptr->MatchSubject);
	printf("Rule 1 Endorsement to Match:  %s\n", rule_1_ptr->MatchEndorsement);
	printf("Rule 1 Access Type Mask:      %d\n", rule_1_ptr->MatchAccess );
	// Q. i dont see a 0 in the defined ACC types
	// A. 
	printf("Rule 1 Action Type Mask:      %d\n\n", rule_1_ptr->Action);

	pCxssPolInclude inclSpec_1_ptr = (pCxssPolInclude) (xaGetItem(&(rootPolPtr->Inclusions), 0));
	printf("Incl Spec ProbeSQL:           %s\n", inclSpec_1_ptr->ProbeSQL);
	printf("Incl Spec Path:               %s\n", inclSpec_1_ptr->Path);
	printf("Incl Spec Domain:             %s\n", inclSpec_1_ptr->Domain);
	printf("Incl Spec AllowInclusion:     %s\n", inclSpec_1_ptr->AllowInclusion ? "true" : "false");
	printf("Incl Spec AllowSubjectlist:   %s\n", inclSpec_1_ptr->AllowSubjectlist ? "true" : "false");
	printf("Incl Spec AllowRule:          %s\n", inclSpec_1_ptr->AllowRule ? "true" : "false");
	printf("Incl Spec AllowMode:          %s\n\n", inclSpec_1_ptr->AllowMode ? "true" : "false");

	pCxssPolRule rule_2_ptr = (pCxssPolRule) (xaGetItem(&(rootPolPtr->Rules), 1));
	printf("Rule 2 Object to Match:       %s\n", rule_2_ptr->MatchObject );
	printf("Rule 2 Subject to Match:      %s\n", rule_2_ptr->MatchSubject);
	printf("Rule 2 Endorsement to Match:  %s\n", rule_2_ptr->MatchEndorsement);
	printf("Rule 2 Access Type Mask:      %d\n", rule_2_ptr->MatchAccess );
	printf("Rule 2 Action Type Mask:      %d\n\n", rule_2_ptr->Action);

	// Q. why does this not break? there should only be 2 rules
	// A. other spaces in array are empty Rule structs
	pCxssPolRule rule_3_ptr = (pCxssPolRule) (xaGetItem(&(rootPolPtr->Rules), 2));
	printf("Rule 3 Object to Match:       %s\n\n", rule_3_ptr->MatchObject);
	

	printf("\n\n");
    return 1;
    }
