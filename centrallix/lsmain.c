#include <stdio.h>
#include <string.h>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#include "mtask.h"
#include "obj.h"
#include "xstring.h"
#include "xarray.h"
#include "xhash.h"
#include "stparse.h"
#include "mtlexer.h"
#ifndef CENTRALLIX_CONFIG
#define CENTRALLIX_CONFIG "/usr/local/etc/centrallix.conf"
#endif

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
/* Module: 	lsmain.c               					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	December 4, 1998 					*/
/* Description:	Main startup code to start the Centrallix as a server	*/
/*		process waiting for network connections.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: lsmain.c,v 1.29 2003/06/03 20:29:10 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/lsmain.c,v $

    $Log: lsmain.c,v $
    Revision 1.29  2003/06/03 20:29:10  gbeeley
    Fix to CSV driver due to uninitialized memory causing a segfault when
    opening CSV files from time to time.

    Revision 1.28  2003/05/30 17:39:47  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.27  2003/04/04 05:56:53  gbeeley
    Forgot to seed the random number generator (gulp)

    Revision 1.26  2003/03/31 22:54:11  jorupp
     * I really ought to make clean before I commit....

    Revision 1.25  2003/03/10 15:41:38  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.

    Revision 1.24  2002/11/22 19:29:36  gbeeley
    Fixed some integer return value checking so that it checks for failure
    as "< 0" and success as ">= 0" instead of "== -1" and "!= -1".  This
    will allow us to pass error codes in the return value, such as something
    like "return -ENOMEM;" or "return -EACCESS;".

    Revision 1.23  2002/09/27 22:26:03  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.22  2002/08/16 03:09:44  jorupp
     * added the ability to disable the dynamic loading modules -- needed under cygwin
       -- centrallix now compiles and runs under cygwin (without dynamic loading modules)

    Revision 1.21  2002/08/12 09:16:26  mattphillips
    Added -V for version, and more helpful (and standard) usage message.

    Revision 1.20  2002/07/23 15:57:32  mattphillips
    - Modify the daemonizing routine based on
      http://www.unixguide.net/unix/programming/1.7.shtml (information from David
      Miller)

    Revision 1.19  2002/07/18 04:38:13  mattphillips
    Added '-d' option to centrallix to fork centrallix into the background on
    startup.  Also adding final support for the centrallix init script.

    Revision 1.18  2002/07/11 20:08:00  lkehresman
    Put quotes around the default path.  Was throwing parse errors otherwise.

    Revision 1.17  2002/06/13 15:21:04  mattphillips
    Adding autoconf support to centrallix

    Revision 1.16  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.

    Revision 1.15  2002/04/25 03:13:50  jorupp
     * added label widget
     * bug fixes in form and osrc

    Revision 1.14  2002/03/23 06:26:49  gbeeley
    Added BDQS network listener.  Be sure to cvs update the centrallix-os
    module to get a fresh copy of the centrallix.conf with the net_bdqs
    section in it, and be sure to cvs update the centrallix-lib module, as
    this module depends on it.

    Revision 1.13  2002/03/16 02:54:59  bones120
    This might help...

    Revision 1.12  2002/03/13 19:48:46  gbeeley
    Fixed a window-dragging issue with nested html windows.  Added the
    dropdown widget to lsmain.c.  Updated changelog.

    Revision 1.11  2002/03/09 02:42:01  bones120
    Initial commit of the spinner box.

    Revision 1.10  2002/03/09 02:38:48  jheth
    Make OSRC work with Form - Query at least

    Revision 1.9  2002/03/08 02:07:13  jorupp
    * initial commit of alerter widget
    * build callback listing object for form
    * form has many more of it's callbacks working

    Revision 1.8  2002/03/06 23:30:35  lkehresman
    Added form status widget.

    Revision 1.7  2002/02/22 23:48:39  jorupp
    allow editbox to work without form, form compiles, doesn't do much

    Revision 1.6  2002/02/14 00:55:20  gbeeley
    Added configuration file centrallix.conf capability.  You now MUST have
    this file installed, default is /usr/local/etc/centrallix.conf, in order
    to use Centrallix.  A sample centrallix.conf is found in the centrallix-os
    package in the "doc/install" directory.  Conf file allows specification of
    file locations, TCP port, server string, auth realm, auth method, and log
    method.  rootnode.type is now an attribute in the conf file instead of
    being a separate file, and thus is no longer used.

    Revision 1.5  2001/11/12 20:43:43  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.4  2001/11/03 02:09:54  gbeeley
    Added timer nonvisual widget.  Added support for multiple connectors on
    one event.  Added fades to the html-area widget.  Corrected some
    pg_resize() geometry issues.  Updated several widgets to reflect the
    connector widget changes.

    Revision 1.3  2001/10/23 00:25:09  gbeeley
    Added rudimentary single-line editbox widget.  No data source linking
    or anything like that yet.  Fixed a few bugs and made a few changes to
    other controls to make this work more smoothly.  Page widget still needs
    some key de-bounce and key repeat overhaul.  Arrow keys don't work in
    Netscape 4.xx.

    Revision 1.2  2001/10/02 20:45:03  gbeeley
    New build/subbuild system; fixed Makefile to work better with it...

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:50  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** start - this function is called by the MTASK module once the 
 *** mtInitialize() routine is finished intializing the first thread.  This
 *** is where processing really starts.  The purpose of this function is
 *** primarily to initialize the various modules and thus get things
 *** started.
 ***/
void
start(void* v)
    {

	/** Initialization. **/
	if (cxInitialize() < 0) thExit();
	cxHtInit();
	cxNetworkInit();

    thExit();
    }


/*** go_background - this function is used to drop a process into the
 *** background (become a daemon). The process will fork at this point, and
 *** the foreground process will exit successfully.
 ***/
static void
go_background()
    {
    pid_t pid;     

	pid = fork();

	if (pid < 0) 
	    {
	    printf("Can't fork\n");
	    exit(1);
	    }
	if (pid != 0)
	    {
	    /** parent **/
	    exit(0);
	    }

	/** child **/
	if (setsid() < 0) 
	    {
	    printf("setsid() error\n");
	    exit(1);
	    }

	/** fork again to lose our controlling terminal **/
	pid = fork();
	if (pid < 0) 
	    {
	    printf("Can't fork\n");
	    exit(1);
	    }
	if (pid != 0)
	    {
	    /** second parent **/
	    exit(0);
	    }

	/** second child **/
	chdir("/");
	umask(0);
	if (fclose(stdin)) 
	    {
	    printf("Can't close stdin\n");
	    exit(1);
	    }
	open("/dev/null", O_RDONLY);
	if (fclose(stdout)) 
	    {
	    printf("Can't close stdout\n");
	    exit(1);
	    }
	open("/dev/null", O_WRONLY);
	if (fclose(stderr)) 
	    {
	    printf("Can't close stderr\n");
	    exit(1);
	    }
	open("/dev/null", O_WRONLY);
    }

/*** usage - print out the usage message to stderr
 ***/
void
usage(char* name)
    {
	fprintf(stderr,
		"Usage: %s [-Vdqh] [-c configfile]\n\n"
		"\t-V          print version number,\n"
		"\t-d          daemonize (fork into the background),\n"
		"\t-q          initialize quietly,\n"
		"\t-c <file>   use <file> for configuration,\n"
		"\t-h          this message.\n\n"
		"E-mail bug reports to: %s\n\n",
		name, PACKAGE_BUGREPORT);
    }

/*** main - called from the C runtime to start the program.
 ***/
int 
main(int argc, char* argv[])
    {
    int ch;
    char* name;
    unsigned int seed;
    int fd;

	/** Seed random number generator **/
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
	    {
	    srand(time(NULL));
	    }
	else
	    {
	    if (read(fd, &seed, sizeof(seed)) != sizeof(seed))
		{
		srand(time(NULL));
		}
	    else
		{
		srand(seed);
		}
	    close(fd);
	    }

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, CENTRALLIX_CONFIG);
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;

	/** Check for config file options on the command line **/
#ifdef HAVE_BASENAME
	name = (char*)basename(argv[0]);
#else
	name = argv[0];
#endif
	while ((ch=getopt(argc,argv,"Vhdc:q")) > 0)
	    {
	    switch (ch)
	        {
		case 'V':	fprintf(stderr, 
					"%s\n"
					"Copyright (C) %s LightSys Technology Services, Inc.\n\n"
					"This is free software; see the source for copying conditions.  There is NO\n"
					"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n", 
					PACKAGE_STRING, YEARS);
				exit(0);

		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'd':	CxGlobals.QuietInit = 1;
				go_background();
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	usage(name);
				exit(0);

		case '?':
		default:	usage(name);
				exit(1);
		}
	    }

	/** Init the multithreading module to start the first thread **/
	/** 'start' is the name of the function to be the first thread **/
	mtInitialize(MT_F_NOYIELD, start);

    return 0; /* never reached */
    }

