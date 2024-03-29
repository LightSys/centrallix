# Authorization tokens overview
Date: 13-Nov-2013

Author: Greg Beeley (GRB)

## 1. HTTP cookie
- Received from the browser with every request, whether user-initiated or not.
- 128-bit hex-encoded secure PRNG value.
- Transmitted by the server when the user successfully authenticates

## 2. Application Key - Session Token
- 128-bit hex-encoded secure PRNG value which identifies the session
- Present on the Location URL line
- Received with AJAX requests
- Received when opening a new page in the current application group
- Transmitted within an application when the app is generated

## 3. Application Key - AppGroup Token
- 64-bit hex-encoded secure PRNG value which identifies the App Group within a session
- Present on the Location URL line
- Received AJAX requests
- Received when opening a new page in the current application group
- Transmitted within an application when the app is generated

## 4. Application Key - App Token
- 64-bit hex-encoded secure PRNG value which identifies the Application within an App Group
- NEVER present on the Location URL line for normal requests
- Received with AJAX requests
- Transmitted within an application when the app is generated

## 5. Session Linking Ticket
- 128-bit value
- Valid for a configurable amount of time, default 60 seconds
- Single-use only
- Source IP address must match
- Requestor can specify what application will be loaded
- Generated on-demand via ls__mode=genticket and added to a one-time use list in the session structure of session linking tickets
- Transmitted in response to the genticket request
- Received when browser is requesting a new page
- Server responds by changing the cookie it sends to that associated with the session being linked
- AKey session and appgroup tokens must also be valid

## 6. BASIC http credentials
- Username:Password in base64-encoding
- Received from the browser with each request when the user is using http basic authentication
- Password part is never transmitted by the server
- Username part is transmitted within an application when the app is generated
- If present, it must be correct, even after a session is established
