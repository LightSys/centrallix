#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif
#include "obj.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	obj_datatype.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	April 29, 1999  					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		==> obj_datatype.c implements a unified interface to	*/
/*		some commonly-needed datatype functions.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_datatypes.c,v 1.14 2005/02/26 06:42:39 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_datatypes.c,v $

    $Log: obj_datatypes.c,v $
    Revision 1.14  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.13  2004/12/31 04:38:17  gbeeley
    - a fix and speedup to objCopyData (copy a POD)

    Revision 1.12  2004/08/27 01:28:32  jorupp
     * cleaning up some compile warnings

    Revision 1.11  2004/08/02 14:09:36  mmcgill
    Restructured the rendering process, in anticipation of new deployment methods
    being added in the future. The wgtr module is now the main widget-related
    module, responsible for all non-deployment-specific widget functionality.
    For example, Verifying a widget tree is non-deployment-specific, so the verify
    functions have been moved out of htmlgen and into the wgtr module.
    Changes include:
    *   Creating a new folder, wgtr/, to contain the wgtr module, including all
        wgtr drivers.
    *   Adding wgtr drivers to the widget tree module.
    *   Moving the xxxVerify() functions to the wgtr drivers in the wgtr module.
    *   Requiring all deployment methods (currently only DHTML) to register a
        Render() function with the wgtr module.
    *   Adding wgtrRender(), to abstract the details of the rendering process
        from the caller. Given a widget tree, a string representing the deployment
        method to use ("DHTML" for now), and the additional args for the rendering
        function, wgtrRender() looks up the appropriate function for the specified
        deployment method and calls it.
    *   Added xxxNew() functions to each wgtr driver, to be called when a new node
        is being created. This is primarily to allow widget drivers to declare
        the interfaces their widgets support when they are instantiated, but other
        initialization tasks can go there as well.

    Also in this commit:
    *   Fixed a typo in the inclusion guard for iface.h (most embarrasing)
    *   Fixed an overflow in objCopyData() in obj_datatypes.c that stomped on
        other stack variables.
    *   Updated net_http.c to call wgtrRender instead of htrRender(). Net drivers
        can now be completely insulated from the deployment method by the wgtr
        module.

    Revision 1.10  2004/05/04 18:23:00  gbeeley
    - Adding DATA_T_BINARY data type for counted (non-zero-terminated)
      strings of data.

    Revision 1.9  2004/02/24 20:11:00  gbeeley
    - fixing some date/time related problems
    - efficiency improvement for net_http allowing browser to actually
      cache .js files and images.

    Revision 1.8  2002/08/13 14:11:36  lkehresman
    * silenced some more unnecessarily verbose output in the makefile
    * removed an unused variable in obj_datatypes.c

    Revision 1.7  2002/08/10 02:00:15  gbeeley
    Starting work on better date formatting and internationalization of
    date formats.  Main change that has taken effect now is that the
    presence of "II" at the beginning of a date format string causes the
    server to interpret something like 01/02/2002 as Feb 1st (international)
    instead of Jan 2nd (united states).

    Revision 1.6  2002/08/01 08:25:21  mattphillips
    Include sys/time.h if configure tells us that struct tm is defined there.

    Revision 1.5  2002/06/19 23:29:34  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.4  2002/04/25 04:26:07  gbeeley
    Basic overhaul of objdrv_sybase to fix some security issues, improve
    robustness with key data in particular, and so forth.  Added a new
    flag to the objDataToString functions: DATA_F_SYBQUOTE, which quotes
    strings like Sybase wants them quoted (using a pair of quote marks to
    escape a lone quote mark).

    Revision 1.3  2001/10/02 15:45:26  gbeeley
    Oops - fixed the adding of ".0" to whole integer double values so that
    the routine does NOT overwrite the trailing digit ;)

    Revision 1.2  2001/10/02 15:43:12  gbeeley
    Updated data type conversion functions.  Converting to string now can
    properly escape quotes in the string.  Converting double to string now
    formats the double a bit better.

    Revision 1.1.1.1  2001/08/13 18:00:58  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:53  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:59  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** Some date-time constants ***/
char* obj_short_months[] = {"Jan","Feb","Mar","Apr","May","Jun",
			    "Jul","Aug","Sep","Oct","Nov","Dec"};

char* obj_long_months[] =  {"January","February","March","April","May",
			    "June","July","August","September","October",
			    "November","December"};

unsigned char obj_month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

char* obj_short_week[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

char* obj_long_week[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday",
			 "Friday","Saturday"};

/*** date format:
 ***   DDDD = long day-of-week name	NOT YET IMPL.
 ***   DDD = short day-of-week abbrev.	NOT YET IMPL.
 ***   dd = 2-digit day of month
 ***   ddd = day of month plus cardinality '1st','2nd','3rd', etc.
 ***   MMMM = full (long) month name
 ***   MMM = short month abbreviation
 ***   MM = 2-digit month of year
 ***   yy = 2-digit year (bad)
 ***   yyyy = 4-digit year (good)
 ***   HH = hour in 24-hour format
 ***   hh = hour in 12-hour format, appends AM/PM also
 ***   mm = minutes
 ***   ss = seconds
 ***   II = at beginning of date format, indicates that the month and day,
 ***        if ambiguous, should be interpreted as dd/mm (international)
 ***        instead of the default mm/dd (United States).  Does not influence
 ***        how dates are printed, just how they are interpreted.
 ***   Lm[], LM[], Lw[], LW[] = specify the short (m,w) and long (M,W) names
 ***        for months and weekdays that will be used in interpretation and
 ***        in generation.  U.S. english month/week names are always accepted
 ***        on input.  List of names should be comma-separated inside the [].
 ***
 ***   example:
 ***
 ***        "dd-MMM-yyyy HH:mm:ss Lm[Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec]"
 ***/
char* obj_default_date_fmt = "dd MMM yyyy HH:mm";

/*** money format:
 ***   # = optional digit unless after the '.' or first before '.'
 ***   0 = mandatory digit, no zero suppression
 ***   , = insert a comma
 ***   . = decimal point (only one allowed)
 ***   $ = dollar sign
 ***   + = mandatory sign, whether + or - (if 0, +)
 ***   - = optional sign, space if + or 0
 ***   ^ = if last digit, round it (trunc is default).  Otherwise, like '0'.  NOT YET IMPL.
 ***  () = surround # with () if it is negative.
 ***  [] = surround # with () if it is positive.
 ***     = (space) optional digit, but put space in its place if suppressing 0's.
 ***   * = (asterisk) optional digit, put asterisk in its place if suppressing 0s.
 ***/
char* obj_default_money_fmt = "$0.00";

/*** NULL printing format:
 ***   This format is just the string that gets printed in place of a NULL
 ***   value and is generally settable by the 'nfmt' session variable.
 ***/
char* obj_default_null_fmt = "NULL";


/*** obj_internal_ParseDateLang - looks up a list of language internationalization
 *** strings inside the date format.  WARNING - modifies the "srcptr" data in
 *** place.
 ***/
int
obj_internal_ParseDateLang(char *dest_array[], int dest_array_len, char* srcptr, char* searchstart, char* searchend)
    {
    char* ptr;
    char* endptr;
    char* enditemptr;
    int n_items;

	/** No format string? **/
	if (!srcptr)
	    {
	    return -1;
	    }

	/** Locate a possible list of language strings **/
	ptr = strstr(srcptr, searchstart);
	if (!ptr || !strstr(ptr+strlen(searchstart),searchend)) return -1;

	/** Ok, got it.  Now start parsing 'em **/
	n_items = 0;
	ptr = ptr + strlen(searchstart);
	endptr = strstr(ptr,searchend);
	while(1)
	    {
	    /** Find the end of the current item. **/
	    enditemptr = strpbrk(ptr,",[]|;");
	    if (!enditemptr)
		{
		/* a work in progress... */
		}
	    }

    return 0;
    }


/*** obj_internal_SetDateLang - set date formatting language stuff from a
 *** simple array rather than from a text string.
 ***/
int
obj_internal_SetDateLang(char* dest_array[], int array_cnt, char* src_array[])
    {
    return 0;
    }


/*** obj_internal_FormatDate - formats a date/time value to the given string
 *** value.
 ***/
int
obj_internal_FormatDate(pDateTime dt, char* str)
    {
    char* fmt;
    char* myfmt;
    int append_ampm = 0;
    char *Lw[7], *LW[7], *Lm[12], *LM[12];
    char *Lwptr, *LWptr, *Lmptr, *LMptr;

    	/** Get the current date format. **/
	fmt = mssGetParam("dfmt");
	if (!fmt) fmt = obj_default_date_fmt;
	myfmt = nmSysStrdup(fmt);

	/** Lookup language internationalization in the format. **/
	Lwptr = strstr(myfmt,"Lw[");
	LWptr = strstr(myfmt,"LW[");
	Lmptr = strstr(myfmt,"Lm[");
	LMptr = strstr(myfmt,"LM[");
	if (obj_internal_ParseDateLang(Lw, 7, Lwptr, "Lw[", "]") < 0)
	    obj_internal_SetDateLang(Lw, 7, obj_short_week);
	if (obj_internal_ParseDateLang(LW, 7, LWptr, "LW[", "]") < 0)
	    obj_internal_SetDateLang(LW, 7, obj_long_week);
	if (obj_internal_ParseDateLang(Lm, 12, Lmptr, "Lm[", "]") < 0)
	    obj_internal_SetDateLang(Lm, 12, obj_short_months);
	if (obj_internal_ParseDateLang(LM, 12, LMptr, "LM[", "]") < 0)
	    obj_internal_SetDateLang(LM, 12, obj_long_months);

	/** Step through the fmt string one piece at a time **/
	while(*fmt) switch(*fmt)
	    {
	    /** Day of Week **/
	    case 'D':
	        fmt++;
	        break;

	    /** Day of Month **/
	    case 'd':
	        if (fmt[1] == 'd' && fmt[2] == 'd')
		    {
		    if (dt->Part.Day == 0 || dt->Part.Day == 20 || dt->Part.Day == 30)
		        sprintf(str,"%dst",dt->Part.Day+1);
		    else if (dt->Part.Day == 1 || dt->Part.Day == 21)
		        sprintf(str,"%dnd",dt->Part.Day+1);
		    else if (dt->Part.Day == 2 || dt->Part.Day == 22)
		        sprintf(str,"%drd",dt->Part.Day+1);
		    else
		        sprintf(str,"%dth",dt->Part.Day+1);
		    str = memchr(str,'\0',16);
		    fmt+=2;
		    }
		else if (fmt[1] == 'd')
		    {
		    sprintf(str,"%2.2d",dt->Part.Day+1);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		break;

	    /** Month name or number **/
	    case 'M':
	        if (fmt[1] == 'M' && fmt[2] == 'M' && fmt[3] == 'M')
		    {
		    strcpy(str,obj_long_months[dt->Part.Month]);
		    str = memchr(str,'\0',16);
		    fmt+=3;
		    }
		else if (fmt[1] == 'M' && fmt[2] == 'M')
		    {
		    strcpy(str,obj_short_months[dt->Part.Month]);
		    str+=3;
		    fmt+=2;
		    }
		else if (fmt[1] == 'M')
		    {
		    sprintf(str,"%2.2d",dt->Part.Month+1);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		break;

	    case 'y':
	        if (fmt[1] == 'y' && fmt[2] == 'y' && fmt[3] == 'y')
		    {
		    sprintf(str,"%4.4d",dt->Part.Year+1900);
		    str+=4;
		    fmt+=3;
		    }
		else if (fmt[1] == 'y')
		    {
		    sprintf(str,"%2.2d",dt->Part.Year % 100);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		break;

	    case 'H':
	        if (fmt[1] == 'H')
		    {
		    sprintf(str,"%2.2d",dt->Part.Hour);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		append_ampm = 0;
		break;

	    case 'h':
	        if (fmt[1] == 'h')
		    {
		    sprintf(str,"%2.2d",((dt->Part.Hour%12)==0)?12:(dt->Part.Hour%12));
		    str+=2;
		    fmt++;
		    }
		fmt++;
	        append_ampm = 1;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(str,(dt->Part.Hour >= 12)?"PM":"AM");
		    str+=2;
		    }
		break;

	    case 'm':
	        if (fmt[1] == 'm')
		    {
		    sprintf(str,"%2.2d",dt->Part.Minute);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(str,(dt->Part.Hour >= 12)?"PM":"AM");
		    str+=2;
		    }
		break;

	    case 's':
	        if (fmt[1] == 's')
		    {
		    sprintf(str,"%2.2d",dt->Part.Second);
		    str+=2;
		    fmt++;
		    }
		fmt++;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(str,(dt->Part.Hour >= 12)?"PM":"AM");
		    str+=2;
		    }
		break;

	    case 'I':
		/** Data-entry date-order internationalization **/
		if (fmt[1] == 'I')
		    {
		    fmt++;
		    }
		fmt++;
		break;

	    case 'L':
		/** Language internationalization **/
		if ((fmt[1] == 'm' || fmt[1] == 'M' || fmt[1] == 'w' || fmt[1] == 'W') && fmt[2] == '[' && strchr(fmt,']'))
		    {
		    while(*fmt != ']') fmt++;
		    }
		break;

	    default:
	        *(str++) = *(fmt++);
		break;
	    }
	*str = '\0';

	nmSysFree(myfmt);

    return 0;
    }


/*** obj_internal_FormatMoney - format a money data type into a string
 *** using the default format or the one stored in the session.
 ***/
int
obj_internal_FormatMoney(pMoneyType m, char* str)
    {
    char* fmt;
    char* ptr;
    int n_whole_digits = 0;
    int print_whole;
    int print_fract;
    int in_decimal_part = 0;
    int suppressing_zeros = 1;
    int automatic_sign = 1;
    int tens_multiplier = 1;
    int d;
    char* start_fmt;
    int orig_print_whole;

    	/** Get the format **/
	fmt = mssGetParam("mfmt");
	if (!fmt) fmt = obj_default_money_fmt;
	start_fmt = fmt;

	/** Determine number of explicitly-specified whole part digits **/
	ptr = fmt;
	while(*ptr && *ptr != '.')
	    {
	    if (*ptr == '0' || *ptr == '#' || *ptr == '^' || *ptr == ' ' || *ptr == '*') 
	        {
		n_whole_digits++;
		tens_multiplier *= 10;
		}
	    ptr++;
	    }
	if (tens_multiplier > 0) tens_multiplier /= 10;

	/** Any reasons to omit the automatic-sign? **/
	if (strpbrk(fmt,"+-()[]")) automatic_sign = 0;

	/** Determine the 'print' version of whole/fraction parts **/
	if (m->WholePart > 0 || m->FractionPart == 0)
	    {
	    print_whole = m->WholePart;
	    print_fract = m->FractionPart;
	    }
	else
	    {
	    print_whole = m->WholePart + 1;
	    print_fract = 10000 - m->FractionPart;
	    }
	orig_print_whole = m->WholePart;
	if (print_whole < 0) print_whole = -print_whole;

	/** Ok, start generating the thing. **/
	while(*fmt) 
	    {
            if (automatic_sign)
                {
                automatic_sign = 0;
                if (orig_print_whole>=0) *(str++) = ' ';
                else *(str++) = '-';
                }
	    switch(*fmt)
                {
                case '$':
                    *(str++) = '$';
                    break;
   
    		case ' ':
		case '*':
                case '0':
                case '^':
                case '#':
		    if (in_decimal_part)
		        {
			d = (print_fract/tens_multiplier)%10;
			tens_multiplier /= 10;
			}
		    else
		        {
			d = print_whole/tens_multiplier;
			print_whole -= d*tens_multiplier;
			tens_multiplier /= 10;
			}
		    if (d != 0 || *fmt == '0' || *fmt == '^') suppressing_zeros = 0;
		    if (suppressing_zeros)
		        {
			if (*fmt == ' ') *(str++) = ' ';
			else if (*fmt == '*') *(str++) = '*';
			}
		    else
		        {
			sprintf(str,"%d",d);
			str = memchr(str,'\0',24);
			}
		    break;

                case ',':
		    if (!suppressing_zeros) 
		        {
			*(str++) = ',';
			}
		    else
		        {
			/** Replace comma with space if no digits and surrounded by placeholders **/
			if (!((start_fmt != fmt && fmt[-1] == '#') || fmt[1] == '#'))
			    {
			    *(str++) = ' ';
			    }
			}
		    break;

                case '.':
		    if (print_whole != 0)
		        {
			sprintf(str,"%d",print_whole);
			str = memchr(str,'\0',24);
			}
                    in_decimal_part = 1;
		    suppressing_zeros = 0;
		    tens_multiplier = 1000;
		    *(str++) = '.';
                    break;
    
                case '-':
		    if (orig_print_whole >= 0) *(str++) = ' ';
		    else *(str++) = '-';
		    break;

                case '+':
		    if (orig_print_whole >= 0) *(str++) = '+';
		    else *(str++) = '-';
		    break;

                case '[':
		    if (orig_print_whole >= 0) *(str++) = '(';
		    else *(str++) = ' ';
		    break;

                case '(':
		    if (orig_print_whole < 0) *(str++) = '(';
		    else *(str++) = ' ';
		    break;

                case ']':
		    if (orig_print_whole >= 0) *(str++) = ')';
		    else *(str++) = ' ';
		    break;

                case ')':
		    if (orig_print_whole < 0) *(str++) = ')';
		    else *(str++) = ' ';
		    break;

                default:
                    break;
                }
	    fmt++;
	    }
	*str = '\0';

    return 0;
    }


/*** objDataToString - concatenates the string representation of the given
 *** data type onto the end of the given XString.
 ***/
int 
objDataToString(pXString dest, int data_type, void* data_ptr, int flags)
    {
    char sbuf[80];
    int i;
    pMoneyType m;
    pDateTime d;
    pIntVec iv;
    pStringVec sv;
    char* tmpstr;

    	/** Check for a NULL. **/
	if (data_ptr == NULL)
	    {
	    if (flags & DATA_F_QUOTED)
	        xsConcatenate(dest, " NULL ", 6);
	    else
	        xsConcatenate(dest, "NULL", 4);
	    return 0;
	    }

	/** Print the thing to a string. **/
    	switch(data_type)
	    {
	    case DATA_T_INTEGER:
	        if (flags & DATA_F_QUOTED)
	            sprintf(sbuf," %d ",*(int*)data_ptr);
		else
	            sprintf(sbuf,"%d",*(int*)data_ptr);
		xsConcatenate(dest, sbuf, -1);
		break;

	    case DATA_T_STRING:
		tmpstr = objDataToStringTmp(DATA_T_STRING, data_ptr, flags);
	        xsConcatenate(dest, tmpstr, -1);
		break;

	    case DATA_T_BINARY:
		tmpstr = objDataToStringTmp(DATA_T_BINARY, data_ptr, flags);
	        xsConcatenate(dest, tmpstr, -1);
		break;

	    case DATA_T_DOUBLE:
	        if (flags & DATA_F_QUOTED)
	            sprintf(sbuf," %f ", *(double*)data_ptr);
		else
	            sprintf(sbuf,"%f", *(double*)data_ptr);
		xsConcatenate(dest, sbuf, -1);
		break;

	    case DATA_T_MONEY:
	        m = (pMoneyType)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		obj_internal_FormatMoney(m, sbuf + strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		xsConcatenate(dest, sbuf, -1);
		break;

	    case DATA_T_DATETIME:
	        d = (pDateTime)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " '");
		obj_internal_FormatDate(d, sbuf + strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, "' ");
		xsConcatenate(dest, sbuf, -1);
		break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr;
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest," (",2);
		for(i=0;i<iv->nIntegers;i++)
		    {
		    sprintf(sbuf,"%s%d", (i==0)?"":",", iv->Integers[i]);
		    xsConcatenate(dest, sbuf, -1);
		    }
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest,") ",2);
		break;
	        
	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr;
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest," (",2);
		for(i=0;i<sv->nStrings;i++)
		    {
		    sprintf(sbuf,"%s\"%s\"", (i==0)?"":",", sv->Strings[i]);
		    xsConcatenate(dest, sbuf, -1);
		    }
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest,") ",2);
		break;
	    }

    return 0;
    }


/*** objDataToInteger - returns the given data type converted to an integer.
 *** the integer may not always intuitively represent the string.
 ***/
int 
objDataToInteger(int data_type, void* data_ptr, char* format)
    {
    int v;
    pMoneyType m;
    pStringVec sv;
    pIntVec iv;
    int base;
    char* uptr;

    	/** NULL? return 0 **/
	if (data_ptr == NULL) return 0;

	/** Pick the right type. **/
	switch(data_type)
	    {
	    case DATA_T_INTEGER: 
	        v = *(int*)data_ptr; break;

	    case DATA_T_STRING: 
	        if (format == NULL) base = 0;
		else base = strtol(format,NULL,0);
	        v = strtol((char*)data_ptr,&uptr,base); 
		if (format && strchr(format,'U') && uptr)
		    {
		    switch (*uptr)
		        {
			case 'K':
			    v = v*1024;
			    break;
			case 'M':
			    v = v*1024*1024;
			    break;
			}
		    }
		if (format && strchr(format,'u') && uptr)
		    {
		    switch (*uptr)
		        {
			case 'K':
			    v = v*1000;
			    break;
			case 'M':
			    v = v*1000*1000;
			    break;
			}
		    }
		break;

	    case DATA_T_DATETIME: 
	        v = ((pDateTime)data_ptr)->Value; break;

	    case DATA_T_DOUBLE: 
	        v = (int)(*(double*)data_ptr); break;

	    case DATA_T_MONEY: 
	        m = (pMoneyType)data_ptr;
                if (m->FractionPart==0 || m->WholePart>=0)
		    v = m->WholePart;
		else
		    v = m->WholePart - 1;
		break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr;
		if (iv->nIntegers == 0) v = 0; else v = iv->Integers[0];
		break;

	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr;
		if (sv->nStrings == 0) v = 0; else v = strtol(sv->Strings[0],NULL,0);
		break;
	    
	    default:
		mssError(1,"OBJ","Warning: could not convert data type %d to integer", data_type);
		return 0;
	    }

    return v;
    }


/*** objDataToDouble - convert data type to a double-precision floating point
 *** type.
 ***/
double
objDataToDouble(int data_type, void* data_ptr)
    {
    double v;
    pMoneyType m;

    	switch(data_type)
	    {
	    case DATA_T_INTEGER: v = *(int*)data_ptr; break;
	    case DATA_T_STRING: v = strtod((char*)data_ptr, NULL); break;
	    case DATA_T_MONEY: m = (pMoneyType)data_ptr; v = m->WholePart + (m->FractionPart/10000.0); break;
	    case DATA_T_DOUBLE: v = *(double*)data_ptr; break;
	    default: v = 0.0; break;
	    }

    return v;
    }


/*** objDataToStringTmp - writes the string representation of the given
 *** data type to a temporary string, or in some cases, returns the string
 *** buffer from the data type itself.  In no case should the calling routine
 *** make any assumptions about the buffer - it must be copied to another 
 *** location before any other call to this function, or any call which yields
 *** MTASK control since another thread could call this.
 ***/
char* 
objDataToStringTmp(int data_type, void* data_ptr, int flags)
    {
    static char sbuf[80];
    static char* alloc_str = NULL;
    static int alloc_len = 0;
    pDateTime d;
    pMoneyType m;
    pStringVec sv;
    pIntVec iv;
    char* ptr = sbuf;
    int new_len,i;
    char* tmpptr;
    char quote = (flags & DATA_F_SINGLE)?'\'':'"';
    pBinary bn;
    char* str_ptr = data_ptr;
    int len_limit = 0x7FFFFFFF;

    	/** Check for a NULL. **/
	if (data_ptr == NULL)
	    {
	    if (flags & DATA_F_QUOTED)
	        strcpy(sbuf, " NULL ");
	    else
	        strcpy(sbuf, "NULL");
	    return sbuf;
	    }

    	/** Pick the data type to convert... **/
	switch(data_type)
	    {
	    case DATA_T_INTEGER:
	        if (flags & DATA_F_QUOTED)
	            sprintf(sbuf," %d ",*(int*)data_ptr);
		else
	            sprintf(sbuf,"%d",*(int*)data_ptr);
		break;

	    case DATA_T_BINARY:
		bn = (pBinary)data_ptr;
		if (!(flags & DATA_F_QUOTED))
		    {
		    new_len = bn->Size+1;
		    if (!alloc_str || new_len > alloc_len)
			{
			if (alloc_str) nmSysFree(alloc_str);
			alloc_str = (char*)nmSysMalloc(new_len);
			}
		    ptr = alloc_str;
		    memcpy(ptr, (char*)(bn->Data), bn->Size);
		    ptr[bn->Size] = '\0';
		    break;
		    }
		else
		    {
		    str_ptr = bn->Data;
		    len_limit = bn->Size;
		    }
		/* fallthrough */
	    case DATA_T_STRING:
	        if (flags & DATA_F_QUOTED)
		    {
		    if (len_limit < 0x7FFFFFFF)
			new_len = len_limit*2+5;
		    else
			new_len = strlen((char*)str_ptr)*2 + 5;
		    if (!alloc_str || new_len > alloc_len)
		        {
		        if (alloc_str) nmSysFree(alloc_str);
		        alloc_str = (char*)nmSysMalloc(new_len);
			}
		    tmpptr = (char*)str_ptr;
		    ptr = alloc_str;
		    *(ptr++) = ' ';
		    *(ptr++) = quote;
		    while(*tmpptr && (tmpptr - str_ptr) < len_limit)
		        {
			if (flags & DATA_F_CONVSPECIAL)
			    {
			    if (*tmpptr == '\t')
				{
				tmpptr++;
				*(ptr++) = '\\';
				*(ptr++) = 't';
				continue;
				}
			    else if (*tmpptr == '\n')
				{
				tmpptr++;
				*(ptr++) = '\\';
				*(ptr++) = 'n';
				continue;
				}
			    else if (*tmpptr == '\r')
				{
				tmpptr++;
				*(ptr++) = '\\';
				*(ptr++) = 'r';
				continue;
				}
			    }
			if (flags & DATA_F_SYBQUOTE)
			    {
			    if (*tmpptr == quote) 
				*(ptr++) = quote;
			    }
			else
			    {
			    if (*tmpptr == quote || *tmpptr == '\\')
				*(ptr++) = '\\';
			    }
			*(ptr++) = *(tmpptr++);
			}
		    *(ptr++) = quote;
		    *(ptr++) = ' ';
		    *(ptr) = '\0';
		    ptr = alloc_str;
		    }
		else
		    {
		    ptr = (char*)data_ptr;
		    }
		break;

	    case DATA_T_DOUBLE:
	        /** sbuf is 80 chars, plenty for our purposes here. **/
	        if (flags & DATA_F_QUOTED)
	            sprintf(sbuf," %.15g ", *(double*)data_ptr);
		else
	            sprintf(sbuf,"%.15g", *(double*)data_ptr);
		ptr = sbuf+strlen(sbuf)-1;

		/** Do zero suppression, but only if decimal pt present. **/
		if (strchr(sbuf,'.'))
		    {
		    /** Eliminate extraneous zeros right of decimal **/
		    while (ptr > sbuf && *ptr == '0' && ptr[-1] != '.') *(ptr--) = '\0';
		    }
		else
		    {
		    /** Otherwise, add decimal point for 'proper form' **/
		    strcpy(ptr+1,".0");
		    }
		ptr = sbuf;
		break;

	    case DATA_T_MONEY:
	        m = (pMoneyType)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		obj_internal_FormatMoney(m, sbuf + strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		break;

	    case DATA_T_DATETIME:
	        d = (pDateTime)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " '");
		obj_internal_FormatDate(d, sbuf + strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, "' ");
		break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr;
	        if (flags & DATA_F_QUOTED) 
		    {
		    strcpy(ptr," (");
		    ptr += 2;
		    }
		for(i=0;i<iv->nIntegers;i++)
		    {
		    sprintf(ptr,"%s%d", (i==0)?"":",", iv->Integers[i]);
		    ptr += strlen(ptr);
		    }
	        if (flags & DATA_F_QUOTED) 
		    {
		    strcpy(ptr,") ");
		    ptr += 2;
		    }
		ptr = sbuf;
		break;
	        
	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr;
	        if (flags & DATA_F_QUOTED) 
		    {
		    strcpy(ptr," (");
		    ptr += 2;
		    }
		for(i=0;i<sv->nStrings;i++)
		    {
		    sprintf(ptr,"%s\"%s\"", (i==0)?"":",", sv->Strings[i]);
		    ptr += strlen(ptr);
		    }
	        if (flags & DATA_F_QUOTED) 
		    {
		    strcpy(ptr,") ");
		    ptr += 2;
		    }
		ptr = sbuf;
		break;
	    }

    return ptr;
    }


/*** objDataToDateTime - convert a data type (usually string) to a date-time 
 *** value.  Currently only string is accepted.
 ***/
int
objDataToDateTime(int data_type, void* data_ptr, pDateTime dt, char* format)
    {
    int got_hr=-1, got_min=-1, got_sec=-1;
    int got_day=-1, got_yr=-1, got_mo=-1;
    int last_num;
    char* startptr;
    char* endptr;
    char* origptr;
    int i;
    struct tm *t;
    time_t int_time;
    int reversed_day=0;

    	/** Only accept string... **/
	if (data_type != DATA_T_STRING) return -1;

	/** Check for reversed days (ie, international format, dd-mm-yyyy)
	 ** Default is to interpret as mm-dd-yyyy (U.S. format)
	 **/
	if (format)
	    {
	    if (!strncmp(format,"II",2)) reversed_day = 1;
	    }

	startptr = (char*)data_ptr;
	origptr = startptr;
	while(*startptr)
	    {
	    /** Skip whitespace **/
	    while(*startptr == ' ' || *startptr == '\t' || *startptr == ',') startptr++;

	    /** Try to convert a number. **/
	    last_num = strtol(startptr, &endptr, 10);
	    if (endptr != startptr)
	        {
		/** Got a number **/
		if (*endptr == ':')
		    {
		    /** time field.  Check which ones we have. **/
		    if (got_hr == -1) got_hr = last_num;
		    else if (got_min == -1) got_min = last_num;
		    endptr++;
		    }
		else if (*endptr == '/' || *endptr == '-')
		    {
		    /** Date field.  Check. **/
		    if (reversed_day)
		        {
		        if (got_day == -1) got_day = last_num;
		        else if (got_mo == -1) got_mo = last_num-1;
			}
		    else
		        {
		        if (got_mo == -1) got_mo = last_num-1;
		        else if (got_day == -1) got_day = last_num;
			}
		    endptr++;
		    }
		else
		    {
		    /** End-of-string -or- space-separated date/time **/
		    if (startptr != origptr && startptr[-1] == ':')
		        {
			/** For seconds in '12:00:01' or minutes in '12:00' **/
			if (got_min == -1) got_min = last_num;
			if (got_sec == -1) got_sec = last_num;
			}
		    else if (startptr != origptr && (startptr[-1] == '/' || startptr[-1] == '-'))
		        {
			/** For year in '1/1/1999' or day in '1/1' or '1/1 12pm 1999' **/
			if (got_day == -1) got_day = last_num;
			if (got_yr == -1) got_yr = last_num;
			}
		    else if (!strncasecmp(endptr,"AM",2) || !strncasecmp(endptr," AM",3) ||
		             !strncasecmp(endptr,"PM",2) || !strncasecmp(endptr," PM",3))
		        {
			/** For the case <hour> PM or <hour>PM when min/sec are missing **/
			if (got_hr == -1) got_hr = last_num;
			}
		    else if (got_mo != -1 && got_yr == -1 && got_day == -1)
		        {
			/** For the case 'Jan 1 1999', when just parsed 'Jan'. **/
			got_day = last_num;
			}
		    else if (startptr != origptr && startptr[-1] == '.')
		        {
			/** Milliseconds as in 12:00:01.000 -- just ignore them **/
			}
		    else if (got_mo != -1 && got_day != -1 && got_yr == -1)
		        {
			/** For the case 'Jan 1 1999', when just parsed 'Jan' and '1'. **/
			got_yr = last_num;
			}
		    else if (got_mo == -1 && got_yr == -1 && got_day == -1)
		        {
			/** For the case '1 Jan 1999' **/
			got_day = last_num;
			}
		    }
		}
	    else
	        {
		/** No number.  Check for AM/PM, or month name, or weekday, or timezone **/
		if (got_mo == -1 || got_day == -1)
		    {
		    for(i=0;i<12;i++)
		        {
			if (!strncasecmp(startptr,obj_long_months[i],strlen(obj_long_months[i])))
			    {
			    endptr = startptr + strlen(obj_long_months[i]);
			    if (got_mo != -1) got_day = got_mo;
			    got_mo = i;
			    break;
			    }
			if (!strncasecmp(startptr,obj_short_months[i], 3))
			    {
			    endptr = startptr+3;
			    if (got_mo != -1) got_day = got_mo;
			    got_mo = i;
			    break;
			    }
			}
		    }
		
		/** Still didn't find anything? **/
		if (endptr == startptr)
		    {
		    if (!strncasecmp(startptr,"AM",2) && got_hr >= 0)
		        {
			endptr = startptr + 2;
			if (got_hr == 12) got_hr = 0;
			}
		    else if (!strncasecmp(startptr,"PM",2) && got_hr >= 0)
		        {
			endptr = startptr + 2;
			if (got_hr < 12) got_hr += 12;
			}
		    else if (!strncasecmp(startptr,"AM",2) && got_hr == -1 && got_day != -1 && got_mo == -1)
		        {
			/** For the case 12 pm 1/1/99 **/
			got_hr = got_day;
			got_day = -1;
			if (got_hr == 12) got_hr = 0;
			endptr = startptr + 2;
			}
		    else if (!strncasecmp(startptr,"PM",2) && got_hr == -1 && got_day != -1 && got_mo == -1)
		        {
			/** For the case 12 pm 1/1/99 **/
			got_hr = got_day;
			got_day = -1;
			if (got_hr < 12) got_hr += 12;
			endptr = startptr + 2;
			}
		    }

		/** Still nothing?  Ignore it if so. **/
		if (startptr == endptr) 
		    {
		    while((*endptr >= 'A' && *endptr <= 'Z') || (*endptr >= 'a' && *endptr <= 'z')) 
			endptr++;
		    }

		/** Not even an alpha item? **/
		if (startptr == endptr && *startptr)
		    {
		    endptr++;
		    }
		}

	    /** Next item. **/
	    startptr = endptr;
	    }

	/** Get the current date/time to fill in some (possibly) blank values **/
	int_time = time(NULL);
	t = localtime(&int_time);
	if (got_yr == -1) got_yr = t->tm_year;
	if (got_day == -1) got_day = t->tm_mday + 1;
	if (got_mo == -1) got_mo = t->tm_mon;

	/** Fill in hour/min/sec - set to midnight, or set min/sec to 0 **/
	if (got_hr == -1) got_hr = 0;
	if (got_min == -1) got_min = 0;
	if (got_sec == -1) got_sec = 0;

	/** Fix the values of the numbers to the internal representation. **/
	/** First, the year. **/
	if (got_yr < 5) 
	    dt->Part.Year = got_yr + 100;
	else if (got_yr < 100)
	    dt->Part.Year = got_yr + 0;
	else if (got_yr >= 1900)
	    dt->Part.Year = got_yr - 1900;

	/** Next, month and day **/
	dt->Part.Month = got_mo;
	dt->Part.Day = got_day - 1;

	/** Hour/minute/second **/
	dt->Part.Hour = got_hr;
	dt->Part.Minute = got_min;
	dt->Part.Second = got_sec;

    return 0;
    }


/*** objDataToMoney - convert a data type to a money data type.
 ***/
int
objDataToMoney(int data_type, void* data_ptr, pMoneyType m)
    {
    char* ptr;
    char* endptr;
    char* endptr2;
    double dbl;

    	/** Select the correct type. **/
	switch(data_type)
	    {
	    case DATA_T_STRING:
	        ptr = (char*)data_ptr;
		m->FractionPart = 0;
		m->WholePart = strtol(ptr, &endptr, 10);
		if (endptr != ptr && *endptr == '.') m->FractionPart = strtol(endptr+1, &endptr2, 10)*100;
		if (endptr2 == endptr+2) m->FractionPart *= 10;
		if (m->WholePart < 0 && m->FractionPart != 0)
		    {
		    m->WholePart--;
		    m->FractionPart = 10000 - m->FractionPart;
		    }
		break;
	    
	    case DATA_T_DOUBLE:
	        dbl = *(double*)data_ptr + 0.000001;
	        m->WholePart = floor(dbl);
		m->FractionPart = (dbl - m->WholePart)*10000;
		break;
	
	    case DATA_T_INTEGER:
	        m->WholePart = *(int*)data_ptr;
		m->FractionPart = 0;
		break;

	    case DATA_T_MONEY:
	        m->WholePart = ((pMoneyType)data_ptr)->WholePart;
		m->FractionPart = ((pMoneyType)data_ptr)->FractionPart;
		break;
	    }

    return 0;
    }


/*** objDataCompare - compares two given datatypes of possibly different kinds
 *** and returns -1, 0, or 1 depending on whether the first is less than, equal to,
 *** or greater than the second.  Returns -2 if the comparison is invalid or a
 *** conversion can't be done.
 ***/
int 
objDataCompare(int data_type_1, void* data_ptr_1, int data_type_2, void* data_ptr_2)
    {
    int cmp_direction = 1;
    int cmp_value = 0;
    int err = 0, i;
    void* tmpptr;
    int tmptype;
    pMoneyType m;
    pDateTime dt;
    pStringVec sv,sv2;
    pIntVec iv,iv2;
    int intval;
    char* strval;
    DateTime dt_v;
    MoneyType m_v;
    double dblval;

    	/** Need to transpose v1 and v2 to simplify? **/
	if ((data_type_1 != DATA_T_INTEGER && data_type_2 == DATA_T_INTEGER) ||
	    (data_type_1 != DATA_T_STRING && data_type_2 == DATA_T_STRING) ||
	    (data_type_1 != DATA_T_DATETIME && data_type_2 == DATA_T_DATETIME) ||
	    (data_type_1 != DATA_T_MONEY && data_type_2 == DATA_T_MONEY) ||
	    (data_type_1 != DATA_T_DOUBLE && data_type_2 == DATA_T_DOUBLE) ||
	    (data_type_1 != DATA_T_INTVEC && data_type_2 == DATA_T_INTVEC) ||
	    (data_type_1 != DATA_T_STRINGVEC && data_type_2 == DATA_T_STRINGVEC))
	    {
	    tmptype = data_type_1;
	    tmpptr = data_ptr_1;
	    data_type_1 = data_type_2;
	    data_ptr_1 = data_ptr_2;
	    data_type_2 = tmptype;
	    data_ptr_2 = tmpptr;
	    cmp_direction = -cmp_direction;
	    }

	/** Choose cmp based on first data type, then second. **/
	switch(data_type_1)
	    {
	    case DATA_T_INTEGER:
	        /** Compare integer... to what? **/
		intval = *(int*)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_INTEGER:
		        cmp_value = intval - *(int*)data_ptr_2;
			break;

		    case DATA_T_STRING:
		        cmp_value = intval - strtol((char*)data_ptr_2, NULL, 0);
			break;

		    case DATA_T_DATETIME:
		    case DATA_T_STRINGVEC:
		        err = 1;
		        break;

		    case DATA_T_MONEY:
		        m = (pMoneyType)data_ptr_2;
			if (m->WholePart > intval) cmp_value = -1;
			else if (m->WholePart < intval) cmp_value = 1;
			else cmp_value = m->FractionPart?-1:0;
			break;

		    case DATA_T_INTVEC:
		        iv = (pIntVec)data_ptr_2;
			if (iv->nIntegers != 1)
			    err = 1;
			else
			    cmp_value = intval - iv->Integers[0];
			break;

		    case DATA_T_DOUBLE:
		        dblval = *(double*)data_ptr_2;
			if (intval > dblval) cmp_value = 1;
			else if (intval < dblval) cmp_value = -1;
			else cmp_value = 0;
			break;
		    }
	        break;

	    case DATA_T_STRING:
	        /** Compare string... to what? **/
		strval = (char*)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_STRING:
		        cmp_value = strcmp(strval, (char*)data_ptr_2);
			break;

		    case DATA_T_DATETIME:
		        objDataToDateTime(DATA_T_STRING, data_ptr_1, &dt_v, NULL);
			dt = (pDateTime)data_ptr_2;
			cmp_value = dt_v.Value - dt->Value;
			break;
		
		    case DATA_T_MONEY:
		        objDataToMoney(DATA_T_STRING, data_ptr_1, &m_v);
			m = (pMoneyType)data_ptr_2;
			if (m_v.WholePart > m->WholePart) cmp_value = 1;
			else if (m_v.WholePart < m->WholePart) cmp_value = -1;
			else cmp_value = m_v.FractionPart - m->FractionPart;
			break;

		    case DATA_T_DOUBLE:
		        dblval = strtod(strval, NULL);
			if (dblval == *(double*)data_ptr_2) cmp_value = 0;
			else if (dblval > *(double*)data_ptr_2) cmp_value = 1;
			else cmp_value = -1;
			break;
		    
		    case DATA_T_INTVEC:
		        err = 1;
			break;

		    case DATA_T_STRINGVEC:
		        sv = (pStringVec)data_ptr_2;
			if (sv->nStrings != 1)
			    err = 1;
			else
			    cmp_value = strcmp(strval, sv->Strings[0]);
			break;
		    }
	        break;

	    case DATA_T_DATETIME:
	        /** Comparing with datetime. **/
	        dt = (pDateTime)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_DATETIME:
		        cmp_value = dt->Value - ((pDateTime)data_ptr_2)->Value;
			break;

		    case DATA_T_INTVEC:
		        iv = (pIntVec)data_ptr_2;
		        if (iv->nIntegers != 6)
			    {
			    err = 1;
			    }
			else
			    {
		            dt_v.Part.Year = iv->Integers[0];
			    if (dt_v.Part.Year < 5) dt_v.Part.Year += 100;
			    else if (dt_v.Part.Year >= 1900) dt_v.Part.Year -= 1900;
			    dt_v.Part.Month = iv->Integers[1]-1;
			    dt_v.Part.Day = iv->Integers[2]-1;
			    dt_v.Part.Hour = iv->Integers[3];
			    dt_v.Part.Minute = iv->Integers[4];
			    dt_v.Part.Second = iv->Integers[5];
			    cmp_value = dt->Value - dt_v.Value;
			    }
			break;

		    case DATA_T_MONEY:
		    case DATA_T_DOUBLE:
		    case DATA_T_STRINGVEC:
		        err = 1;
			break;
		    }
	        break;

	    case DATA_T_MONEY:
	        /** Compare a money type to another type. **/
		m = (pMoneyType)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_MONEY: 
			if (m->WholePart > ((pMoneyType)data_ptr_2)->WholePart) cmp_value = 1;
			else if (m->WholePart < ((pMoneyType)data_ptr_2)->WholePart) cmp_value = -1;
			else cmp_value = m->FractionPart - ((pMoneyType)data_ptr_2)->FractionPart;
			break;

		    case DATA_T_DOUBLE:
		        dblval = m->WholePart + (m->FractionPart/10000.0);
			if (dblval == *(double*)data_ptr_2) cmp_value = 0;
			else if (dblval > *(double*)data_ptr_2) cmp_value = 1;
			else cmp_value = -1;
			break;

		    case DATA_T_INTVEC:
		        iv = (pIntVec)data_ptr_2;
			if (iv->nIntegers != 2)
			    {
			    err = 1;
			    }
			else
			    {
		            if (m->WholePart > iv->Integers[0]) cmp_value = 1;
			    else if (m->WholePart < iv->Integers[0]) cmp_value = -1;
			    else cmp_value = m->FractionPart - iv->Integers[1];
			    }
			break;

		    case DATA_T_STRINGVEC:
		        err = 1;
			break;
		    }
	        break;

	    case DATA_T_DOUBLE:
	        dblval = *(double*)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_DOUBLE:
		        if (dblval > *(double*)data_ptr_2) cmp_value = 1;
			else if (dblval < *(double*)data_ptr_2) cmp_value = -1;
			else cmp_value = 0;
			break;

		    case DATA_T_INTVEC:
		        iv = (pIntVec)data_ptr_2;
			if (iv->nIntegers != 1)
			    {
			    err = 1;
			    }
			else
			    {
			    if (dblval > iv->Integers[0]) cmp_value = 1;
			    else if (dblval < iv->Integers[0]) cmp_value = -1;
			    else cmp_value = 0;
			    }
			break;

		    case DATA_T_STRINGVEC:
		        err = 1;
			break;
		    }
	        break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr_1;
		if (data_type_2 == DATA_T_INTVEC)
		    {
		    iv2 = (pIntVec)data_ptr_2;
		    cmp_value = 0;
		    for(i=0;i<iv->nIntegers || i<iv2->nIntegers;i++)
		        {
			if (i == iv->nIntegers)
			    {
			    cmp_value = 1;
			    break;
			    }
			else if (i == iv2->nIntegers)
			    {
			    cmp_value = -1;
			    break;
			    }
			cmp_value = iv->Integers[i] - iv2->Integers[i];
			if (cmp_value != 0) break;
			}
		    }
		else
		    {
		    err = 1;
		    }
	        break;

	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr_1;
		if (data_type_2 != DATA_T_STRINGVEC)
		    {
		    sv2 = (pStringVec)data_ptr_2;
		    cmp_value = 0;
		    for(i=0;i<sv->nStrings || i<sv2->nStrings;i++)
		        {
			if (i == sv->nStrings)
			    {
			    cmp_value = 1;
			    break;
			    }
			else if (i == sv2->nStrings)
			    {
			    cmp_value = -1;
			    break;
			    }
			cmp_value = strcmp(sv->Strings[i], sv2->Strings[i]);
			if (cmp_value != 0) break;
			}
		    }
		else
		    {
		    err = 1;
		    }
	        break;
	    }
	if (cmp_value > 0) cmp_value = 1;
	if (cmp_value < 0) cmp_value = -1;

    	/** Error? **/
	if (err) return -2;

    return cmp_direction*cmp_value;
    }


/*** objDataOperation - performs the given operation on the two data types,
 *** with result to the third, which may be the same object as the first,
 *** but not the second.  Operations may be '+','-','/','*'.
 ***/
int
objDataOperation(char op, int dt1, void* dp1, int dt2, void* dp2, int dt3, void* dp3)
    {
    return 0;
    }


/*** objDataToWords - converts a data item (integer, money, etc) to an english
 *** textual representation.  The result value is in a temporary location, so must
 *** be copied out before any task-switch operation might occur.
 ***/
char*
objDataToWords(int data_type, void* data_ptr)
    {
    static XString tmpbuf;
    static int is_init = 0;
    static char* digits[11] = { "Zero","One","Two","Three","Four","Five","Six","Seven","Eight","Nine","Ten" };
    static char* teens[9] = { "Eleven","Twelve","Thirteen","Fourteen","Fifteen","Sixteen","Seventeen","Eighteen","Nineteen" };
    static char* tens[9] = { "Ten","Twenty","Thirty","Fourty","Fifty","Sixty","Seventy","Eighty","Ninety" };
    static char* multiples[] = { "", "Thousand","Million","Billion","Trillion","Quadrillion" };
    unsigned long integer_part, fraction_part = 0;
    int multiple_cnt, n, i;
    pMoneyType m;
    char nbuf[16];
    	
	/** Initialize the buffer **/
	if (!is_init)
	    {
	    is_init = 1;
	    xsInit(&tmpbuf);
	    }
	xsCopy(&tmpbuf,"", -1);

    	/** Convert the integer part first.  **/
	if (data_type == DATA_T_INTEGER)
	    {
	    if (*(int*)data_ptr < 0)
	        {
	        integer_part = - *(int*)data_ptr;
		xsConcatenate(&tmpbuf,"Negative ", -1);
		}
	    else
	        {
	        integer_part = *(int*)data_ptr;
		}
	    }
	else if (data_type == DATA_T_MONEY)
	    {
	    m = (pMoneyType)data_ptr;
	    if (m->WholePart < 0)
	        {
		if (m->FractionPart == 0)
		    {
		    integer_part = -m->WholePart;
		    fraction_part = 0;
		    }
		else
		    {
		    integer_part = (-m->WholePart) - 1;
		    fraction_part = 10000 - m->FractionPart;
		    }
		xsConcatenate(&tmpbuf, "Negative ", -1);
		}
	    else
	        {
		integer_part = m->WholePart;
		fraction_part = m->FractionPart;
		}
	    }
	else
	    {
	    mssError(1,"OBJ","Warning: can only 'convert to words' integer and money types");
	    return "";
	    }

	/** Ok, take it in chunks of three digits **/
	if (integer_part == 0)
	    {
	    xsConcatenate(&tmpbuf, digits[0], -1);
	    xsConcatenate(&tmpbuf, " ", -1);
	    }
	else
	    {
	    multiple_cnt = 5;
	    while(multiple_cnt >= 0)
	        {
		/** Determine the current 3 digits **/
		n = integer_part;
		for(i=0;i<multiple_cnt;i++) n /= 1000;
		n = n%1000;

		/** Need to put hundreds? **/
		if (n >= 100)
		    {
		    xsConcatenate(&tmpbuf, digits[n/100], -1);
		    xsConcatenate(&tmpbuf, " Hundred ", -1);
		    }

		/** Now, put tens digit?  And ones digit? **/
		if ((n%100) > 19)
		    {
		    xsConcatenate(&tmpbuf, tens[((n%100)/10)-1], -1);
		    if (n%10 != 0) 
		        {
		        xsConcatenate(&tmpbuf, "-", 1);
			xsConcatenate(&tmpbuf, digits[n%10], -1);
		        xsConcatenate(&tmpbuf, " ", 1);
			}
		    else
		        {
		        xsConcatenate(&tmpbuf, " ", 1);
			}
		    }
		else if ((n%100) > 10)
		    {
		    xsConcatenate(&tmpbuf, teens[(n%100) - 11], -1);
		    xsConcatenate(&tmpbuf, " ", 1);
		    }
		else if ((n%100) > 0)
		    {
		    xsConcatenate(&tmpbuf, digits[(n%100)], -1);
		    xsConcatenate(&tmpbuf, " ", 1);
		    }

		/** Append the multiplier units (thousand, million, etc) and possibly a comma **/
		if (n != 0) xsConcatenate(&tmpbuf, multiples[multiple_cnt], -1);
		if (n != 0 && n>10 && ((n%10) != 0 || n > 100) && multiple_cnt != 0)
		    {
		    xsConcatenate(&tmpbuf, ", ", 2);
		    }
		else
		    {
		    if (multiple_cnt != 0 && n != 0) xsConcatenate(&tmpbuf, " ", 1);
		    }
		multiple_cnt--;
		}
	    }

	/** Now take care of cents if a money type **/
	if (data_type == DATA_T_MONEY)
	    {
	    if (fraction_part == 0)
	        {
		xsConcatenate(&tmpbuf, "And No/100 ", -1);
		}
	    else
	        {
	        sprintf(nbuf, "And %2.2ld/100 ", fraction_part/100);
		xsConcatenate(&tmpbuf, nbuf, -1);
		}
	    }

    return tmpbuf.String;
    }


/*** objCopyData() - copies from one POD to another, based on type.
 ***/
int
objCopyData(pObjData src, pObjData dst, int type)
    {

	/** Copy the ObjData raw **/
	switch(type)
	    {
	    case DATA_T_INTEGER:
		dst->Integer = src->Integer;
		break;
	    case DATA_T_STRING:
	    case DATA_T_MONEY:
	    case DATA_T_DATETIME:
	    case DATA_T_INTVEC:
	    case DATA_T_STRINGVEC:
	    case DATA_T_CODE:
		dst->Generic = src->Generic;
		break;
	    case DATA_T_DOUBLE:
		dst->Double = src->Double;
		break;
		
	    default:
		return -1;
	    }

    return 0;
    }


/*** objDebugDate() - prints date/time value on stdout in various 
 *** formats.
 ***/
int
objDebugDate(pDateTime dt)
    {
    int i;
	
	printf("Parts:  %4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\n",
		dt->Part.Year+1900, dt->Part.Month+1, dt->Part.Day+1,
		dt->Part.Hour, dt->Part.Minute, dt->Part.Second);
	printf("Value:  %16.16llx\n", dt->Value);
	printf("Binary: ");
	for(i=63;i>=0;i--)
	    printf("%d", (int)((dt->Value>>i)&1));
	printf("\n");

    return 0;
    }

