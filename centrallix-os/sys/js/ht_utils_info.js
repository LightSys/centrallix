// Copyright (C) 2004 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

// Obj info definitions - MUST match those in obj.h
var cx_info_flags = new Object();
cx_info_flags.no_subobj = 1;
cx_info_flags.has_subobj = 2;
cx_info_flags.can_have_subobj = 4;
cx_info_flags.cant_have_subobj = 8;
cx_info_flags.subobj_cnt_known = 16;
cx_info_flags.can_add_attr = 32;
cx_info_flags.cant_add_attr = 64;
cx_info_flags.can_seek_full = 128;
cx_info_flags.can_seek_rewind = 256;
cx_info_flags.cant_seek = 512;
cx_info_flags.can_have_content = 1024;
cx_info_flags.cant_have_content = 2048;
cx_info_flags.has_content = 4096;
cx_info_flags.no_content = 8192;
cx_info_flags.supports_inheritance = 16384;

// Extract flags from string sent from a list with ls__info=1 enabled
function cx_info_extract_flags(a)
    {
    var arr = a.split(":");
    return arr[0];
    }

// Extract num subobjects
function cx_info_extract_cnt(a)
    {
    var arr = a.split(":");
    return arr[1];
    }

// Extract original string
function cx_info_extract_str(a)
    {
    var arr = a.split(":");
    return arr[2];
    }
