# Web FIDO2-enabled Logins for Centrallix

Author:	Greg Beeley

Date:	June 12, 2025

## Overview

Currently, Centrallix accepts logins only via HTTP Basic authentication, either in strict or non-strict mode.  This has inherent limitations, including the lack of ability to make use of second factors via FIDO2 / U2F.

This document describes a proposal for enabling web form logins and FIDO2 / U2F support.

## Dependencies

1.	Credentials Manager.  Centrallix will need a way to store user public keys in order to enable FIDO2 support.  The credentials manager has been developed and is waiting on code review, and will need to be reviewed and integrated into the master branch prior to starting this project.

2.	libfido2.  The Yubico libfido2 C library will need to be reviewed for use in Centrallix, including any requirements for certain types of memory management, threading, callbacks, etc., that may or may not be compatible with how Centrallix and Centrallix-lib work.  If compatibility issues exist, then the libfido2 integration will need to be done out-of-process via socket communications.

3.	CXSS authentication component of security policy.  The security policy subsystem will need to be completed to the point where the authentication configuration component works, so that FIDO2/U2F can be utilized.

4.	SMTP objectsystem driver.  This will be needed for sending email challenges for two-factor authentication.  Support for the MIME objectsystem driver may also be needed.

5.	Twilio support.  This will be needed for sending Text/SMS challenges for two-factor authentication.

## Design Principles

1.	CXSS wrapper.  A set of functions in CXSS should be developed to "wrap" the FIDO2 functionality and invoke libfido2 or else communicate with a libfido2 authentication subprocess.  These functions should include both registration and authentication functionality.

2.	Stateless.  The interaction with the user should be completely stateless - no sessions should be created in Centrallix until authentication by all required factors is complete.  This will require a design which utilizes authenticated (signed) data so that the server can recognize, with cryptographic certainty, that the data provided by the client represents a full authentication process.

3.	Minimal attack surface.  Only a minimum of server functionality should be accessible to the connecting user before authentication is complete.  This likely means that any login screens will be static HTML rather than Centrallix apps, and communication with the server will be via specific endpoints that do not include any of the standard Centrallix client interaction features and APIs.

4.	Password login first.  The user should be asked to log in via their username and password first, and if that succeeds, challenge them with the FIDO2 / U2F authentication.  This is the way that most web apps operate, and it provides an oppotunity to recognize potentially compromised passwords if the user provides a correct password but no valid FIDO2 authentication.

5.	Registration management.  If the user does not have a second factor, registering a second factor is allowed without already having one.  Once the user has a second factor, then a second factor must be provided in order to manage factor registrations or to add another registration.

6.	Supported factors.  Supported second factors should include at least FIDO2, Text/SMS, Email, and Recovery codes.  Not all sites may choose to enable all factor types.
