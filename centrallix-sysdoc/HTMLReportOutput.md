## Generating a Report

To generate a report, open up the Kardia/Centrallix instance running on your firewall.  The ports will be shown on the Kardia interface.
There is no specific path to generating a report, 

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