# Email/SMTP ObjectSystem Driver

Author: Greg Beeley (GRB)

Date: 17-Nov-2013

## Table of Contents
- [Email/SMTP ObjectSystem Driver](#emailsmtp-objectsystem-driver)
  - [Table of Contents](#table-of-contents)
  - [Object Tree Structure](#object-tree-structure)
  - [Node Attributes](#node-attributes)
  - [Email Message Attributes](#email-message-attributes)
  - [Email Message Content](#email-message-content)

## Object Tree Structure 
The object tree will consist of the SMTP node object, which contains the configuration including the email sending method, and a series of zero or more email messages that can be interpreted by the MIME driver.  The email SMTP driver will not have knowledge about the internal format of an email message.

```
NodeObject.smtp
    |
    +-- Email1.eml
    |     |
    |     +-- Body.txt
    |     |
    |     +-- Attachment1.pdf
    |     |
    |     +-- Attachment2.pdf
    |
    +-- Email2.eml
    |
    +-- Email3.eml
```

## Node Attributes

| Attribute         | Description
| ----------------- | -----------
| send_method       | Specifies how emails are to be sent.  This can be either "smtp" for direct-to-MTA emails, or "sendmail" to use the server's builtin email sending program (which could actually also be postfix or qmail or whatnot).
| server            | The DNS name or IP address of the server to use when sending via direct-to-MTA SMTP.
| port              | The TCP port to use on the remote server when sending via direct-to-MTA SMTP.
| spool_dir         | The OSML spool directory that will be used for storing messages that have not yet been sent.  This should be a location that supports the storage of arbitrary files.
| log_dir           | The OSML directory in which to place log messages about the success or failure of transmitting email messages. This should be a location that supports the log attributes listed below.
| log_date_attr     | The attribute name in which to place the date that the log message was created.
| log_msgid_attr    | The attribute name in which to place the Message-ID of the email message being referenced by the log message.
| log_info_attr     | The attribute name in which to place the content of the log message itself.
| ratelimit_time    | The minimum number of seconds between each email sent. This can be a floating-point value and so can be fractional (such as 0.5 to send at most two emails per second).  This defaults to 1 second (60 emails per minute).
| domlimit_time     | The minimum number of seconds between each email sent to recipients at a given domain name.  This defaults to 5 seconds (20 emails per minute).

The following attributes are only used when sending directly via SMTP (as opposed to via issuing a sendmail command and using the OS's builtin MTA).

| Attribute         | Description
| ----------------- | ------------
| expire_time       | The number of seconds that an email message and its status information will be kept in the spool directory after the email has either been sent or encountered a permanent error (default 3 days).
| retry_time        | The number of seconds to wait before attempting to send a particular email again (default 15 minutes).
| queue_time        | The maximum time (in seconds) that the system will keep retrying a temporarily failed email (default 5 days).
| handshake_time    | The maximum number of seconds the system will wait for the remote end to respond with various SMTP protocol messages (default 3 minutes).
| conn_limit        | The maximum number of SMTP connections to have open at any one given time.  This defaults to 1.  Note that this also controls the number of SMTP worker threads that this driver will spawn to handle SMTP connections.
| domconn_limit     | The maximum number of SMTP connections to have open at any one given time for a given domain.  This defaults to 1.

## Email Message Attributes
These attributes will be present on email message objects; however, the attributes will not be visible unless the messages are opened in such a way that the MIME driver does not automatically "cascade" onto the open operation.

These attributes will be stored in a structure file in the spool directory, using the same name as the .eml file but with a .struct file extension instead.  Both the .eml and .struct files will be removed after the expire_date has passed.

Attributes marked with a plus (+) can be modified until is_ready is set to 1, at which point all attributes become read-only.

Attributes marked with an asterisk (*) are only available when using direct SMTP (as opposed to sendmail).

| Attribute         | Description
| ----------------- | -----------
| name              | A unique identifier for this email.  Likely will be the same as the Message-ID, but with ".eml" appended to the end, for clarity.
| message_id        | The Message-ID of the email message being created.
| env_from+         | The envelope From address of the email (return-path).
| env_to+           | The envelope recipient (or recipient list) of the email.
| status            | The status of the email: Draft, Pending, Sent, Error.  The 'Error' status indicates a permanent failure to send the email.  If a TempFail (4xx) occurs while sending, then the message will remain in the Pending status (and the last_try_status attribute will be set to TempFail).
| is_ready+         | Either 0 (default) to indicate that the email is not ready to be sent or set to 1 to indicate that the email is ready for the SMTP driver to send.
| first_try_date    | The date/time of the first attempt to send this email.
| try_until_date*   | The latest that the driver will attempt to send this email.
| try_count*        | The number of times that the system has attempted to transmit the message.
| expire_date       | The latest date/time that this driver will retain status information about this email (after that, only the log messages in log_dir will remain available).  This defaults to 72 hours (3 days) after the email either permanently failed or was successfully sent.
| last_try_date     | The date/time of the most recent attempt to send this email.
| last_try_status   | The status of the last attempt to send this email (None, TempFail, Fail).
| last_try_msg      | The message from the last attempt to send; this could be the message the remote SMTP server provided in response to the attempt to send this email.

## Email Message Content
Each email message will contain content, which can contain arbitrary data but will be of type 'message/rfc822'.

The idea here is for the MIME message objectsystem driver to take care of the content of the email.

When a new email is created, the SMTP driver will provide some basic information to the MIME driver (by creating the template object content), including the following:

- Message-ID (containing the message_id)
- User-Agent (containing "Centrallix/0.9.1" or whatnot)
- Subject (blank)
- Date (current date)
- MIME-Version (containing "1.0")
