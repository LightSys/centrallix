#ifndef GLYPH_H
#define	GLYPH_H

/************************************************************************/
/* Centrallix Application Server System                                 */
/* Centrallix Core                                                      */
/*                                                                      */
/* Copyright (C) 1998-2012 LightSys Technology Services, Inc.           */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation; either version 2 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with this program; if not, write to the Free Software          */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA             */
/* 02111-1307  USA                                                      */
/*                                                                      */
/* A copy of the GNU General Public License has been included in this   */
/* distribution in the file "COPYING".                                  */
/*                                                                      */
/* Module:      glyph.h                                                 */
/* Author:      Israel Fuller                                           */
/* Creation:    October 27, 2025                                        */
/* Description: A simple debug visualizer to make pretty patterns in    */
/*              developer's terminal which can be surprisingly useful   */
/*              for debugging algorithms.                               */
/************************************************************************/

#include <stdlib.h>

/** Uncomment to use glyphs. **/
/** TODO: Israel - Comment this out. **/
// #define ENABLE_GLYPHS

#ifdef ENABLE_GLYPHS
#define glyph_print(s) printf("%s", s);
/*** Initialize a simple debug visualizer to make pretty patterns in the
 *** developer's terminal. Great for when you need to run a long task and
 *** want a super simple way to make sure it's still working.
 *** 
 *** @attention - Relies on storing data in variables in scope, so calling
 ***    glyph() requires a call to glyph_init() previously in the same scope.
 *** 
 *** @param name The symbol name of the visualizer.
 *** @param str The string printed for the visualization.
 *** @param interval The number of invokations of glyph() required to print.
 *** @param flush Whether to flush on output.
 ***/
#define glyph_init(name, str, interval, flush) \
    const char* vis_##name##_str = str; \
    const unsigned int vis_##name##_interval = interval; \
    const bool vis_##name##_flush = flush; \
    unsigned int vis_##name##_i = 0u;

/*** Invoke a visualizer.
 *** 
 *** @param name The name of the visualizer to invoke.
 ***/
#define glyph(name) \
    if (++vis_##name##_i % vis_##name##_interval == 0) \
    { \
	glyph_print(vis_##name##_str); \
    if (vis_##name##_flush) fflush(stdout); \
    }
#else
#define glyph_print(str)
#define glyph_init(name, str, interval, flush)
#define glyph(name)
#endif

#endif /* End of .h file. */
