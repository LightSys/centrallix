<!-------------------------------------------------------------------------->
<!-- Centrallix Application Server System                                 -->
<!-- Centrallix Core                                                      -->
<!--                                                                      -->
<!-- Copyright (C) 1998-2012 LightSys Technology Services, Inc.           -->
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
<!-- File:        mtask.md                                                -->
<!-- Author:      Greg Beeley                                             -->
<!-- Creation:    January 13th, 1999                                      -->
<!-- Description: Describes how to use the mtask module in centrallix-lib -->
<!--              to manage network connections.                          -->
<!--              This text was moved to here from OSDriver_Authoring.md  -->
<!--              by Israel Fuller in November and Descember of 2025.     -->
<!-------------------------------------------------------------------------->

# Handling Network Connection

**Author**:  Greg Beeley

**Date**:    January 13, 1999

**Updated**: December 11, 2025

**License**: Copyright (C) 2001-2025 LightSys Technology Services. See LICENSE.txt for more information.


## Table of Contents
- [Handling Network Connection](#the-mtsession-library)
  - [Introduction](#introduction)
  - [netConnectTCP()](#netconnecttcp)
  - [netCloseTCP()](#netclosetcp)
  - [fdWrite()](#fdwrite)
  - [fdRead()](#fdread)


## Introduction
The `MTASK` module provides simple and easy TCP/IP connectivity.  It includes many functions, only a few of which are documented below:

- ⚠️ **Warning**: This documentation is incomplete, as many relevant functions are not explained here.  You can help by expanding it.


## netConnectTCP()
```c
pFile netConnectTCP(char* host_name, char* service_name, int flags);
```
This function creates a client socket and connects it to a server on a given TCP service/port and host name.  It takes the following three parameters:
- `host_name`: The host name or ascii string for the host's ip address.
- `service_name`: The name of the service (from `/etc/services`) or its numeric representation as a string.
- `flags`: Normally left 0.

- 📖 **Note**: The `NET_U_NOBLOCK` flag causes the function to return immediately even if the connection is still being established.  Further reads and writes will block until the connection either establishes or fails.

This function returns the connection file descriptor if successful, or `NULL` if an error occurs.


## netCloseTCP()
```c
int netCloseTCP(pFile net_filedesc, int linger_msec, int flags);
```
This function closes a network connection (either a TCP listening, server, or client socket).  It will also optionally waits up to `linger_msec` milliseconds (1/1000 seconds) for any data written to the connection to make it to the other end before performing the close.  If `linger_msec` is set to 0, the connection is aborted (reset).  The linger time can be set to 1000 msec or so if no writes were performed on the connection prior to the close.  If a large amount of writes were performed immediately prior to the close, offering to linger for a few more seconds (perhaps 5 or 10 by specifying 5000 or 10000 msec) can be a good idea.


## fdWrite()
```c
int fdWrite(pFile filedesc, char* buffer, int length, int offset, int flags);
```
This function writes data to an open file descriptor, from a given `buffer` and `length` of data to write.  It also takes an optional seek `offset` and and `flags`, which can be zero or more of:
- `FD_U_NOBLOCK` - If the write can't be performed immediately, don't perform it at all.
- `FD_U_SEEK` - The `offset` value is valid.  Seek to it before writing.  Not allowed for network connections.
- `FD_U_PACKET` - *ALL* of the data specified by `length` in `buffer` must be written.  Normal `write()` semantics in UNIX state that not all data has to be written, and the number of bytes actually written is returned.  Setting this flag makes sure all data is really written before returning.


## fdRead()
```c
int fdRead(pFile filedesc, char* buffer, int maxlen, int offset, int flags);
```
This function works the same as [`fdWrite()`](#fdwrite) except that it reads data instead of writing it.  It takes the same flags as above, except that `FD_U_PACKET` now requires that all of `maxlen` bytes must be read before returning.  This is good for reading a packet of a known length that might be broken up into fragments by the network (TCP/IP has a maximum frame transmission size of about 1450 bytes).
