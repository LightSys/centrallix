## Tooling

To generate a report, open up the Kardia/Centrallix instance running on your firewall.  The ports will be shown on the Kardia interface.
There is no specific path to generating a report, 

### Generating Compile Commands

Install compiledb with `sudo python3 -m pip install compiledb`.
Generate the compile_commands.json with the following commands (starting at root).

```sh
cd centrallix
compiledb make all
```

The reason for creating this is to use a tool such as [clangd](https://clangd.llvm.org/).
There is a VSCode extension which supports using it, even on servers which do not support clangd.
`clangd` will allow for LSP usage within the project, adding helpful multi-file errors and jump to definition.
Perhaps it may be wise to at some point go over the entire project and resolve its warnings and errors as possible.

## Process

### Generating a Report

To generate a report, open up the Kardia/Centrallix instance running on your firewall. The ports will be shown on the Kardia interface.
There is no specific path to generating a report, it depends on which sort of report you would like to generate.  
Ask Greg which reports he would like you to work on.

### Sending an HTML Email

This requires the use of a Mutt client on a Linux machine for simplicity.
The configuration used ([source](https://mritunjaysharma394.medium.com/how-to-set-up-mutt-text-based-mail-client-with-gmail-993ae40b0003)) is specific to GMail clients.
The template configuration for Mutt using a GMail account can be found below.
You will need to generate an app password with the instructions [here](https://support.google.com/accounts/answer/185833?hl=en), and use that as your password.

```
# ================  IMAP ====================
set imap_user = <youremail>@gmail.com
set imap_pass = <pass>
set spoolfile = imaps://imap.gmail.com/INBOX
set folder = imaps://imap.gmail.com/
set record="imaps://imap.gmail.com/[Gmail]/Sent Mail"
set postponed="imaps://imap.gmail.com/[Gmail]/Drafts"
set mbox="imaps://imap.gmail.com/[Gmail]/All Mail"
set header_cache = "~/.mutt/cache/headers"
set message_cachedir = "~/.mutt/cache/bodies"
set certificate_file = "~/.mutt/certificates"
# ================  SMTP  ====================
set smtp_url = "smtp://<youremail>@smtp.gmail.com:587/"
set smtp_pass = $imap_pass
set ssl_force_tls = yes # Require encrypted connection
# ================  Composition  ====================
set editor = "nvim"      # Set your favourite editor.
set edit_headers = yes  # See the headers when editing
set charset = UTF-8     # value of $LANG; also fallback for send_charset
# Sender, email address, and sign-off line must match
unset use_domain
set realname = "<name>"
set from = "<youremail>@gmail.com"
set use_from = yes
```

Once Mutt has been set up, use the following command to send the HTML email from your GMail account.
It is indeed totally valid to use your own email address as the target email.
```
mutt -e "set content_type=text/html" <TARGET_EMAIL> -s "<EMAIL_SUBJECT>" < <PATH_TO_FILE>
```

## PrtObjStream and pPrtObjStream

The `PrtObjStream` struct and pointer (`pPrtObjStream`) store all of the information about the data being printed. The struct definition can be found in [prtmgmt_v3.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3.h).

### Special Flags

There are a variety of useful flags in the `PrtObjStream` struct scattered throughout the project. We have documented most of the ones we have found below. Most of the flags are defined in the same file as the `PrtObjStream` flag.

#### PRT_OBJ_F_NEWLINE

- **Location**: [prtmgmt_v3.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3.h)

- **Description**: Indicates that the object begins with a newline (typically for string flow).

#### PRT_OBJ_F_SOFTNEWLINE

- **Location**: [prtmgmt_v3.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3.h)

- **Description**: Indicates that the object begins with a soft newline (typically for string flow). Soft newlines are inserted when the PrtObjStream is created to define breakpoints for the PDF generator; they are not defined in the .rpt files.

#### PRT_OBJ_F_XSET

- **Location**: [prtmgmt_v3.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3.h)

- **Description**: Indicates that the object has an abosulute x-position defined by the .rpt file.

#### PRT_OBJ_F_YSET

- **Location**: [prtmgmt_v3.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3.h)

- **Description**: Indicates that the object has an abosulute y-position defined by the .rpt file.

#### PRT_TEXTLM_F_RMSPACE

- **Location**: [prtmgmt_v3_lm_text.h](../centrallix/include/prtmgmt_v3/prtmgmt_v3_lm_text.h)

- **Description**: Indicates that the string object replaced a space character with a soft newline. Useful for deciding whether or not to insert a space character when a soft newline is found.

