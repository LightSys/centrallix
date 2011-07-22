#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "st_node.h"
#include "hints.h"
#include "cxlib/mtsession.h"
#include "centrallix.h"
#include "cxlib/strtcpy.h"
#include "cxlib/qprintf.h"
#include "cxlib/util.h"
#include <assert.h>
#include <mysql.h>

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Core                                                       */
/*                                                                         */
/* Copyright (C) 1998-2008 LightSys Technology Services, Inc.                */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify        */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation; either version 2 of the License, or        */
/* (at your option) any later version.                                        */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                                */
/*                                                                         */
/* You should have received a copy of the GNU General Public License        */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA                  */
/* 02111-1307  USA                                                        */
/*                                                                        */
/* A copy of the GNU General Public License has been included in this        */
/* distribution in the file "COPYING".                                        */
/*                                                                         */
/* Module:         objdrv_mysql.c                                                 */
/* Author:        Greg Beeley (GB)                                               */
/* Creation:        February 21, 2008                                              */
/* Description:        A MySQL driver for Centrallix.  Eventually this driver        */
/*                should be merged with the Sybase driver in something of        */
/*                an intelligent manner.                                    */
/************************************************************************/

