#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <byteswap.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#endif
#include "obj.h"
#include "expression.h"
#include "cxlib/xstring.h"
#include "cxlib/mtsession.h"
#include "cxlib/util.h"
#include "cxss/cxss.h"

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
 ***   I = a leading 'I' is not printed but indicates the currency is in "international format" with commas and periods reversed.
 ***   Z = a leading 'Z' is not printed but means zeros should be printed as "-0-"
 ***   z = a leading 'z' is not printed but means zeros should be printed as "0"
 ***   B = a leading 'B' is not printed but means zeros should be printed as "" (blank)
 ***   # = optional digit unless after the '.' or first before '.'
 ***   0 = mandatory digit, no zero suppression
 ***   , = insert a comma (or a period, if in international format)
 ***   . = decimal point (only one allowed) (prints a comma if in international format)
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
    /*char* endptr;*/
    char* enditemptr;
    /*int n_items;*/

	/** No format string? **/
	if (!srcptr)
	    {
	    return -1;
	    }

	/** Locate a possible list of language strings **/
	ptr = strstr(srcptr, searchstart);
	if (!ptr || !strstr(ptr+strlen(searchstart),searchend)) return -1;

	/** Ok, got it.  Now start parsing 'em **/
	/*n_items = 0;*/
	ptr = ptr + strlen(searchstart);
	/*endptr = strstr(ptr,searchend);*/
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


int
objCurrentDate(pDateTime dt)
    {
    struct tm* tmptr;
    time_t t;

	t = time(NULL);
	tmptr = localtime(&t);
	dt->Value = 0;
	dt->Part.Second = tmptr->tm_sec;
	dt->Part.Minute = tmptr->tm_min;
	dt->Part.Hour = tmptr->tm_hour;
	dt->Part.Day = tmptr->tm_mday - 1;
	dt->Part.Month = tmptr->tm_mon;
	dt->Part.Year = tmptr->tm_year;

    return 0;
    }

/*** obj_internal_FormatDate - formats a date/time value to the given string
 *** value.
 ***/
int
obj_internal_FormatDate(pDateTime dt, char* str, char* format, int length)
    {
    char* fmt;
    char* myfmt;
    int append_ampm = 0;
    char *Lw[7], *LW[7], *Lm[12], *LM[12];
    char *Lwptr, *LWptr, *Lmptr, *LMptr;
    char tmp[20];
    XString xs;
	
	xsInit(&xs);
    
    	/** Get the current date format. **/
	if (format)
	    fmt = format;
	else
	    cxssGetVariable("dfmt", &fmt, obj_default_date_fmt);
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
		        sprintf(tmp,"%dst",dt->Part.Day+1);
		    else if (dt->Part.Day == 1 || dt->Part.Day == 21)
		        sprintf(tmp,"%dnd",dt->Part.Day+1);
		    else if (dt->Part.Day == 2 || dt->Part.Day == 22)
		        sprintf(tmp,"%drd",dt->Part.Day+1);
		    else
		        sprintf(tmp,"%dth",dt->Part.Day+1);
		    xsConcatenate(&xs, tmp, -1);
		    fmt+=2;
		    }
		else if (fmt[1] == 'd')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Day+1);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		break;

	    /** Month name or number **/
	    case 'M':
	        if (fmt[1] == 'M' && fmt[2] == 'M' && fmt[3] == 'M')
		    {
		    xsConcatenate(&xs,obj_long_months[dt->Part.Month], -1);
		    fmt+=3;
		    }
		else if (fmt[1] == 'M' && fmt[2] == 'M')
		    {
		    xsConcatenate(&xs,obj_short_months[dt->Part.Month], -1);
		    fmt+=2;
		    }
		else if (fmt[1] == 'M')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Month+1);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		break;

	    case 'y':
	        if (fmt[1] == 'y' && fmt[2] == 'y' && fmt[3] == 'y')
		    {
		    sprintf(tmp,"%4.4d",dt->Part.Year+1900);
		    xsConcatenate(&xs, tmp, -1);
		    fmt+=3;
		    }
		else if (fmt[1] == 'y')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Year % 100);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		break;

	    case 'H':
	        if (fmt[1] == 'H')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Hour);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		append_ampm = 0;
		break;

	    case 'h':
	        if (fmt[1] == 'h')
		    {
		    sprintf(tmp,"%2.2d",((dt->Part.Hour%12)==0)?12:(dt->Part.Hour%12));
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
	        append_ampm = 1;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(tmp,(dt->Part.Hour >= 12)?"PM":"AM");
		    xsConcatenate(&xs, tmp, -1);
		    }
		break;

	    case 'm':
	        if (fmt[1] == 'm')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Minute);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(tmp,(dt->Part.Hour >= 12)?"PM":"AM");
		    xsConcatenate(&xs, tmp, -1);
		    }
		break;

	    case 's':
	        if (fmt[1] == 's')
		    {
		    sprintf(tmp,"%2.2d",dt->Part.Second);
		    xsConcatenate(&xs, tmp, -1);
		    fmt++;
		    }
		fmt++;
		if (append_ampm && (*fmt == ' ' || *fmt == ',' || *fmt == '\0'))
		    {
		    append_ampm = 0;
		    strcpy(tmp,(dt->Part.Hour >= 12)?"PM":"AM");
		    xsConcatenate(&xs, tmp, -1);
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
		xsConcatenate(&xs, fmt, 1);
		fmt++;
		break;
	    }

	nmSysFree(myfmt);
	
	if(strlen(xs.String) < length)
	    {
	    strcpy(str,xs.String);
	    xsDeInit(&xs);
	    return 0;
	    }
	else
	    {
	    xsDeInit(&xs);
	    return -1;
	    }
    }

 /*** objFormatDateTmp - formats a date/time value to a tmp buffer
 *** returns a pointer to the date string
 ***/
char*
objFormatDateTmp(pDateTime dt, char* format)
    {
    /** this should hold the datetime data just fine 
     ** but format strings better not be user supplied...
     **/
    static char tmp[80];

	if (!dt) return NULL;
        if(obj_internal_FormatDate(dt,tmp,format,80)) return NULL;

    return tmp;
    }


/*** obj_internal_FormatMoney - format a money data type into a string
 *** using the default format or the one stored in the session.
 ***/
int
obj_internal_FormatMoney(pMoneyType m, char* str, char* format, int length)
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
    char tmp[20];
    XString xs;
    /*int intl_format = 0;*/
    int zero_type = 0; /* 0=normal, 1='-0-', 2='0', 3='' */
    char* zero_strings[] = {NULL, "-0-", "0", ""};
    char decimal = '.';
    char comma = ',';
    
	/** Get the format **/
	if (format)
	    fmt = format;
	else
	    cxssGetVariable("mfmt", &fmt, obj_default_money_fmt);
	start_fmt = fmt;

	/** Determine number of explicitly-specified whole part digits **/
	ptr = fmt;
	while(*ptr && *ptr != decimal)
	    {
	    if (*ptr == '0' || *ptr == '#' || *ptr == '^' || *ptr == ' ' || *ptr == '*') 
	        {
		n_whole_digits++;
		tens_multiplier *= 10;
		}
	    else if (*ptr == 'I')
		{
		/*intl_format = 1;*/
		decimal = ',';
		comma = '.';
		}
	    else if (*ptr == 'Z')
		zero_type = 1;
	    else if (*ptr == 'z')
		zero_type = 2;
	    else if (*ptr == 'B')
		zero_type = 3;
	    ptr++;
	    }
	if (tens_multiplier > 0) tens_multiplier /= 10;

	/** Special handling of zeros **/
	if (m->Value == 0 && zero_type != 0)
	    {
	    if (strlen(zero_strings[zero_type]) >= length)
		return -1;
	    else
		strcpy(str, zero_strings[zero_type]);
	    return 0;
	    }

	xsInit(&xs);

	/** Any reasons to omit the automatic-sign? **/
	if (strpbrk(fmt,"+-()[]")) automatic_sign = 0;

	/** Determine the 'print' version of whole/fraction parts **/
	orig_print_whole = m->Value/10000;
	if (m->Value < 0)
        {
            print_whole = -m->Value/10000;
            print_fract = -m->Value%10000;
        }
	else
        {
            print_whole = m->Value/10000;
            print_fract = m->Value%10000;
        }

	/** Ok, start generating the thing. **/
	while(*fmt) 
	    {
            if (automatic_sign)
                {
                automatic_sign = 0;
                if (orig_print_whole < 0)
		    *(str++) = '-';
                /*else
		    *(str++) = ' ';*/
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
			if (*fmt == ' ') xsConcatenate(&xs," ",1);
			else if (*fmt == '*') xsConcatenate(&xs," ",1);
			}
		    else
		        {
			sprintf(tmp,"%d",d);
			xsConcatenate(&xs,tmp,-1);
			}
		    break;

                case ',':
		    if (!suppressing_zeros) 
		        {
			xsConcatenate(&xs,&comma,1);
			}
		    else
		        {
			/** Replace comma with space if no digits and surrounded by placeholders **/
			if (!((start_fmt != fmt && fmt[-1] == '#') || fmt[1] == '#'))
			    {
			    xsConcatenate(&xs," ",1);
			    }
			}
		    break;

                case '.':
		    if (print_whole != 0)
		        {
			sprintf(tmp,"%d",print_whole);
			xsConcatenate(&xs,tmp,1);
			}
                    in_decimal_part = 1;
		    suppressing_zeros = 0;
		    tens_multiplier = 1000;
		    xsConcatenate(&xs,&decimal,1);
                    break;
    
                case '-':
		    if (orig_print_whole >= 0) xsConcatenate(&xs," ",1);
		    else xsConcatenate(&xs,"-",1);
		    break;

                case '+':
		    if (orig_print_whole >= 0) xsConcatenate(&xs,"+",1);
		    else xsConcatenate(&xs,"-",1);
		    break;

                case '[':
		    if (orig_print_whole >= 0) xsConcatenate(&xs,"(",1);
		    else xsConcatenate(&xs," ",1);
		    break;

                case '(':
		    if (orig_print_whole < 0) xsConcatenate(&xs,"(",1);
		    else xsConcatenate(&xs," ",1);
		    break;

                case ']':
		    if (orig_print_whole >= 0) xsConcatenate(&xs,")",1);
		    else xsConcatenate(&xs," ",1);
		    break;

                case ')':
		    if (orig_print_whole < 0) xsConcatenate(&xs,")",1);
		    else xsConcatenate(&xs," ",1);
		    break;

                default:
                    break;
                }
	    fmt++;
	    }

	if(strlen(xs.String) < length)
	    {
	    strcpy(str,xs.String);
	    xsDeInit(&xs);
	    return 0;
	    }
	else
	    {
	    xsDeInit(&xs);
	    return -1;
	    }
    }


/*** objFormatMoneyTmp - formats a money value to a tmp buffer
 *** returns pointer to the money string
 ***/
char*
objFormatMoneyTmp(pMoneyType m, char* format)
    {
    /** this should hold the money data just fine 
     ** but format strings better not be user supplied...
     **/
    static char tmp[64]; 
    
        if(obj_internal_FormatMoney(m,tmp,format,64)) return NULL;
    return tmp;
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
		obj_internal_FormatMoney(m, sbuf + strlen(sbuf),NULL,80-strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		xsConcatenate(dest, sbuf, -1);
		break;

	    case DATA_T_DATETIME:
	        d = (pDateTime)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_DATECONV) strcat(sbuf, " convert(datetime,");
	        if (flags & (DATA_F_QUOTED | DATA_F_DATECONV)) strcat(sbuf, " '");
		obj_internal_FormatDate(d, sbuf + strlen(sbuf),NULL,80-strlen(sbuf));
	        if (flags & (DATA_F_QUOTED | DATA_F_DATECONV)) strcat(sbuf, "' ");
	        if (flags & DATA_F_DATECONV) strcat(sbuf, ") ");
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
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest, (flags & DATA_F_BRACKETS)?" [":" (", 2);
		for(i=0;i<sv->nStrings;i++)
		    {
		    xsConcatQPrintf(dest, (flags & DATA_F_SINGLE)?"%[,%]%STR&QUOT":"%[,%]%STR&DQUOT", i!=0, sv->Strings[i]);
		    /*sprintf(sbuf,"%s\"%s\"", (i==0)?"":",", sv->Strings[i]);
		    xsConcatenate(dest, sbuf, -1);*/
		    }
	        if (flags & DATA_F_QUOTED) xsConcatenate(dest, (flags & DATA_F_BRACKETS)?"] ":") ", 2);
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
		else base = strtoi(format,NULL,0);
	        v = strtoi((char*)data_ptr,&uptr,base); 
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
	        if (m->Value/10000 > INT_MAX)
                mssError(1, "OBJ", "Warning: %d overflow; cannot fit value of that size in int ", data_type);
	        else
	            v = m->Value/10000;
		break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr;
		if (iv->nIntegers == 0) v = 0; else v = iv->Integers[0];
		break;

	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr;
		if (sv->nStrings == 0) v = 0; else v = strtoi(sv->Strings[0],NULL,0);
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
	    case DATA_T_MONEY: m = (pMoneyType)data_ptr; v = m->Value/10000.0; break;
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
    static char sbuf[160];
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
		    sprintf(sbuf, " %d ", *(int*)data_ptr);
		else
		    sprintf(sbuf, "%d", *(int*)data_ptr);
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
		    str_ptr = (char*)(bn->Data);
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
	        /** sbuf is 160 chars, plenty for our purposes here. **/
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
		    if (flags & DATA_F_QUOTED)
			strcpy(ptr,".0 ");
		    else
			strcpy(ptr+1,".0");
		    }
		ptr = sbuf;
		break;

	    case DATA_T_MONEY:
	        m = (pMoneyType)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		obj_internal_FormatMoney(m, sbuf + strlen(sbuf),NULL, sizeof(sbuf)-strlen(sbuf));
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " ");
		break;

	    case DATA_T_DATETIME:
	        d = (pDateTime)data_ptr;
		sbuf[0] = '\0';
	        if (flags & DATA_F_QUOTED) strcat(sbuf, " '");
		obj_internal_FormatDate(d, sbuf + strlen(sbuf),NULL, sizeof(sbuf)-strlen(sbuf));
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
		    snprintf(ptr, sizeof(sbuf) - (ptr - sbuf) - 2, "%s%d", (i==0)?"":",", iv->Integers[i]);
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
		    strcpy(ptr, (flags & DATA_F_BRACKETS)?" [":" (");
		    ptr += 2;
		    }
		for(i=0;i<sv->nStrings;i++)
		    {
		    snprintf(ptr, sizeof(sbuf) - (ptr - sbuf) - 2, "%s\"%s\"", (i==0)?"":",", sv->Strings[i]);
		    ptr += strlen(ptr);
		    }
	        if (flags & DATA_F_QUOTED) 
		    {
		    strcpy(ptr, (flags & DATA_F_BRACKETS)?"] ":") ");
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
    int got_hroffset=9999, got_minoffset=9999;
    int last_num;
    char* prev_startptr;
    char* startptr;
    char* endptr;
    char* origptr;
    int i;
    struct tm *t;
    time_t int_time;
    int reversed_day=0;
    int iso = 0;
    time_t z_time, loc_time;
    int ouroffset;
    int timediff;

    	/** Only accept string, datetime, integer... **/
	if (data_type != DATA_T_STRING && data_type != DATA_T_DATETIME) return -1;

	/** Integer conversion **/
	if (data_type == DATA_T_INTEGER)
	    {
	    memset(dt, 0, sizeof(DateTime));
	    dt->Value = *((int*)data_ptr);
	    return 0;
	    }

	/** "Conversion" of dt->dt? **/
	if (data_type == DATA_T_DATETIME)
	    {
	    memcpy(dt, data_ptr, sizeof(DateTime));
	    return 0;
	    }

	/** Default is to interpret as mm-dd-yyyy (U.S. format)
	 ** "II" uses the more common non-U.S. format, dd-mm-yyyy
	 ** "ISO" uses an international format similar to ISO 8601: 
	 ** 		yyyy-mm-dd hh:mm:ss
	 ** this provides the standard format for many SQL engines
	 **/
	if (format)
	    {
	    if (!strncmp(format,"II",2)) reversed_day = 1;
	    if (!strncmp(format,"ISO",5)) iso = 1;
	    }

	prev_startptr = NULL;
	startptr = (char*)data_ptr;
	origptr = startptr;
	while(*startptr)
	    {
	    /** Skip whitespace **/
	    while(*startptr == ' ' || *startptr == '\t' || *startptr == ',') startptr++;

	    /** Try to convert a number. **/
	    last_num = strtoi(startptr, &endptr, 10);
	    if (last_num < 0) last_num = -last_num;
	    if (endptr != startptr)
	        {
		/** Got a number **/
		if (*endptr == ':')
		    {
		    /** If the number starts with + or - and ends with :, it may be a timezone offset **/
		    if (startptr == origptr || (startptr[0] != '-' && startptr[0] != '+'))
			{
			/** time field.  Check which ones we have. **/
			if (got_hr == -1)
			    got_hr = last_num;
			else if (got_min == -1)
			    got_min = last_num;
			}
		    else
			{
			/** timezone offset hours field **/
			if (got_hroffset == 9999)
			    {
			    got_hroffset = last_num;
			    if (startptr != origptr && startptr[0] == '-')
				got_hroffset = -got_hroffset;
			    }
			}
		    endptr++;
		    }
		else if (*endptr == '/' || (*endptr == '-' && (got_day == -1 || got_mo == -1 || got_yr == -1)))
		    {
		    /** Date field.  Check. **/
		    if (last_num > 99)
			{
			reversed_day = 0;
			got_yr = last_num;
			}
		    else if (reversed_day)
		        {
		        if (got_day == -1) got_day = last_num-1;
		        else if (got_mo == -1) got_mo = last_num-1;
			}
		    else if(iso)
			{
			if (got_yr == -1) got_yr = last_num;
			else if (got_mo == -1) got_mo = last_num-1;
			else if (got_day == -1) got_day = last_num-1;
			}
		    else
		        {
		        if (got_mo == -1) got_mo = last_num-1;
		        else if (got_day == -1) got_day = last_num-1;
			}
		    endptr++;
		    }
		else
		    {
		    /** End-of-string -or- space-separated date/time **/
		    if (startptr != origptr && startptr[-1] == ':')
		        {
			/** If we have a tz hr offset but no min offset, and the previous
			 ** number had a + or - right before it, this is a TZ minute
			 ** offset.
			 **/
			if (got_hroffset != 9999 && got_minoffset == 9999 && prev_startptr && prev_startptr != origptr && (prev_startptr[0] == '-' || prev_startptr[0] == '+'))
			    {
			    got_minoffset = last_num;
			    if (prev_startptr[0] == '-')
				got_minoffset = -got_minoffset;
			    }
			/** Otherwise, seconds in '12:00:01' or minutes in '12:00' **/
			else if (got_min == -1)
			    got_min = last_num;
			else if (got_sec == -1)
			    got_sec = last_num;
			}
		    else if (startptr != origptr && (startptr[-1] == '/' || startptr[-1] == '-'))
		        {
			/** For year in '1/1/1999' or day in '1/1' or '1/1 12pm 1999' **/
			if (got_day == -1) got_day = last_num-1;
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
			got_day = last_num-1;
			}
		    else if (startptr != origptr && startptr[-1] == '.')
		        {
			/** Milliseconds as in 12:00:01.000 -- just ignore them **/
			}
		    else if (startptr != origptr && (startptr[0] == '+' || startptr[0] == '-') && got_day != -1 && got_yr != -1 && got_mo != -1)
			{
			/** Timezone offset, hours **/
			got_hroffset = last_num;
			if (startptr[0] == '-')
			    got_hroffset = -got_hroffset;
			}
		    else if (got_mo != -1 && got_day != -1 && got_yr == -1)
		        {
			/** For the case 'Jan 1 1999', when just parsed 'Jan' and '1'. **/
			got_yr = last_num;
			}
		    else if (got_mo == -1 && got_yr == -1 && got_day == -1)
		        {
			/** For the case '1 Jan 1999' **/
			got_day = last_num-1;
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
	    prev_startptr = startptr;
	    startptr = endptr;
	    }

	/** Get the current date/time to fill in some (possibly) blank values **/
	int_time = time(NULL);
	t = localtime(&int_time);
	if (got_yr == -1) got_yr = t->tm_year + 1900;
	if (got_day == -1) got_day = t->tm_mday + 1;
	if (got_mo == -1) got_mo = t->tm_mon;

	/** Fill in hour/min/sec - set to midnight, or set min/sec to 0 **/
	if (got_hr == -1) got_hr = 0;
	if (got_min == -1) got_min = 0;
	if (got_sec == -1) got_sec = 0;

	/** Fix the values of the numbers to the internal representation. **/
	dt->Value = 0;

	/** First, the year. **/
	if (got_yr < (t->tm_year - 100 + 10)) 
	    dt->Part.Year = got_yr + 100;
	else if (got_yr < 100)
	    dt->Part.Year = got_yr + 0;
	else if (got_yr >= 1900 && got_yr < (4096+1900))
	    dt->Part.Year = got_yr - 1900;
	else
	    {
	    printf("Warning: date exceeded internal representation (year = %d)\n", got_yr);
	    dt->Part.Year = 4095;
	    }

	/** Next, month and day **/
	dt->Part.Month = got_mo;
	dt->Part.Day = got_day;

	/** Hour/minute/second **/
	dt->Part.Hour = got_hr;
	dt->Part.Minute = got_min;
	dt->Part.Second = got_sec;

	/** Adjust for timezone, if necessary.  If no offset was
	 ** given in the date/time, then we assume the date/time is
	 ** local time and no more work needs to be done.  IF an
	 ** offset however was supplied, then we need to convert
	 ** to local time.
	 **/
	if (got_hroffset != 9999)
	    {
	    /** Get offset in the date string **/
	    if (got_minoffset == 9999)
		{
		/** ISO form, +XXXX **/
		got_minoffset = got_hroffset/100*60 + got_hroffset%100;
		got_hroffset = 0;
		}
	    else
		{
		/** MySQL form, +XX:XX **/
		got_minoffset = got_minoffset + got_hroffset*60;
		got_hroffset = 0;
		}

	    /** Determine local offset, in minutes **/
	    loc_time = time(NULL);
	    t = gmtime(&loc_time);
	    z_time = mktime(t);
	    ouroffset = difftime(loc_time, z_time)/60;
	    timediff = ouroffset - got_minoffset;

	    /** Adjust the date/time **/
	    objDateAdd(dt, 0, timediff, 0, 0, 0, 0);
	    }

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
    int is_neg = 0;
    unsigned long intval;
    int scale;
    char* fmt;
    int intl_format;
    char tmpbuf[40];
    char* tptr;

    	/** Select the correct type. **/
	switch(data_type)
	    {
	    case DATA_T_STRING:
		cxssGetVariable("mfmt", &fmt, obj_default_money_fmt);
		intl_format = strchr(fmt,'I')?1:0;

	        ptr = (char*)data_ptr;
		while(*ptr == ' ') ptr++;
		if (strlen(ptr) < sizeof(tmpbuf))
		    {
		    /** strip commas (or periods, if in intl format) **/
		    tptr = tmpbuf;
		    while(*ptr)
			{
			if (*ptr != (intl_format?'.':','))
			    *(tptr++) = *ptr;
			ptr++;
			}
		    *tptr = '\0';
		    ptr = tmpbuf;
		    }
		if (*ptr == '-')
		    {
		    is_neg = 1;
		    ptr++;
		    }
		if (*ptr == '$') ptr++;
		if (*ptr == '-')
		    {
		    is_neg = !is_neg;
		    ptr++;
		    }
		intval = 0;
		m->Value = 0;
		intval = strtoul(ptr, &endptr, 10);
		/** Checking for overflow, Max UL can be greater than Max LL **/
		if (intval > 0x7FFFFFFFFFFFFFFFLL)
		    return -1;
		if ((endptr - ptr) != strspn(ptr, "0123456789"))
		    return -1;
		if (is_neg)
		    m->Value = -intval*10000;
		else
		    m->Value = intval*10000;
		
		/** Handling the "fraction" portion after the decimal point **/
		if (*endptr == (intl_format?',':'.'))
		    {
		    intval = strtoul(endptr+1, &endptr2, 10);
		    scale = endptr2 - (endptr+1);
		    if (scale != strspn(endptr+1, "0123456789"))
			return -1;
		    while(scale < 4) { scale++; intval *= 10; }
		    while(scale > 4) { scale--; intval /= 10; }
		    if (is_neg)
                {
		            m->Value -= intval;
                }
		    else
                {
		            m->Value += intval;
                }
		    endptr = endptr2;
		    }
		if (endptr == ptr)
		    {
		    return -1;
		    }
		if (*endptr == '-')
		    {
		    m->Value = -m->Value;
		    is_neg = !is_neg;
		    }
		break;
	    
	    case DATA_T_DOUBLE:
            dbl = *(double*)data_ptr * 10000.0;
            m->Value = (long long)dbl;
		break;
	
	    case DATA_T_INTEGER:
	            m->Value = *(int*)data_ptr * 10000;
		break;

	    case DATA_T_MONEY:
	        m->Value = ((pMoneyType)data_ptr)->Value;
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
    long long dt_cmp_value;

    	/** Need to transpose v1 and v2 to simplify? **/
	/*if ((data_type_1 != DATA_T_INTEGER && data_type_2 == DATA_T_INTEGER) ||
	    (data_type_1 != DATA_T_STRING && data_type_2 == DATA_T_STRING) ||
	    (data_type_1 != DATA_T_DATETIME && data_type_2 == DATA_T_DATETIME) ||
	    (data_type_1 != DATA_T_MONEY && data_type_2 == DATA_T_MONEY) ||
	    (data_type_1 != DATA_T_DOUBLE && data_type_2 == DATA_T_DOUBLE) ||
	    (data_type_1 != DATA_T_INTVEC && data_type_2 == DATA_T_INTVEC) ||
	    (data_type_1 != DATA_T_STRINGVEC && data_type_2 == DATA_T_STRINGVEC))*/
	if (data_type_1 > data_type_2)
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
		        cmp_value = intval - strtoi((char*)data_ptr_2, NULL, 0);
			break;

		    case DATA_T_DOUBLE:
		        dblval = *(double*)data_ptr_2;
			if (intval > dblval) cmp_value = 1;
			else if (intval < dblval) cmp_value = -1;
			else cmp_value = 0;
			break;

		    case DATA_T_DATETIME:
		    case DATA_T_STRINGVEC:
		        err = 1;
		        break;

		    case DATA_T_MONEY:
		        m = (pMoneyType)data_ptr_2;
			if (m->Value/10000 > intval) cmp_value = -1;
			else if (m->Value/10000 < intval) cmp_value = 1;
			else cmp_value = m->Value%10000?-1:0;
			break;

		    case DATA_T_INTVEC:
		        iv = (pIntVec)data_ptr_2;
			if (iv->nIntegers != 1)
			    err = 1;
			else
			    cmp_value = intval - iv->Integers[0];
			break;

		    default:
			err = 1;
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
			dt_cmp_value = dt_v.Value - dt->Value;
			if (dt_cmp_value > 0)
			    cmp_value = 1;
			else if (dt_cmp_value < 0)
			    cmp_value = -1;
			else
			    cmp_value = 0;
			break;
		
		    case DATA_T_MONEY:
		        objDataToMoney(DATA_T_STRING, data_ptr_1, &m_v);
			m = (pMoneyType)data_ptr_2;
			if (m_v.Value > m->Value) cmp_value = 1;
			else if (m_v.Value < m->Value) cmp_value = -1;
			else cmp_value = 0;
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

		    default:
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

		    case DATA_T_MONEY:
			m = (pMoneyType)data_ptr_2;
		        dblval = m->Value/10000.0;
			if (dblval == *(double*)data_ptr_1) cmp_value = 0;
			else if (dblval > *(double*)data_ptr_1) cmp_value = -1;
			else cmp_value = 1;
			break;

		    case DATA_T_STRINGVEC:
		    case DATA_T_DATETIME:
		    default:
		        err = 1;
			break;
		    }
	        break;

	    case DATA_T_DATETIME:
	        /** Comparing with datetime. **/
	        dt = (pDateTime)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_DATETIME:
		        dt_cmp_value = dt->Value - ((pDateTime)data_ptr_2)->Value;
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
			    dt_cmp_value = dt->Value - dt_v.Value;
			    }
			break;

		    case DATA_T_MONEY:
		    case DATA_T_STRINGVEC:
		    default:
		        err = 1;
			break;
		    }
		if (!err)
		    {
		    if (dt_cmp_value > 0)
			cmp_value = 1;
		    else if (dt_cmp_value < 0)
			cmp_value = -1;
		    else
			cmp_value = 0;
		    }
	        break;

	    case DATA_T_INTVEC:
	        iv = (pIntVec)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_INTVEC:
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
			break;

		    case DATA_T_STRINGVEC:
			err = 1;
			break;

		    case DATA_T_MONEY:
			m = (pMoneyType)data_ptr_2;
			if (iv->nIntegers != 2)
			    {
			    err = 1;
			    }
			else
			    {
		            if (m->Value/10000 > iv->Integers[0])
                    {cmp_value = -1; printf("Reached -1");}
			    else if (m->Value/10000 < iv->Integers[0]) 
                    {cmp_value = 1; printf("Reached 1");}
			    else {cmp_value = iv->Integers[1] - m->Value%10000; printf("Reached 0");}
			    }
			break;

		    default:
			err = 1;
			break;
		    }
	        break;

	    case DATA_T_STRINGVEC:
	        sv = (pStringVec)data_ptr_1;
		switch(data_type_2)
		    {
		    case DATA_T_STRINGVEC:
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
			break;

		    case DATA_T_MONEY:
			err = 1;
			break;

		    default:
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
			if (m->Value > ((pMoneyType)data_ptr_2)->Value) cmp_value = 1;
			else if (m->Value < ((pMoneyType)data_ptr_2)->Value) cmp_value = -1;
			else cmp_value = 0;
			break;

		    default:
			err = 1;
			break;
		    }
	        break;

	    default:
		err = 1;
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
    static char* tens[9] = { "Ten","Twenty","Thirty","Forty","Fifty","Sixty","Seventy","Eighty","Ninety" };
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
	    if (m->Value < 0)
	        {
	        /** Keep the opposite (positive) value for printing **/
            integer_part = -m->Value/10000;
            fraction_part = -m->Value%10000;
		xsConcatenate(&tmpbuf, "Negative ", -1);
		}
	    else
	        {
		integer_part = m->Value/10000;
		fraction_part = m->Value%10000;
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


/*** objDataFromString() - same as objDataFromStringAlloc, but is based on
 *** the caller having allocated the memory.
 ***/
int
objDataFromString(pObjData pod, int type, char* str)
    {
    
	switch(type)
	    {
	    case DATA_T_INTEGER:
		pod->Integer = objDataToInteger(DATA_T_STRING, str, NULL);
		break;

	    case DATA_T_STRING:
		pod->String = str;
		break;

	    case DATA_T_DOUBLE:
		pod->Double = objDataToDouble(DATA_T_STRING, str);
		break;

	    case DATA_T_DATETIME:
		objDataToDateTime(DATA_T_STRING, str, pod->DateTime, NULL);
		break;

	    case DATA_T_MONEY:
		objDataToMoney(DATA_T_STRING, str, pod->Money);
		break;

	    default:
		return -1;
	    }

    return 0;
    }


/*** objDataFromStringAlloc() - convert data from a string value to a data type
 *** that is specified.
 ***/
int
objDataFromStringAlloc(pObjData pod, int type, char* str)
    {
    
	switch(type)
	    {
	    case DATA_T_INTEGER:
	    case DATA_T_STRING:
	    case DATA_T_DOUBLE:
		break;

	    case DATA_T_DATETIME:
		pod->DateTime = (pDateTime)nmMalloc(sizeof(DateTime));
		if (!pod->DateTime) return -1;
		break;

	    case DATA_T_MONEY:
		pod->Money = (pMoneyType)nmMalloc(sizeof(MoneyType));
		if (!pod->Money) return -1;
		break;

	    default:
		return -1;
	    }

	if (objDataFromString(pod, type, str) < 0) return -1;

	if (type == DATA_T_STRING)
	    pod->String = nmSysStrdup(pod->String);

    return 0;
    }


/*** BuildBinaryItem - returns 0 on success, -1 on failure, 1 if NULL.
 *** Sets item to point to the data for the item, and sets itemlen to the length.
 *** Requires a tmp_buf from the caller of at least 12 bytes.
 ***/
int
obj_internal_BuildBinaryItem(char** item, int* itemlen, pExpression exp, pParamObjects objlist, unsigned char* tmp_buf)
    {
    int tmp_int;
    double tmp_dbl;
    int rval = 0;
    int j;

	/** Evaluate the item **/
	if (expEvalTree(exp, objlist) < 0)
	    {
	    mssError(0,"MQ","Error evaluating expression");
	    return -1;
	    }

	/** We'll return '1' if NULL. **/
	if (exp->Flags & EXPR_F_NULL)
	    return 1;

	/** Pick the data type and copy the stinkin thing **/
	switch(exp->DataType)
	    {
	    case DATA_T_INTEGER:
		/** XOR 0x80000000 to convert to Offset Zero form. **/
		tmp_int = htonl(exp->Integer ^ 0x80000000);
		memcpy(tmp_buf, &tmp_int, sizeof(tmp_int));
		*item = (char*)tmp_buf;
		*itemlen = sizeof(tmp_int);
		break;

	    case DATA_T_STRING:
		*item = exp->String;
		*itemlen = strlen(exp->String)+1;
		break;

	    case DATA_T_DATETIME:
		((unsigned short*)tmp_buf)[0] = htons(exp->Types.Date.Part.Year);
		tmp_buf[2] = exp->Types.Date.Part.Month;
		tmp_buf[3] = exp->Types.Date.Part.Day;
		tmp_buf[4] = exp->Types.Date.Part.Hour;
		tmp_buf[5] = exp->Types.Date.Part.Minute;
		tmp_buf[6] = exp->Types.Date.Part.Second;
		*item = (char*)(tmp_buf);
		*itemlen = 7;
		break;

	    case DATA_T_MONEY:
		/** XOR 0x8000000000000000 to convert to Offset Zero form. **/
		((long long*)tmp_buf)[0] = bswap_64(exp->Types.Money.Value ^ 0x8000000000000000);
		*item = (char*)(tmp_buf);
        *itemlen = 8;
		break;

	    case DATA_T_DOUBLE:
		/** IEEE double.  sign is most-sig, then 11 bit exponent, then 24 bit mantissa.
		 ** for negatives, we need to invert the whole thing to force sorting in the
		 ** opposite direction.  Note - little endianness affects doubles too.
		 **/
		tmp_dbl = exp->Types.Double;

		/** Correct for byte ordering **/
		if (htonl(0x12345678) != 0x12345678)
		    for(j=0;j<=7;j++) tmp_buf[j+1] = ((char*)&tmp_dbl)[7-j];
		else
		    memcpy(tmp_buf+1, &tmp_dbl, sizeof(double));

		/** Make it binary searchable by handling the negative **/
		if (tmp_buf[1] >= 0x80)
		    {
		    tmp_buf[0] = 0;
		    for(j=1; j<=8; j++) tmp_buf[j] = ~tmp_buf[j];
		    }
		else
		    {
		    tmp_buf[0] = 1;
		    }
		*item = (char*)tmp_buf;
		*itemlen = 9;
		break;

	    default:
		return -1;
	    }

    return rval;
    }


/*** Build a 'binary image' of a list of fields that is suitable for comparison
 *** in grouping and ordering.  Places that image in 'buf' and returns the
 *** length in 'buf' that was used for the image.  Returns -1 on error.
 ***
 *** This is binary sort safe -- meaning data values are copied here so that
 *** doing a memcmp() on the resulting values is accurate both for equality and
 *** ordering comparisons.
 ***/
int
objBuildBinaryImage(char* buf, int buflen, void* fields_v, int n_fields, void* objlist_v, int asciz)
    {
    pExpression* fields = (pExpression*)fields_v;
    pParamObjects objlist = (pParamObjects)objlist_v;
    int rval;
    int i,j;
    pExpression exp;
    char* ptr;
    char* cptr;
    char* fieldstart;
    int clen;
    unsigned char tmp_data[12];
    char hex[] = "0123456789abcdef";
    char val;

	if (asciz)
	    {
	    if (buflen < 1)
		return -1;
	    else
		buflen--;
	    }

	ptr = buf;
	for(i=0;i<n_fields;i++)
	    {
	    fieldstart = ptr;

	    /** Evaluate the item **/
	    exp = fields[i];
	    if (!exp)
		rval = 1;
	    else
		rval = obj_internal_BuildBinaryItem(&cptr, &clen, exp, objlist, tmp_data);
	    if (rval < 0) return -1;

	    /** NULL indication **/
	    if (ptr+1 >= buf+buflen) return -1;
	    if (rval == 1)
		{
		*(ptr++) = '0';
		}
	    else
		{
		/** Not null.  Copy null indication and data **/
		*(ptr++) = '1';

		/** Copy the data to the binary image buffer **/
		if (asciz)
		    {
		    /** Won't fit in buffer? **/
		    if (ptr+clen*2 >= buf+buflen) return -1;

		    /** Swap the null indication **/
		    if (exp->Flags & EXPR_F_DESC)
			ptr[-1] = ('1' + '0') - ptr[-1];

		    /** Copy it, transforming it to a hex string */
		    for(j=0; j<clen; j++)
			{
			val = cptr[j];
			if (exp->Flags & EXPR_F_DESC)
			    val = ~val;
			ptr[j*2] = hex[(cptr[j]>>4)&0xf];
			ptr[j*2+1] = hex[cptr[j]&0xf];
			}
		    ptr += clen*2;
		    }
		else
		    {
		    /** Won't fit in buffer? **/
		    if (ptr+clen >= buf+buflen) return -1;

		    memcpy(ptr, cptr, clen);
		    ptr += clen;

		    /** If sorting in DESC order for this item... **/
		    if (exp->Flags & EXPR_F_DESC)
			{
			/** Start at the null ind. to pick up the null value flag too **/
			for(j=0;j<(ptr - fieldstart);j++) fieldstart[j] = ~fieldstart[j];
			}
		    }

		}
	    }

	if (asciz)
	    {
	    *(ptr++) = '\0';
	    }

    return (ptr - buf);
    }


/*** Same as above, just to an xstring instead of a c-string
 ***/
int
objBuildBinaryImageXString(pXString str, void* fields_v, int n_fields, void* objlist_v, int asciz)
    {
    pExpression* fields = (pExpression*)fields_v;
    pParamObjects objlist = (pParamObjects)objlist_v;
    int i,j;
    int fieldoffset;
    int startoffset;
    int clen;
    char* cptr;
    unsigned char tmp_data[12];
    int rval;
    pExpression exp;
    char hex[] = "0123456789abcdef";
    char val, hval;

	startoffset = str->Length;
	for(i=0;i<n_fields;i++)
	    {
	    fieldoffset = str->Length;

	    /** Evaluate the item **/
	    exp = fields[i];
	    if (!exp)
		rval = 1;
	    else
		rval = obj_internal_BuildBinaryItem(&cptr, &clen, exp, objlist, tmp_data);
	    if (rval < 0) return -1;

	    if (rval == 1)
		{
		xsConcatenate(str, "0", 1);
		}
	    else
		{
		xsConcatenate(str, "1", 1);

		if (asciz)
		    {
		    /** Swap the null indication **/
		    if (exp->Flags & EXPR_F_DESC)
			str->String[str->Length-1] = ('1' + '0') - str->String[str->Length-1];

		    /** Copy it, transforming it to a hex string */
		    for(j=0; j<clen; j++)
			{
			val = cptr[j];
			if (exp->Flags & EXPR_F_DESC)
			    val = ~val;
			hval = hex[(cptr[j]>>4)&0xf];
			xsConcatenate(str, &hval, 1);
			hval = hex[cptr[j]&0xf];
			xsConcatenate(str, &hval, 1);
			}
		    }
		else
		    {
		    xsConcatenate(str, cptr, clen);

		    /** If sorting in DESC order for this item... **/
		    if (exp->Flags & EXPR_F_DESC)
			{
			/** Start at null ind. to pick up the null value flag too **/
			for(j=fieldoffset;j<str->Length;j++) str->String[j] = ~str->String[j];
			}
		    }
		}
	    }

    return str->Length - startoffset;
    }


int
obj_internal_DateAddModAdd(int v1, int v2, int mod, int* overflow)
    {
    int rv;
    rv = (v1 + v2)%mod;
    *overflow = (v1 + v2)/mod;
    if (rv < 0)
	{
	*overflow -= 1;
	rv += mod;
	}
    return rv;
    }


int
objDateAdd(pDateTime dt, int diff_sec, int diff_min, int diff_hr, int diff_day, int diff_mo, int diff_yr)
    {
    int carry;

    /** Do the add **/
    dt->Part.Second = obj_internal_DateAddModAdd(dt->Part.Second, diff_sec, 60, &carry);
    diff_min += carry;
    dt->Part.Minute = obj_internal_DateAddModAdd(dt->Part.Minute, diff_min, 60, &carry);
    diff_hr += carry;
    dt->Part.Hour = obj_internal_DateAddModAdd(dt->Part.Hour, diff_hr, 24, &carry);
    diff_day += carry;

    /** Now add months and years **/
    dt->Part.Month = obj_internal_DateAddModAdd(dt->Part.Month, diff_mo, 12, &carry);
    diff_yr += carry;
    dt->Part.Year += diff_yr;

    /** Correct for jumping to a month with fewer days **/
    if (dt->Part.Day >= (obj_month_days[dt->Part.Month] + ((dt->Part.Month==1 && IS_LEAP_YEAR(dt->Part.Year+1900))?1:0)))
	{
	dt->Part.Day = (obj_month_days[dt->Part.Month] + ((dt->Part.Month==1 && IS_LEAP_YEAR(dt->Part.Year+1900))?1:0)) - 1;
	}

    /** Adding days is more complicated **/
    while (diff_day > 0)
	{
	dt->Part.Day++;
	if (dt->Part.Day >= (obj_month_days[dt->Part.Month] + ((dt->Part.Month==1 && IS_LEAP_YEAR(dt->Part.Year+1900))?1:0)))
	    {
	    dt->Part.Day = 0;
	    dt->Part.Month = obj_internal_DateAddModAdd(dt->Part.Month, 1, 12, &carry);
	    dt->Part.Year += carry;
	    }
	diff_day--;
	}
    while (diff_day < 0)
	{
	if (dt->Part.Day == 0)
	    {
	    dt->Part.Day = (obj_month_days[obj_internal_DateAddModAdd(dt->Part.Month, -1, 12, &carry)] + ((dt->Part.Month==2 && IS_LEAP_YEAR(dt->Part.Year+1900))?1:0)) - 1;
	    dt->Part.Month = obj_internal_DateAddModAdd(dt->Part.Month, -1, 12, &carry);
	    dt->Part.Year += carry;
	    }
	else
	    {
	    dt->Part.Day--;
	    }
	diff_day++;
	}

    return 0;
    }
