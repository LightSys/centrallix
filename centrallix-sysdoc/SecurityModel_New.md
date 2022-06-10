# Security subsystem

Author:	Greg Beeley (GRB)

Date:	13-Nov-2012, updated 28-Sep-2015

## Overview
This document describes the Centrallix Security Subsystem (CXSS), a role- based policy-driven security model.

## Table of Contents
- [Security subsystem](#security-subsystem)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [Terminology](#terminology)
  - [Application Properties](#application-properties)
    - [A.  add_endorsements = string[,string ...]](#a--add_endorsements--stringstring-)
    - [A.  require_endorsements = string[,string ...]](#a--require_endorsements--stringstring-)
  - [Subject Identifiers](#subject-identifiers)
  - [Object/Attribute Identifiers](#objectattribute-identifiers)
  - [Endorsement Identifiers](#endorsement-identifiers)
    - [system:from_application](#systemfrom_application)
  - [Access Types](#access-types)
  - [Security Type Declarations](#security-type-declarations)
  - [Application-Specific Security Configuration](#application-specific-security-configuration)
  - [Security Policy File](#security-policy-file)
    - [A.	Global Policy Settings "system/sec-policy"](#aglobal-policy-settings-systemsec-policy)
      - [mode = {disable|warn|enforce}](#mode--disablewarnenforce)
      - [default = {allow|deny|none}](#default--allowdenynone)
      - [domain = "string"](#domain--string)
      - [domain_path = "string"](#domain_path--string)
    - [B.	Subject Definitions "system/sec-policy-subject"](#bsubject-definitions-systemsec-policy-subject)
      - [authentication_method = {static|unix|altpasswd|...}](#authentication_method--staticunixaltpasswd)
      - [required_authentications = Number](#required_authentications--number)
      - [identity = "string"](#identity--string)
      - [password = "string"](#password--string)
      - [key = "string"](#key--string)
      - [use_as_default = {no|yes}](#use_as_default--noyes)
      - [criteria = {expression}](#criteria--expression)
      - [add_endorsement = "string"](#add_endorsement--string)
  - [Signing Key Configuration](#signing-key-configuration)
  - [Authentication Mapping and Token Storage](#authentication-mapping-and-token-storage)
    - [A.	User Token (password)](#auser-token-password)
    - [B.	User Salt](#buser-salt)
    - [C.	User Key](#cuser-key)
    - [D.	User Public Key and Private Key](#duser-public-key-and-private-key)
    - [E.	Resource Data](#eresource-data)
  - [OSML Hooks (Coverage Enforcement)](#osml-hooks-coverage-enforcement)
    - [objOpen:](#objopen)
    - [obj_internal_ProcessOpen:](#obj_internal_processopen)
    - [driver xxxOpen():](#driver-xxxopen)
    - [objClose:](#objclose)
    - [objCreate:](#objcreate)
    - [objDelete, objDeleteObj:](#objdelete-objdeleteobj)
    - [objInfo, objPresentationHints, objRead, objGetAttrType, objGetAttrValue, objGetFirstAttr, objGetNextAttr, objGetFirstMethod, objGetNextMethod, objOpenAttr:](#objinfo-objpresentationhints-objread-objgetattrtype-objgetattrvalue-objgetfirstattr-objgetnextattr-objgetfirstmethod-objgetnextmethod-objopenattr)
    - [objSetAttrValue, objExecuteMethod, objWrite, objAddAttr, objAddVirtualAttr:](#objsetattrvalue-objexecutemethod-objwrite-objaddattr-objaddvirtualattr)
    - [objOpenQuery, objQueryClose:](#objopenquery-objqueryclose)
    - [objQueryFetch:](#objqueryfetch)
    - [objQueryDelete:](#objquerydelete)
    - [objQueryCreate:](#objquerycreate)
    - [objMultiQuery, objGetQueryCoverageMask, objGetQueryIdentityPath:](#objmultiquery-objgetquerycoveragemask-objgetqueryidentitypath)
  - [Code Signing](#code-signing)
    - [A.	Code Signing Format](#acode-signing-format)
    - [B.	Code Signing Utility](#bcode-signing-utility)
    - [C.	Verification of Authorship](#cverification-of-authorship)
    - [D.	Digital Signature Verification](#ddigital-signature-verification)
  - [Security Subsystem Tasks and Phasing](#security-subsystem-tasks-and-phasing)
    - [Phase One](#phase-one)
    - [Phase Two](#phase-two)
    - [Phase Three](#phase-three)

## Terminology
Subject: the user that is engaging in a certain type of access.  This user may be represented by their user name, or by their role or user group.

Object: the item in the Centrallix system that is being accessed, such as a file, database table row, structure file subgroup, etc.  Objects are identified primarily by their pathname, but also by the application context to which they belong and by their object type.

Access Type: this is a very fine-grained privilege that a Subject may use when accessing an Object.  Access types include read, write, observe, delete, create, exec, and noexec, as well as a couple of internal access types (delegate, endorse) related to policy delegation and endorsement granting.

Endorsement: this is a very high-level privilege that a Subject may have, and which may be used by the security policy to determine what access types the Subject may engage in with regard to different Objects.

Policy: the policy is a collection of policy files which work together to determine what types of Access Types a given Subject may engage in with regard to a given Object.  A Policy contains a set of Rules, as well as a set of definitions of object types, endorsement types, application (domain) types, and roles.  Multiple policy files are required because installable applications within Centrallix will normally have special knowledge of roles, permissions, and such, that should be honored.

Rule: a single item within a policy which specifies whether an access should be allowed or denied in a specific situation.  Policies contain a set of rules which provide access control coverage over a potentially complex system.  Rules can match certain Subjects, certain Objects, require certain Endorsements, relate to certain Access Types, require certain Object authorship (and with a certain level of assurance), and may take one or more Actions with regard to the request for access to the Object.  The rule may also match against the Object's attributes, or in the case of modification, the attributes before and/or after a proposed change.

Action: the result of a rule matching a given access to an object.  Actions may include allowing access, denying access, or adding a new Endorsement to the user's session in the context of the current operation.

Delegation: a policy file may delegate control to another policy file for a specific subset of Objects in the system.  In such a situation, the main policy retains veto ability in that it can still deny access, but the sub policy file has greater knowledge of its domain (application context) and is in a better position to mediate access to that application's objects.

Domain: a subtree of the Centrallix ObjectSystem which is subject to the control of a particular policy file, as delegated by other policy files.  Typically, this represents an "application", or a "module" within an application.

Group: a set of users specified by the administrator of the system.

Role: a "sense" that a user can engage in which provides access to a specific subset of functionality.  Roles may be set up to be mutually exclusive in order to implement certain separation of duties types of controls.

## Application Properties
Applications and components will gain the following properties in order to manage user endorsements:

### A.  add_endorsements = string[,string ...]

This property allows an application or component to add an endorsement to the context that the user is operating within.  If specified with a subquery-style SQL statement, multiple endorsements can be added from a database backend, for instance.

All widgets can have the following properties:

### A.  require_endorsements = string[,string ...]

This specifies a list of one or more endorsements that a widget requires in order to render property.

## Subject Identifiers
Subject identifiers must distinguish between a specific user, a group that a user is in, or a role that a user is acting in.

There are four types of subject identifiers that don't identify a specific subject.  These are a:, l:, c:, and e:, and are described below.

| Identifier   | Description
| ------------ | ------------
| u:username   | A user with logon 'username'
| g:grpname    | A group with group name 'grpname'
| r:rolename   | A role with role name 'rolename'
| a:           | A user who has not authenticated ('anonymous')
| l:           | A logged-in/authenticated user
| c:           | The creator/author of a given object
| e:           | Everyone

The c: subject identifier is context-dependent.  It is used to refer to the creator of a given object, and so is dependent on which object is being accessed.

## Object/Attribute Identifiers
The object identifier will have the capability of referring to an object type, specific object, attribute for a type of object, or attribute within a specific object.  It may also refer to all objects within a given application.

The general object ID specification is formatted thus:

`appname:objecttype:objectname:attrname`

In specific instances, the following are also accepted.  An unspecified name or type is a wildcard.

A.	All objects belonging to a specific application:

`appname:::`

B.	All attributes of a given name within an application:

`appname:::attrname`

C.	All objects of a specific type in an application:

`appname:objecttype::`

D.	A specific object within an application:

```
appname:objecttype:objectname:
appname:objecttype:/path/to/objectname:
appname:objecttype:/objects/within/*:
```

E.	An attribute of a type of object in an app:

`appname:objecttype::attrname`

F.	An attribute of a specific object:

`appname:objecttype:objectname:attrname`

## Endorsement Identifiers
Likewise, endorsements also have an application-specific naming convention:

`appname:endorsementname`

The appname 'system' is used for internal Centrallix endorsements.  A list of internal endorsements follows:

### system:from_application
Provides assurance that the current context was invoked from a running application that has a valid SGA Key (session management and CSRF token).  Reports and other objects that make changes without additional user interaction, or use large amounts of system resources, may require this endorsement to prevent a security compromise or denial of service attack.

## Access Types
There are seven primary access types that Centrallix uses:

| Access Type | Description
| ----------- | ------------
| create      | permission to create a new object of the type
| delete      | permission to delete an object of the type
| observe     | permission to observe that an object exists
| read        | permission to read the content of an object or attribute
| write       | permission to write to an object or attribute
| exec        | permission to 'execute' an object, which usually means opening it using its associated objectsystem driver, instead of just opening the underlying object.
| noexec      | permission to open an object without automatic 'execution', basically controlling the use of open-as (ls__type) processing and the EXEC/NOEXEC flags on queries and opens.

These two access types are used specifically for controlling the applicability of a security policy file, and for controlling when an endorsement may be added to a user's session:

| Access Type | Description
| ----------- | ------------
| delegate    | permission for a particular .pol file to be used to modify the access permissions of a user or users.
| endorse     | permission for a particular .app or .cmp (or similar) file to add a particular endorsement to a user's session.

Access Type interactions:

- If observe is allowed but read is not, the subject will be able to notice the existence of the object but not view its attributes or contents.

- If read is allowed but observe is not, the subject will be able to access the object only if the subject already knows the object's name (i.e., location in the objectsystem).  The object will not show up in a subobject query on its parent, unless the object is a node object and the query is done without EXEC (e.g., say an object of a database node type has restricted access; it would not show up if the query were set up to exec all nodes it finds.  If the query was running in a noexec manner, the nodes would not be invoked and the underlying file would be visible in the query unless otherwise restricted).

- With both 'execute' and 'noexecute' permissions on an object type, the subject can decide whether or not to access a node object as the underlying object (say, a file object), or as the object type in question (say, a database node object).  This is done via ls__type (to be very specific about what type to open as), or via the open flags OBJ_O_EXEC / OBJ_O_NOEXEC.

- If neither 'execute' nor 'noexecute' is granted for an object type, then the object cannot be opened as the type in question or as the underlying object type.

- If 'execute' permission is granted but not 'noexecute', then the object cannot be opened as its underlying object type.  This can be used to prevent access to sensitive information in the underlying source code of the object, or to prevent access to data except through "official" means (e.g., accessing rows in a CSV file vs. accessing the actual textual content of the CSV file; or accessing a database vs. being able to see the connection information, possibly including passwords, in the underlying database connector file).

- If 'noexecute' is granted but not 'execute', then the object cannot be opened as the type in question, but the underlying object can be opened.  For example, if execute is forbidden on 'shell' objects, then the user would be able to view the underlying shell object file that describes what command is to be executed, but the subject would not be able to open the object and cause the command to be run.  Similarly, if the object were a CSV file, the subject would be able to view the raw CSV file data, but not open the file and query its individual rows.

## Security Type Declarations
Various data sources within Centrallix will need to declare the security type associated with the source's data.  For example, a database connector object may need to declare the security type of the database, its tables, and the rows within the various tables.  There could be one generic type for all rows, with the ability to specify an alternate type for rows in specific tables.  This same concept can apply to fields within tables as well.

## Application-Specific Security Configuration
The security policy system permits the inclusion of (delegation to) other policy files beyond the main system security policy.  This mechanism allows finer grained control of per-application security without having to modify the main policy file for each application's needs.

## Security Policy File
The security policy file will be a structure file.  See the below file for a sample of how a policy file is constructed:

`SecurityModel_SamplePolicy.pol`

Below are the components of a policy:

### A.	Global Policy Settings "system/sec-policy"
#### mode = {disable|warn|enforce}
This setting allows the system administrator to completely disable the security subsystem, or to leave it disabled with warning messages about where security failures would occur.  This setting is only valid in the top level (main) security policy.

#### default = {allow|deny|none}
This setting, most commonly used in sub- policies, controls what the default action is should no rules match.  The 'none' option is only valid in sub-policies.

#### domain = "string"
Declares the name of the domain, which shows up as the first part of object specs and endorsement specs.  This defaults to "system" on the top level policy.  The domain is allowed to be the same as that of the parent policy only if the delegation rules in the parent policy permit it.  Otherwise the domain must be unique system-wide.  Policies encountered with duplicate domains will be rejected.

#### domain_path = "string"
This declares the domain's path in the ObjectSystem, that this policy applies to.  Defaults to "/" on the top level policy, and to the directory containing the .pol file for sub-policies.

### B.	Subject Definitions "system/sec-policy-subject"
#### authentication_method = {static|unix|altpasswd|...}
Defines the method of identification and authentication that are allowed for this subject list.  This is a comma-separated list of values.

#### required_authentications = Number
The number of authentication methods required in order for authentication to succeed.  This is used for two-factor authentication.

#### identity = "string"
This provides the identity (user name), and is required for the 'static' method.

#### password = "string"
For the 'static' authentication method, this provides the password hash, in the same format used in /etc/shadow files.  If this is left unset, then no password is required.

#### key = "string"
An RSA public key, in PEM format, that the subject can use to identify itself.  This is only really useful when the subject list only contains a single subject, and is typically used when codesigning to prove authorship.

#### use_as_default = {no|yes}
Whether this subject is used as the default subject should the user not yet be logged in.  This is used to provide anonymous access to the system.  Defaults to 'no'.

#### criteria = {expression}
Allows for the restriction of this subject list to only a subset of the subjects that the source (unix, altpasswd, etc.) can provide.  

#### add_endorsement = "string"
Adds the given endorsement to the user's session when this subject list is used to identify and authenticate the user.

## Signing Key Configuration
A security policy file may define what signing keys are used to identify an object as having been authored by a specific identity.  The public key of an RSA key pair will be included in or referenced by the policy file in a subjectkey directive.

## Authentication Mapping and Token Storage
The security subsystem will often need to map identities between the Centrallix server and local or remote data sources, and authentication tokens may need to be stored to permit access to those remote sources. Furthermore, sometimes a user may need to connect to a remote source as a different identity, and that information can be stored as well.

The basic premise of the key repository is that the stored auth tokens (passwords, cookies, oauth1/2 tokens, etc.) will be encrypted with the user's login password (or other persistent authentication token) as the primary key dependency.  If the user's login password changes, then the system will need to prompt the user for their old password in order to update the key repository.

The token storage subsystem will have the following aspects:

### A.	User Token (password)
This is supplied by the user when the user logs in.  Since it may be possible for a user to have more than one valid token, there could be multiple entries here for the user, with the User Private Key made available via each of them.

### B.	User Salt
This is a random plaintext value generated when the user logs in the first time, and is used by the KDF (see User Key).

### C.	User Key
When the user logs in, the password (User Token) and the User Salt are processed through a Key Derivation Function (KDF), with a configurable number of rounds, to produce a 256-bit User Key.  The User Key is never stored, but is kept in memory while the user has an active session (login) going, since the KDF is intentionally an expensive operation.

### D.	User Public Key and Private Key
The first time the user connects to the system, an RSA public/private key pair is generated for the user's token storage.  This is different than public/private keys used for proof of authorship.  Public key cryptography is used here so that cryptographic tokens can be provided to a user without having to know the user's User Token or User Key in advance.  The User Public Key is stored in plaintext, and the User Private Key is encrypted with the User Key and stored encrypted.

### E.	Resource Data
This is identification and authentication data that the user needs in order to connect to a service outside of Centrallix, for example a database server or email server or web service.  The Resource Data is stored along with a Resource Data Salt, and is stored encrypted with the User Public Key.

The schema for this data storage will have three main entities:

- A.	User Data:
    1.  User identity (e.g., username or email)
    2.  User Public Key, unencrypted
    3.  Date Record Created

- B.	User Authentications:
    1.  User identity
    2.  Authentication class ("password", "oauth2", etc.)
    3.  User Salt
    4.  User Private Key, encrypted with User Key
    5.  Removal Flag
    6.  Date Record Created

- C.	User Resources:
    1.  User identity
    2.  Resource ID (a string)
    3.  Resource Data Salt
    4.  Resource Data, encrypted with User Private Key and the Resource Data Salt.
    5.  Date Record Created

The process for looking up the correct row in User Authentications may involve one or more tries, since it is not known in advance which record the current User Token applies to.  In order to verify that a record is correct, the User Key is generated, used to decrypt the User Private Key, and then the User Private Key is verified against the User Public Key to ensure a match.  If there is a match, then the correct record has been found, otherwise the system must continue to check records for the given user identity.

If a user changes their user token (password), then this database will need to be updated.  If a mechanism is provided to change a password through Centrallix, or where Centrallix is notified (e.g., via PAM) of the change, this process can take place automatically.  Otherwise, the user may need to be prompted for their old token (password) in order to regain access to the User Private Key.  Then, a new User Auth record is created based on the new User Key, and the previous User Auth record is removed or is marked for removal, to ensure User Private Key recovery in the event of an unauthorized password change.

## OSML Hooks (Coverage Enforcement)
In order to implement, and prevent the bypassing of, the security policy, there needs to be a complete set of hooks in the OSML to call into the security reference monitor for authorization.  These hooks are described below:

### objOpen:
An object may be opened, with a given mode (r, w, or rw), if the corresponding access types (read, write, or read and write) are allowed for the object.  The OSML should check these two access types based on the mode.  In the event of an invalid mode (03), the open must fail.

### obj_internal_ProcessOpen:
This internal processing function handles the chaining of drivers based on object types and so forth.  This function will need to check for exec and noexec permissions when deciding whether to chain drivers or handle the open-as logic.  If a driver would be chained, but open-as would cause it not to be, then noexec permission is required; if noexec is not available, then the open must fail.  If a driver would be chained, and open-as is not instructing otherwise, then exec permission is required; if exec permission is not available in that case, then the driver is simply not chained, as opposed to returning a failure.  If neither exec nor noexec is available in a situation where a driver would normally be chained, then the open must fail.  To summarize:

```
would-chain + open-as + !noexec:	FAIL
would-chain + open-as + noexec:		SUCCEED
would-chain + !exec + noexec:		SUCCEED, NO CHAIN
would-chain + !exec + !noexec:		FAIL
would-chain + exec:			SUCCEED
```

Of course, if chaining is prevented, and the end of the path has not been reached, then the open will fail nonetheless, but not with a security error but with a no driver to handle the path type of error.

### driver xxxOpen():
Since the Open operation, with OBJ_O_CREAT set, can create a new object, each driver instance must check for create permission before acting on the OBJ_O_CREAT flag.

### objClose:
No checks need to be made, unless an automatic commit occurs, in which case write permission is required.

### objCreate:
The hook should check for both create permissions for the given object type and path.  The hook will need to determine the object type before allowing the create operation to proceed, which may require additional OSML functionality.

### objDelete, objDeleteObj:
These functions should check for delete permissions for the given object type and path.

### objInfo, objPresentationHints, objRead, objGetAttrType, objGetAttrValue, objGetFirstAttr, objGetNextAttr, objGetFirstMethod, objGetNextMethod, objOpenAttr:
These functions should check for read permissions for the given object type and path.  objGetAttrType and objGetAttrValue are allowed to return the "name", "inner_type", and "outer_type" attributes if read OR observe permissions are available.

### objSetAttrValue, objExecuteMethod, objWrite, objAddAttr, objAddVirtualAttr:
These functions should check for write permissions for the given object type and path.

### objOpenQuery, objQueryClose:
These function need no security checks.  Checks are performed when objects are actually fetched.

### objQueryFetch:
This function must check for observe permissions on any object to be returned.  If no objects remain that are observable, the function must return NULL.

### objQueryDelete:
This function must check for delete permissions on any subobject to be deleted.

### objQueryCreate:
This function must check for create permissions on the subobject to be created.

### objMultiQuery, objGetQueryCoverageMask, objGetQueryIdentityPath:
These functions need no security checks, since the security checks are handled at a lower level as the SQL engine calls back into the OSML to perform its operations.

## Code Signing
In order to provide strong assurance of authorship, code signing will be available.

### A.	Code Signing Format
The format to be used for code signing will be a PEM digital signature appended to the end of the object in question (an app, cmp, rpt, pol, etc., file).  The signature will be generated based on the entire object content leading up to the beginning of the PEM block, including the newline character right before the PEM block.  As such, code that lacks a trailing newline will need such a newline added as a part of the signing process.  Developers should be aware of this quirk if using an editor which does not automatically ensure a trailing newline.

### B.	Code Signing Utility
A basic code signing utility will be provided, but GPG may be used if the leading "begin pgp signed message", "hash:", and blank line are removed, or if an armored detached signature is created and appended to the file (assuming the file contained a trailing newline).  The code signing tool will 1) remove any existing digital signature from the file, 2) ensure the file has a trailing newline, 3) call gpg to generate a detached signature, and 4) append the signature to the end of the file.

### C.	Verification of Authorship
The OSML will provide a system attribute, cx__author, which returns a string value containing the author's subject name (such as "u:root" or whatnot).

### D.	Digital Signature Verification

## Security Subsystem Tasks and Phasing
### Phase One
- A.	OSML Hooks and CXSS authorization service stub
- B.	Policy data structures and policy loader
- C.	Identification/Authentication CXSS service
- D.	CXSS Authorization service and policy execution logic
- E.	CXSS Endorsement execution/loading service
- F.	CXSS Logging/Audit service

### Phase Two
- G.	Login and Role Selection App and CXSS integration

### Phase Three
- H.	Code Signing Utility
- I.	CXSS Code Signing Verifier Service and cx__author attr implementation
- J.	CXSS Token Mapping and Storage service
