# SQL Exception handling for Centrallix

Author:	Greg Beeley

Date:	November 11, 2025, updated September 7, 2026

## Overview

Currently, Centrallix SQL operations only fail on a lower level error condition, such as an incorrect column name or a nonexistent table or other data source.  This document describes enhancements to CXSQL to improve exception handling and reporting, particularly for exceptions that are recognized by the SQL code itself.

The primary enhancements this document addresses are:

1.	A `RAISERROR` statement to trigger exception processing when the SQL code detects an error condition.

2.	An OSML API mechanism to retrieve error conditions on a query (pObjQuery).

3.	An OSML-over-HTTP interface mechanism to deliver error conditions to the client.

4.	An Error event for widget/osrc that can be hooked by a connector to act on an error condition or (by default) alert the user.

5.	A `FROM` clause enhancement, `RAISERROR EXISTS`, to raise an error when a data source element doesn't exist or when an alternate data source element does exist.

## Future Directions

This document covers some aspects of exception handling, but isn't nearly exhaustive.  Some possible future directions could include:

1.	A TRY CATCH block to handle errors (RAISERROR or lower level errors).

2.	Full transaction management including BEGIN TRANSACTION, COMMIT, and ROLLBACK.

3.	A `MULTIPLE` keyword for `RAISERROR EXISTS` to only raise an error if the `FROM` source matches more than one object.

4.	The ability to `RETRY` an entire script, not just the failing command.

## Error IDs

The `RAISERROR` and `ON ERROR` statements below utilize error IDs to manage error messages.  Error IDs provided by an application can be any value 1000 and above.  Values lower than 1000 are reserved for internal use.

If a statement fails due to an internal or OSML error rather than a `RAISERROR` statement, a lower level error ID will be set.  Below is a proposed list of such predefined IDs; applications may not use these IDs in a `RAISERROR` statement but may use them in `ON ERROR`.

| Error Code | Description
| ---------- | -------------
| 1-199      | Same as Linux errno values, common values shown below:
| 1          | EPERM - operation not permitted (system-level / inherent)
| 2          | ENOENT - the object or file does not exist
| 12         | ENOMEM - out of memory
| 13         | EACCESS - access denied (by policy, file permissions, etc.)
| 17         | EEXIST - the object or file exists (i.e. already exists)
| 22         | EINVAL - Invalid argument or parameter (e.g., bad function argument, parameter failed hints evaluation, etc.)
| 33         | EDOM - Mathematical function argument outside of allowed range of values
| 34         | ERANGE - Mathematical result cannot be represented: overflow, underflow, etc.
| 200        | Syntax or parse error: any malformed command or object

Values matching common Linux errno codes can be matched as numeric values or as their symbolic equivalents (e.g. access denied can be matched as numeric 13 or as EACCESS).

## RAISERROR statement

`RAISERROR` is a SQL statement common to MSSQL and Sybase database servers, and is used to "throw" an error from a SQL script or stored procedure.  The syntax of the proposed `RAISERROR` statement is:

```sql
RAISERROR [ {error-id}, ] {error-text} [ WHERE {error-condition} ]
```

The `error-id` is an optional application-defined code that can be referenced in other error handling primitives.

`error-text` is required and provides a message that can be logged and/or delivered to the end-user.

`error-condition` is a part of an optional `WHERE` clause that gives the condition required in order to raise the error.  Other SQLs that use `RAISERROR` do not have such a `WHERE` clause, but it's needed in this case because CXSQL intentionally avoids procedural control flow structures such as if/else and loops, in order to preserve the more declarative nature of the CXSQL language.

When an error is raised, by default the SQL script terminates and the error message is logged and/or delivered to the user.  This behavior can be modified by an `ON ERROR` command issued earlier in the script.

## RAISERROR EXISTS join source qualifier

Some SQL operations involve data sources that should be considered an error condition if they exist (or do not exist).  An example of existence implying an error is an API response where a sub-object in the response contains an error message or code.  An example of nonexistence implying an error might be a foreign key relationship with a missing code (validation) table entry.

The syntax of the `RAISERROR EXISTS` join source qualifier for a data source item's existence implying an error condition is:

```sql
SELECT
    ...
FROM
    ... ,
    RAISERROR [ {error-id}, {error-text} ] EXISTS /data/source/implying/errors
```

And for a data source item's nonexistence implying an error:

```sql
SELECT
    ...
FROM
    ... ,
    RAISERROR [ {error-id}, {error-text} ] NOT EXISTS /required/validation/table
```

This join source qualifier can be combined with other qualifiers such as `EXPRESSION` and `SUBTREE`.

## ON ERROR error handling statement

By default, when an error is raised, the SQL script halts and the error message is logged and/or passed to the user.  The `ON ERROR` statement can be used to change that behavior.  Possible `ON ERROR` forms include:

1.	Ignoring the error entirely.  This form causes the error to be ignored (not logged, not sent to the user).

```sql
ON ERROR {error-id} IGNORE;
```

2.	Retrying the SQL command.

```sql
ON ERROR {error-id} RETRY;
```

3.	Raising a different error.

```sql
ON ERROR {error-id} RAISERROR ...
```

4.	Aborting the SQL script (the default).

```sql
ON ERROR {error-id} ABORT;
```

5.	Calling another SQL script.

```sql
ON ERROR {error-id} EXEC /path/to/other/script [ parameters... ];
```

6.	Imposing a limit on the number of times that the error can be handled in a certain way.  If the limit is exceeded, the default action will be taken (abort the script).  Note that the limit is applied on a script-wide basis (or until another `ON ERROR` is encountered with a `LIMIT` clause, which resets the count).

```sql
ON ERROR {error-id} LIMIT 3 RETRY;
```

7.	Calling another SQL script and also retrying, with an optional limit as well.

```sql
ON ERROR {error-id} LIMIT 3 EXEC /path/to/script [ parameters... ] AND RETRY;
```

8.	Including a delay (in seconds) when retrying.

```sql
ON ERROR {error-id} LIMIT 3 DELAY 10 EXEC /path/to/script [ parameters... ] AND RETRY;
```

## OSML API Method to Retrieve Error Data: objQueryError()

One "hole" in the OSML API is that errors that occur during query execution (not just in starting the query) are not passed to the caller through the call level interface semantics and are only made available on the error stack.  The call level interface can only return NULL from objQueryFetch().  Thus, an OSML API caller cannot currently recognize the occurrence of an error in query execution in a predictable manner.

`objQueryError()` provides not only a way to tell if the NULL return from objQueryFetch() is an end-of-results or an error condition, but it also provides a way to retrieve the raised error code and message.

Though raised errors will be placed on the mssError() error stack, it can also be useful to retrieve the message provided in a `RAISERROR` statement:

```c
int objQueryError(pObjQuery qy, int* error_id, char* error_message, int maxlen);
```

This function will store the error ID and message in `error_id` and `error_message`, respectively.  Its return value is 0 if the error information was successfully stored, or (-1) if no error message was available (i.e., the script succeeded).  If `error_id` and/or `error_message` are NULL, this script returns the 0 or -1 return value without storing either or both the ID and message, and can thus be used to check for an error condition without actually storing the id or message.

## OSML Driver Interface Updates

The query process in the OSML (and the MultiQuery layer itself) will need to feed error data back to the OSML:

```c
int obj_internal_RaiseQueryError(pObjQuery qy, int error_id, char error_message);
```

Note that this method is not for general mssError() style low level errors, but instead for errors that are noted during the execution of a query or SQL command.  An operation failing, logging error messages via mssError(), and returning a failure status can still result in an error condition that can later be caught or sent to the user.

## Passing Errors via OSML-over-HTTP

Currently, the user has to check the log to find out why an operation failed in the user interface.  This subpart describes a way to deliver error messages to the client via the OSML-over-HTTP current implementation.

The current implementation indicates an error by setting the `<A>` `TARGET` attribute to "ERR".  However, the anchor/link text is typically left entirely blank.

The proposed change is to pass textual error information in the link text field, so that an error might look like this:

```html
<a href="/" target="ERR">Access Denied</a>
```

The intent is for the value in the link text to be actually shown to the user, which will affect for example how deeply into the error stack mssError() style information is provided.

## Error Event on ObjectSource Widget

An `Error` event will be added to "widget/objectsource" so that errors from the server can be caught and handled.  The event will provide properties (event parameters / eparams) `Message` and `ID` to pass error information that came from mssError() or from `RAISERROR`.  The `Error` event can be canceled using `event_cancel`, in which case the server will be instructed to get its act together and redo the operation the way the programmer intended it instead of coded it (just kidding), or simply not display the error message to the user.
