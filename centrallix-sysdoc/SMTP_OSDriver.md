<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 2014-2026 LightSys Technology Services, Inc.           -->
<!--                                                                      -->
<!-- This program is free software; you can redistribute it and/or modify -->
<!-- it under the terms of the GNU General Public License as published by -->
<!-- the Free Software Foundation; either version 2 of the License, or    -->
<!-- (at your option) any later version.                                  -->
<!--                                                                      -->
<!-- This program is distributed in the hope that it will be useful,      -->
<!-- but WITHOUT ANY WARRANTY; without even the implied warranty of       -->
<!-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        -->
<!-- GNU General Public License for more details.                         -->
<!--                                                                      -->
<!-- You should have received a copy of the GNU General Public License    -->
<!-- along with this program; if not, write to the Free Software          -->
<!-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             -->
<!-- 02111-1307  USA                                                      -->
<!--                                                                      -->
<!-- A copy of the GNU General Public License has been included in this   -->
<!-- distribution in the file "COPYING".                                  -->
<!--                                                                      -->
<!-- File:        SMTP_OSDriver.md                                        -->
<!-- Author:      Justin Southworth, Hazen Johnson                        -->
<!-- Creation:    July 31st, 2014                                         -->
<!-- Description: Describes the usage and implementation of the SMTP      -->
<!--              ObjectSystem driver.                                    -->
<!-------------------------------------------------------------------------->

# SMTP ObjectSystem Driver

**Author**:  Justin Southworth, Hazen Johnson

**Date**:    July 31, 2014



## Table of Contents
- [SMTP ObjectSystem Driver](#smtp-objectsystem-driver)
  - [Table of Contents](#table-of-contents)
  - [I Introduction](#i-introduction)
  - [II Usage](#ii-usage)
  - [III Interface and Implementation](#iii-interface-and-implementation)
    - [A. Initialization](#a-initialization)
    - [B. Opening and Closing Objects](#b-opening-and-closing-objects)
    - [C. Creating and Deleting Objects](#c-creating-and-deleting-objects)
    - [D. Reading and Writing Object Content](#d-reading-and-writing-object-content)
    - [E. Querying for Child Objects](#e-querying-for-child-objects)
    - [F. Managing Object Attributes](#f-managing-object-attributes)
    - [G. Managing Object Methods](#g-managing-object-methods)
  - [IV Limitations](#iv-limitations)



## I Introduction
The SMTP OS Driver provides the capability for the Centrallix Object System to send emails with `sendmail`.  An SMTP object's directory structure consists of a root node containing information universal to all sent emails and of multiple emails as child objects.



## II Usage
In order to use the SMTP driver, the root node must first have all required [node attributes](#node-attributes).  Of these, `spool_dir` is the most important attribute, and the optional `expire_time`, `content_has_headers`, and `local_host_name` attributes may also be helpful for defining how emails are created and sent.

Email objects are created as children of the root SMTP node and, when created, contain no content or headers.  Further modification of the email object should be accomplished through the MIME driver; however, this is not currently supported, so the SMTP driver provides several [email node attributes](#email-attributes) for setting certain, important headers (e.g. `To`, `From`, etc.).

> ⚠️ **Warning**: Before an email can be sent, the `envelope_from` attribute must be set.  Otherwise, the sent email will be registered as from the user running Centrallix.  As this is normally blocked by most email servers, this will cause the email to fail to send.

Email recipients should be determined from the email message itself; however, additional recipients may be added by using the `envelope_to` attribute.

To send an email, set the `is_ready` attribute to 1.  The driver hands the email to Postfix through `sendmail` and sets `status` to Pending.  Each time an email is opened or sent, the driver reads the lines Postfix added to `/var/log/maillog` since its spool directory last read it, records their results in the structs of the spool's Pending emails, and sets `status` to Sent or Error once Postfix finishes.  Each spool directory records where it stopped in `.mail_log_cursor`, so it continues there after a restart.  An email that is still Pending 6 days after `last_try_date` becomes Error, or 2 minutes after it if sendmail never reported a result and Postfix never logged it.  Writing the content or setting the attributes of a Pending email fails.

Sent means the next mail server accepted the email for every recipient.



## III Interface and Implementation
The SMTP driver does not implement the entire OS driver interface.  Its functionality in each of the standard interface functions is as follows.


### A. Initialization
The SMTP driver registers itself for the `"system/smtp"` content type.  This identifies the SMTP root node and is a `"system/structure"` type file.

The driver opens `/var/log/maillog` as root to read the results of emails handed to Postfix, so Centrallix must run as root.  To read a different log, set `mail_log` in an `smtp` block of `centrallix.conf`:

```
smtp "system/config"
    {
    mail_log = "/var/log/maillog";
    }
```

> ⚠️ **Warning**: The driver expects to only be initialized once. It initializes global values that are never deinitialized, so multiple initialization calls may cause memory leaks.


### B. Opening and Closing Objects
As far as it has been tested, the SMTP driver conforms to the standards required by the Object System for opening and closing.

> 📖 **Note**:   The `OBJ_O_TRUNC` flag has not been implemented or tested.

Internally, the SMTP driver opens objects as follows:

1.  Determine if the object is a root node or an email object.
2.  Open the root node and initialize any other properties held in common between the root node and email objects using the `smtp_internal_OpenGeneral()` function:
    1.  Attempt to open the root node.
    2.  Create the root node if opening fails, the `O_CREAT` flag is set, and the root node is the last element in the path.
    3.  Initialize the attribute arrays.
3.  Use the root node/email object specific internal open function to initialize the different attributes needed by the respective object types.
    - `smtp_internal_OpenRoot()`: Loads the root node attributes.
    - `smtp_internal_OpenEml()`:
        1.  Attempt to open the email file.
        2.  Create the email object (a struct and a MIME file) with default attributes.
        3.  Record the results Postfix logged since the spool directory last read the mail log.
        4.  Open the email struct file, update its send status if it is Pending, and fill out the attribute array.

The `Close()` routine simply cleans up the structures used to store the SMTP object's attributes after opening as per normal ObjectSystem close.


### C. Creating and Deleting Objects
Both the root node and an email object for the SMTP driver may be created using `smtpCreate()`.  (The root node is handled directly by an internal create function while emails are handled through `smtpOpen()`.)

`OBJ_O_AUTONAME` is supported for email object creation.  Generated names use the form `xxxxxxxx-xxxxxxxx.eml`, where each x is a hex digit of 8 random bytes.  If a file with that name already exists, the driver generates a new name (with up to 100 attempts).

Deleting will fail on the root node of the SMTP directory because the root node is also the node object.  Instead, this driver's parent (usually the file system driver, `objdrv_ux.c`) should be called to delete this object.


### D. Reading and Writing Object Content
SMTP root nodes are not writeable or readable; however, email objects are designed to be relatively transparent to the underlying file.  Operations passed into `smtpRead()`/`smtpWrite()` are passed through to the `fdRead()`/`fdWrite()` functions as is.  The file handle is opened when the email object is opened, and it points to the MIME message file so that the MIME driver can perform reads and writes appropriately.

Encoding and decoding of emails is performed by the MIME driver.


### E. Querying for Child Objects
The only query-able SMTP object is the root node.  The email objects may be query-able; however, this functionality will be provided by the MIME driver.

When the root node is queried, it opens the spool directory.  Each subsequent `smtpQueryFetch()` returns the next email object in the spool directory.  This means that, if two root nodes share the same spool directory, they will also share children (which are emails).  Any changes made to the children of one such root node will change the children of the other root node because the children are the same objects (although the OSML may create a second object because of the OSML pathname difference).


### F. Managing Object Attributes
SMTP attributes are stored in a dynamic array so that new attributes may be easily integrated into SMTP objects as necessary.  Default attributes are stored in the global data structure for the SMTP driver and/or are calculated on the fly when an object is created (e.g. Message ID).

While many attributes were specified in the [Email_OSDriver.md](Email_OSDriver.md) specifications, only a few attributes actually carry functional weight.  Following is a list of the specified attributes and the state of their implementation:

#### Node Attributes
| Attribute           | Description
| ------------------- | -----------
| *send_method*\*     | Specifies how emails are to be sent.  The SMTP driver currently supports only the sendmail utility for sending emails.  As a result, this attribute is useless.
| *server*\*          | The DNS name or IP address of the server to use when sending via direct-to-MTA SMTP.  Not used by sendmail.
| *port*\*            | The TCP port to use on the remote server when sending via direct-to-MTA SMTP.  Not used by sendmail.
| spool_dir           | The file path of the spool directory that will be used for storing messages that have not yet been sent.  This should be a location that supports the storage of arbitrary files.
| *log_dir*\*         | The OSML directory in which to place log messages about the success or failure of transmitting email messages.  This should be a location that supports the log attributes listed below.
| *log_date_attr*\*   | The attribute name in which to place the date that the log message was created.
| *log_msgid_attr*\*  | The attribute name in which to place the Message-ID of the email message being referenced by the log message.
| *log_info_attr*\*   | The attribute name in which to place the content of the log message itself.
| *ratelimit_time*\*  | The minimum number of seconds between each email sent.  This can be a floating-point value and so can be fractional (such as 0.5 to send at most two emails per second).  This defaults to 1 second (60 emails per minute).  Since it is not relevant to sendmail, it is not functional.
| *domlimit_time*\*   | The minimum number of seconds between each email sent to recipients at a given domain name.  This defaults to 5 seconds (12 emails per minute).
| expire_time         | The number of seconds to keep a sent or failed email (default 3 days).  Negative values keep it forever.
| content_has_headers | Whether the content written to an email begins with its own headers (default 1).  When 0, the driver adds a blank line after the headers it writes when sending, so the whole content is treated as the body.
| local_host_name     | The host name used in generated Message-IDs (default: this machine's host name).

\**Not implemented.*

#### Email Attributes
| Attribute                    | Description
| ---------------------------- | -----------
| name                         | The file name of this email, unique within its spool directory.
| message_id                   | The Message-ID of the email message, randomly generated by the driver when the email is created.  The driver tracks the send status by it, so sending fails if it is empty.  Read-only.
| envelope_from                | The envelope From address of the email (return-path).  Because of the way the SMTP driver currently uses sendmail, this attribute determines the outgoing From address received by the foreign email server.  If this is not set correctly, then the email will be from the user currently running Centrallix.  This will generally be blocked by most email receiving servers.
| envelope_to                  | The envelope recipient (or recipient list) of the email.
| header_date                  | The `Date` header, as a datetime.  Defaults to the time the email is sent.
| header_from                  | The `From` header.
| header_to                    | The `To` header.
| header_cc                    | The `Cc` header.
| header_bcc                   | The `Bcc` header.
| header_reply_to              | The `Reply-To` header.
| header_list_unsubscribe      | The `List-Unsubscribe` header.
| header_list_unsubscribe_post | The `List-Unsubscribe-Post` header.
| header_subject               | The `Subject` header.
| header_user_agent            | The `User-Agent` header (default `Centrallix/<version>`).
| header_mime_version          | The `MIME-Version` header.
| tag                          | An arbitrary label (not necessarily unique) used to find this email in later queries.
| status                       | The status of the email: Draft until `is_ready` is set to 1, Pending until Postfix finishes, then Sent or Error.  Read-only.
| is_ready                     | Set to 1 to send the email (default 0).  Setting it to 1 again after the email is Sent or Error sends it again.  Fails while the email is Pending or if `message_id` is empty.
| first_try_date               | The date/time of the first attempt to send this email (01 Jan 1900 until then).  Read-only.
| try_count                    | The number of attempts to send this email.  Read-only.
| expire_date                  | When a sent or failed email expires, set to `expire_time` seconds after it becomes Sent or Error.  01 Jan 1900 means never, and Draft and Pending emails never expire.  Expired emails are deleted when the spool directory is queried or an email is created, at most once per hour.  Read-only.
| last_try_date                | The date/time of the most recent attempt to send this email (01 Jan 1900 until then).  Read-only.
| last_try_status              | The result of the most recent attempt to send this email: None (not tried, or no failures), TempFail (a temporary failure, which Postfix retries while the email is Pending), or Fail (a permanent failure, set as soon as any recipient fails, even while the email is Pending).  Read-only.
| queue_id                     | The Postfix queue ID of the most recent attempt, recorded from the mail log once Postfix queues the email.  Read-only.
| rcpt_count                   | The number of recipients Postfix queued the most recent attempt for.  Read-only.
| last_try_msg                 | The failure details of the most recent attempt to send this email: sendmail's output, or how many recipients were sent the email followed by the latest result (bounced, deferred, or expired) and server reply of each recipient that was not, such as `Sent to 1 of 2 recipients. b@example.com: bounced: 550 5.1.1 User unknown`.  Updated as Postfix reports results.  Empty if there are no failures.  Read-only.

When the email is sent, `message_id` and each non-empty `header_*` attribute are written as headers, replacing any header of the same name in the content.


### G. Managing Object Methods
The SMTP driver does not support getting, calling, or adding methods.



## IV Limitations
- Running multiple Centrallix processes on the same spool directory is undefined behavior.
- After the mail log rotates, the driver finds the rest of the old log by its inode (such as `maillog-20261001`).  If logrotate compresses the old log as it rotates it (`compress` without `delaycompress`), or deletes it before the attributes of an email are reloaded (when it is opened, read, or sent), the results logged there after the last read are missed, so those emails become Error with an unknown send status after the timeout expires (even if they sent successfully).
- While a spool directory reads the mail log, opening its emails, reading their attributes, or sending them waits for the read to finish.  This might be noticeable if the mail log has many new lines (such as on the first read).
- Keep Postfix's `maximal_queue_lifetime` (5 days by default) under 6 days, or an email Postfix is still retrying becomes Error with an unknown send status.
- If Postfix uses a relay host, Sent means the relay accepted the email, so failures after the relay are not tracked.
- A deferred email cannot be resent or cancelled until Postfix gives up on it (`maximal_queue_lifetime`), because setting `is_ready` fails while the email is Pending.
- The driver only supports Postfix: it runs `/usr/sbin/sendmail` and reads Postfix's log format.
- Centrallix must run as root to read the mail log (see [Initialization](#a-initialization)).
- `spool_dir` must be an absolute path, because Centrallix runs from `/`.
- When an email is opened by its path (instead of by querying the SMTP node), the MIME driver handles it and passes the SMTP attributes through to this driver.  They can be read and set by name, but they do not appear in attribute listings such as `select *`.
- Rarely, a status is wrong:
    - Results added to the Postfix logs while they are rotated can be missed, so the email becomes Error with an unknown send status.
    - If sendmail queues the email but is killed by the 60 second timeout, the email becomes Error even though Postfix may send it, and sending it again sends a duplicate.
- Sending an email again reuses its Message-ID, and some mail services (such as Gmail) drop a message whose Message-ID they have already received.
