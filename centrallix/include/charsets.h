#ifndef _CHARSETS_H
#define _CHARSETS_H

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	charsets.c, charsets.h                                  */
/* Author:	Daniel Rothfus                                          */
/* Creation:	June 29, 2011                                           */
/* Description: This module provides utilities for working with the     */
/*              variable character set support built into Centrallix.   */
/************************************************************************/

/** \defgroup charset Character Set Utilities
 \brief This module provides utilities for handling the different character
 set support implemented in Centrallix.
 /{ */

/** \defgroup charset_predefs Predefined Keys for Modules
 \brief A collection of macros that define string constants that are to be 
 valid lookup keys in charsetmap.cfg
 \{ */

/** \brief The string constant for the HTTP generation module. */
#define CHR_MODULE_HTTP "http"
/** \brief The string constant for the MySQL database driver. */
#define CHR_MODULE_MYSQL "mysql"
/** \brief A string constant for the Sybase database driver. */
#define CHR_MODULE_SYBASE "sybase"

/** \} */

/** \brief This is the key in centrallix.conf that specifies the charsetmap.cfg
 file */
#define CHR_CHARSETMAP_FILE_KEY "charsetmap_file"

/** \brief Check for an equivalent name for the current character set.
 
 (See charsetmap.cfg for an introduction to equivalent names.)
 \param name The name of the module/subsystem/library that needs a name for the
 current character set.
 \return This returns a pointer to the name of the charset.  This should never
 return NULL, though it may return an incorrect string if the application is 
 configured incorrectly. */
char* chrGetEquivalentName(char* name);

/** /} */

#endif
