#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "mtask.h"
#include "mtlexer.h"
#include "obj.h"
#include "centrallix.h"
#include <readline/readline.h>
#include <readline/history.h>

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
/* Module:	test_obj.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	November 1998                                           */
/*									*/
/* Description:	This module provides command-line access to the OSML	*/
/*		for testing purposes.  It does not provide a network	*/
/*		interface and does not include the DHTML generation	*/
/*		subsystem when compiled.				*/
/*									*/
/*		THIS MODULE IS **NOT** SECURE AND SHOULD NEVER BE USED	*/
/*		IN PRODUCTION WHERE THE DEVELOPER IS NOT CONTROLLING	*/
/*		ALL ASPECTS OF INPUTS AND DATA BEING HANDLED.  FIXME ;)	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: test_obj.c,v 1.9 2002/06/09 23:44:45 nehresma Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/test_obj.c,v $

    $Log: test_obj.c,v $
    Revision 1.9  2002/06/09 23:44:45  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.8  2002/06/01 19:08:46  mattphillips
    A littl ebit of code cleanup...  getting rid of some compiler warnings.

    Revision 1.7  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.

    Revision 1.6  2002/02/14 01:05:07  gbeeley
    Fixed test_obj so that it works with the new config file stuff.

    Revision 1.5  2001/11/12 20:43:43  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.4  2001/10/02 16:24:24  gbeeley
    Changed %f printf conversion to more intuitive %g.

    Revision 1.3  2001/09/28 19:06:18  gbeeley
    Fixed EOF handling on readline()==NULL; fixed "query" command to use inbuf
    instead of sbuf.

    Revision 1.2  2001/09/18 15:39:23  mattphillips
    Added GNU Readline support.  This adds full commandline editting support, and
    scrollback support.  No tab completion yet, though.

    NOTE: The readline and readline-devel packages (for RPM based distributions)
    are required for building now.

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

void* my_ptr;

#define BUFF_SIZE 1024

void
start(void* v)
    {
    pObjSession s;
    char sbuf[BUFF_SIZE];
    static char* inbuf = (char *)NULL;
    char prompt[1024];
    char* ptr;
    char cmdname[64];
    pObject obj,child_obj,to_obj;
    pObjQuery qy;
    char* filename;
    char* filetype;
    char* fileannot;
    int cnt,i;
    char* attrname;
    char* methodname;
    int type;
    DateTime dtval;
    pDateTime dt;
    pIntVec iv;
    pStringVec sv;
    char* stringval;
    int intval;
    int is_where, is_orderby;
    char* user;
    char* pwd;
    pFile StdOut;
    pLxSession ls = NULL;
    char where[256];
    char orderby[256];
    double dblval;
    pMoneyType m;
    MoneyType mval;
    pObjData pod;
    int use_srctype;
    char mname[64];
    char mparam[256];
    char* mptr;

	/** Initialize. **/
	cxInitialize();

	/** Disable tab complete until we have a function to do something useful with it. **/
	rl_bind_key ('\t', rl_insert);

	/** Authenticate **/
	user = readline("Username: ");
	pwd = getpass("Password: ");
	if (mssAuthenticate(user,pwd) < 0)
	    puts("Warning: auth failed, running outside session context.");
	free(user);
	StdOut = fdOpen("/dev/tty", O_RDWR, 0600);

	/** Open a session **/
	s = objOpenSession("/");

	/** Loop, putting prompt and getting commands **/
	while(1)
	    {
	    is_where = 0;
	    sprintf(prompt,"OSML:%.1000s> ",objGetWD(s));

	    /** If the buffer has already been allocated, return the memory to the free pool. **/
	    if (inbuf)
		{
		free (inbuf);
		inbuf = (char *)NULL;
		}   

	    /** Get a line from the user using readline library call. **/
	    inbuf = readline (prompt);   

	    /** If the line has any text in it, save it on the readline history. **/
	    if (inbuf && *inbuf)
		add_history (inbuf);

	    /** If inbuf is null (end of file, etc.), exit **/
	    if (!inbuf)
	        {
		printf("quit\n");
		thExit();
		}

	    if (ls) mlxCloseSession(ls);
	    ls = mlxStringSession(inbuf,MLX_F_ICASE);
	    if (mlxNextToken(ls) != MLX_TOK_KEYWORD) continue;
	    ptr = mlxStringVal(ls,NULL);
	    if (!ptr) continue;
	    strcpy(cmdname,ptr);
	    mlxSetOptions(ls,MLX_F_IFSONLY);
	    if (mlxNextToken(ls) != MLX_TOK_STRING) ptr = NULL;
	    else ptr = mlxStringVal(ls,NULL);
	    mlxUnsetOptions(ls,MLX_F_IFSONLY);
	    if (!strcmp(cmdname,"cd"))
		{
		if (!ptr)
		    {
		    printf("Usage: cd <directory>\n");
		    continue;
		    }
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("cd: could not change to '%s'.\n",ptr);
		    continue;
		    }
		objSetWD(s,obj);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"query"))
	        {
		if (!ptr)
		    {
		    printf("Usage: query \"<query-text>\"\n");
		    continue;
		    }
		qy = objMultiQuery(s, inbuf + 6);
		if (!qy)
		    {
		    printf("query: could not open query!\n");
		    continue;
		    }
		while((obj=objQueryFetch(qy, O_RDONLY)))
		    {
		    for(attrname=objGetFirstAttr(obj);attrname;attrname=objGetNextAttr(obj))
		        {
			type = objGetAttrType(obj,attrname);
			switch(type)
			    {
			    case DATA_T_INTEGER:
			        if (objGetAttrValue(obj,attrname,POD(&intval)) == 1)
			            printf("Attribute: [%s]  INTEGER  NULL\n", attrname);
				else
			            printf("Attribute: [%s]  INTEGER  %d\n", attrname,intval);
				break;
			    case DATA_T_STRING:
			        if (objGetAttrValue(obj,attrname,POD(&stringval)) == 1)
			            printf("Attribute: [%s]  STRING  NULL\n", attrname);
				else
			            printf("Attribute: [%s]  STRING  \"%s\"\n", attrname,stringval);
			        break;
			    case DATA_T_DOUBLE:
			        if (objGetAttrValue(obj,attrname,POD(&dblval)) == 1)
				    printf("Attribute: [%s]  DOUBLE  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  DOUBLE  %g\n", attrname, dblval);
				break;
			    case DATA_T_DATETIME:
			        if (objGetAttrValue(obj,attrname,POD(&dt)) == 1)
				    printf("Attribute: [%s]  DATETIME  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  DATETIME  %s\n", attrname, objDataToStringTmp(type, dt, 0));
				break;
			    case DATA_T_MONEY:
			        if (objGetAttrValue(obj,attrname,POD(&m)) == 1)
				    printf("Attribute: [%s]  MONEY  NULL\n", attrname);
				else
				    printf("Attribute: [%s]  MONEY  %s\n", attrname, objDataToStringTmp(type, m, 0));
				break;
			    }
			}
		    objClose(obj);
		    }
		objQueryClose(qy);
		}
	    else if (!strcmp(cmdname,"annot"))
	        {
		if (!ptr)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    continue;
		    }
		obj = objOpen(s, ptr, O_RDWR, 0600, "system/object");
		if (!obj)
		    {
		    printf("annot: could not open '%s'.\n",ptr);
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls,NULL);
		objSetAttrValue(obj, "annotation", POD(&ptr));
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"list") || !strcmp(cmdname, "ls"))
		{
		is_where = 0;
		is_orderby = 0;
		if (ptr && !strcmp(ptr,"where"))
		    { 
		    is_where = 1;
		    ptr = "";
		    }
		if (ptr && !strcmp(ptr,"orderby"))
		    {
		    is_orderby = 1;
		    ptr = "";
		    }
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("list: could not open directory '%s'.\n",ptr);
		    continue;
		    }
		if (!is_where && !is_orderby)
		    {
		    if (mlxNextToken(ls) != MLX_TOK_KEYWORD) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    if (ptr && !strcmp(ptr,"where")) is_where = 1;
		    if (ptr && !strcmp(ptr,"orderby")) is_orderby = 1;
		    }
		if (is_where)
		    {
		    if (mlxNextToken(ls) != MLX_TOK_STRING) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    printf("where: '%s'\n",ptr);
		    strcpy(where,ptr);
		    if (mlxNextToken(ls) == MLX_TOK_STRING && !strcmp("orderby",mlxStringVal(ls,NULL)))
		        {
			mlxNextToken(ls);
			strcpy(orderby, mlxStringVal(ls,NULL));
			}
		    else
		        {
			orderby[0] = 0;
			}
		    qy = objOpenQuery(obj,where,orderby[0]?orderby:NULL,NULL,NULL);
		    }
		else if (is_orderby)
		    {
		    mlxNextToken(ls);
		    strcpy(orderby, mlxStringVal(ls,NULL));
		    qy = objOpenQuery(obj,NULL,orderby,NULL,NULL);
		    }
		else
		    {
		    qy = objOpenQuery(obj,"",NULL,NULL,NULL);
		    }
		if (!qy)
		    {
		    objClose(obj);
		    printf("list: object '%s' doesn't support directory queries.\n",ptr);
		    continue;
		    }
		while(NULL != (child_obj = objQueryFetch(qy,O_RDONLY)))
		    {
		    if (objGetAttrValue(child_obj,"name",POD(&filename)) >= 0)
			{
			objGetAttrValue(child_obj,"outer_type",POD(&filetype));
			objGetAttrValue(child_obj,"annotation",POD(&fileannot));
			printf("%-32.32s  %-32.32s    %s\n",filename,fileannot,filetype);
			}
		    objClose(child_obj);
		    }
		objQueryClose(qy);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"show"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/object");
		if (!obj)
		    {
		    printf("show: could not open object '%s'\n",ptr);
		    continue;
		    }
		puts("Attributes:");
		attrname = objGetFirstAttr(obj);
		while(attrname)
		    {
		    type = objGetAttrType(obj,attrname);
		    switch(type)
			{
			case DATA_T_INTEGER:
			    if (objGetAttrValue(obj,attrname,POD(&intval)) == 1)
			        printf("  %20.20s: NULL\n",attrname);
			    else
			        printf("  %20.20s: %d\n",attrname, intval);
			    break;

			case DATA_T_STRING:
			    if (objGetAttrValue(obj,attrname,POD(&stringval)) == 1)
			        printf("  %20.20s: NULL\n",attrname);
			    else
			        printf("  %20.20s: \"%s\"\n",attrname, stringval);
			    break;

			case DATA_T_DATETIME:
			    if (objGetAttrValue(obj,attrname,POD(&dt)) == 1 || dt==NULL)
			        printf("  %20.20s: NULL\n",attrname);
			    else
			        printf("  %20.20s: %2.2d-%2.2d-%4.4d %2.2d:%2.2d:%2.2d\n", 
				    attrname,dt->Part.Month+1, dt->Part.Day+1, dt->Part.Year+1900,
				    dt->Part.Hour, dt->Part.Minute, dt->Part.Second);
			    break;
			
			case DATA_T_DOUBLE:
			    if (objGetAttrValue(obj,attrname,POD(&dblval)) == 1)
			        printf("  %20.20s: NULL\n",attrname);
			    else
			        printf("  %20.20s: %g\n", attrname, dblval);
			    break;

			case DATA_T_MONEY:
			    if (objGetAttrValue(obj,attrname,POD(&m)) == 1 || m == NULL)
			        printf("  %20.20s: NULL\n", attrname);
			    else
			        printf("  %20.20s: %s\n", attrname, objDataToStringTmp(DATA_T_MONEY, m, 0));
			    break;

			case DATA_T_INTVEC:
			    if (objGetAttrValue(obj,attrname,POD(&iv)) == 1 || iv == NULL)
			        {
			        printf("  %20.20s: NULL\n",attrname);
				}
			    else
			        {
			        printf("  %20.20s: ", attrname);
				for(i=0;i<iv->nIntegers;i++) 
				    printf("%d%s",iv->Integers[i],(i==iv->nIntegers-1)?"":",");
				printf("\n");
				}
			    break;

			case DATA_T_STRINGVEC:
			    if (objGetAttrValue(obj,attrname,POD(&sv)) == 1 || sv == NULL)
			        {
			        printf("  %20.20s: NULL\n",attrname);
				}
			    else
			        {
			        printf("  %20.20s: ",attrname);
				for(i=0;i<sv->nStrings;i++) 
				    printf("\"%s\"%s",sv->Strings[i],(i==sv->nStrings-1)?"":",");
				printf("\n");
				}
			    break;

			default:
			    printf("  %20.20s: <unknown type>\n",attrname);
			    break;
			}
		    attrname = objGetNextAttr(obj);
		    }
		puts("\nMethods:");
		methodname = objGetFirstMethod(obj);
		if (methodname)
		    {
		    while(methodname)
			{
			printf("  %20.20s()\n",methodname);
			methodname = objGetNextMethod(obj);
			}
		    }
		else
		    {
		    puts("  (no methods)");
		    }
		puts("");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"print"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "text/plain");
		if (!obj)
		    {
		    printf("print: could not open object '%s'\n",ptr);
		    continue;
		    }
		while((cnt=objRead(obj, sbuf, 255, 0, 0)) >0)
		    {
		    sbuf[cnt] = 0;
		    write(1,sbuf,cnt);
		    }
		puts("");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"copy"))
	        {
		if (!ptr)
		    {
		    printf("copy: must specify <dsttype/srctype> <source> <destination>\n");
		    continue;
		    }
		if (!strcmp(ptr,"srctype")) use_srctype = 1; else use_srctype = 0;
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy: must specify <dsttype/srctype> <source> <destination>\n");
		    continue;
		    }
		mlxCopyToken(ls, sbuf, 1023);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy: must specify <dsttype/srctype> <source> <destination>\n");
		    continue;
		    }
		ptr = mlxStringVal(ls, NULL);
		if (use_srctype)
		    {
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, "application/octet-stream");
		    if (!obj) continue;
		    objGetAttrValue(obj, "inner_type", POD(&stringval));
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT, 0600, stringval);
		    if (!to_obj)
		        {
			objClose(obj);
			continue;
			}
		    }
		else
		    {
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT, 0600, "application/octet-stream");
		    if (!to_obj) continue;
		    objGetAttrValue(to_obj, "inner_type", POD(&stringval));
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, stringval);
		    if (!obj)
		        {
			objClose(to_obj);
			continue;
			}
		    }
		while((cnt = objRead(obj, sbuf, 255, 0, 0)) > 0)
		    {
		    objWrite(to_obj, sbuf, cnt, 0, 0);
		    }
		objClose(obj);
		objClose(to_obj);
		}
	    else if (!strcmp(cmdname,"delete"))
	        {
		if (!ptr)
		    {
		    printf("delete: must specify object.\n");
		    continue;
		    }
		if (objDelete(s, ptr) < 0)
		    {
		    printf("delete: failed.\n");
		    continue;
		    }
		}
	    else if (!strcmp(cmdname,"create"))
	        {
		if (!ptr) 
		    {
		    printf("create: must specify object.\n");
		    continue;
		    }
		obj = objOpen(s, ptr, O_RDWR | O_CREAT, 0600, "system/object");
		if (!obj)
		    {
		    printf("create: could not create object.\n");
		    mssPrintError(StdOut);
		    continue;
		    }
		puts("Enter attributes, blank line to end.");
		while(1)
		    {
		    char* slbuf = readline("");
		    strncpy(sbuf, slbuf, BUFF_SIZE-1);
		    sbuf[BUFF_SIZE-1] = 0;
		    if (sbuf[0] == 0) break;
		    attrname = strtok(sbuf,"=");
		    stringval = strtok(NULL,"=");
		    while(*stringval == ' ') stringval++;
		    while(attrname[strlen(attrname)-1] == ' ') attrname[strlen(attrname)-1]=0;
		    type = objGetAttrType(obj,attrname);
		    if (type == -1 || type == DATA_T_UNAVAILABLE)
		        {
		        if (*stringval >= '0' && *stringval <= '9')
		            {
			    intval = strtol(stringval,NULL,10);
			    objSetAttrValue(obj,attrname,POD(&intval));
			    }
		        else
		            {
			    objSetAttrValue(obj,attrname,POD(&stringval));
			    }
			}
		    else
		        {
			switch(type)
			    {
			    case DATA_T_INTEGER: intval = objDataToInteger(DATA_T_STRING,(void*)stringval,NULL); pod = POD(&intval); break;
			    case DATA_T_STRING: pod = POD(&stringval); break;
			    case DATA_T_DOUBLE: dblval = objDataToDouble(DATA_T_STRING,(void*)stringval); pod = POD(&dblval); break;
			    case DATA_T_DATETIME: dt = &dtval; objDataToDateTime(DATA_T_STRING,(void*)stringval,dt,NULL); pod = POD(&dt); break;
			    case DATA_T_MONEY: m = &mval; objDataToMoney(DATA_T_STRING,(void*)stringval, m); pod = POD(&m); break;
			    }
			objSetAttrValue(obj,attrname,pod);
			}
		    }
		if (objClose(obj) < 0) mssPrintError(StdOut);
		}
	    else if (!strcmp(cmdname,"quit"))
		{
		break;
		}
	    else if (!strcmp(cmdname,"exec"))
	        {
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "application/octet-stream");
		if (!obj)
		    {
		    printf("exec: could not open object '%s'\n",ptr);
		    continue;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    continue;
		    }
		mlxCopyToken(ls, mname, 63);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    continue;
		    }
		mlxCopyToken(ls, mparam, 255);
		mptr = mparam;
		objExecuteMethod(obj, mname, POD(&mptr));
		objClose(obj);
		}
	    else
		{
		printf("Unknown command '%s'\n",cmdname);
		}
	    }

	/*objCloseSession(s);*/

    thExit();
    }


int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, "/usr/local/etc/centrallix.conf");
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"hc:q")) > 0)
	    {
	    switch (ch)
	        {
		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	printf("Usage:  test_obj [-c <config-file>]\n");
				exit(0);

		case '?':
		default:	printf("Usage:  test_obj [-c <config-file>]\n");
				exit(1);
		}
	    }

    mtInitialize(0, start);
    return 0;
    }
