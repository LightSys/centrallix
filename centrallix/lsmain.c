#include <stdio.h>
#include <string.h>
//#ifdef HAVE_LIBGEN_H
#include <libgen.h>
//#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#include "cxlib/mtask.h"
#include "obj.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "stparse.h"
#include "cxlib/mtlexer.h"
#include "cxlib/strtcpy.h"
#include <errno.h>
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
	if (cxDriverInit() < 0) thExit();
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
		"\t-d          become a service (fork into the background),\n"
		"\t-q          initialize quietly,\n"
		"\t-c <file>   use <file> for configuration,\n"
		"\t-p <file>   use <file> for the PID file.\n"
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
    char pidbuf[16];

	nmInitialize();				/* memory manager */

	/** Seed random number generator **/
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
	    {
	    srand(time(NULL));
	    srand48(time(NULL));
	    }
	else
	    {
	    if (read(fd, &seed, sizeof(seed)) != sizeof(seed))
		{
		srand(time(NULL));
		srand48(time(NULL));
		}
	    else
		{
		srand(seed);
		srand48(seed);
		}
	    close(fd);
	    }

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, CENTRALLIX_CONFIG);
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	CxGlobals.ArgV = argv;
	CxGlobals.Flags = 0;

	/** Check for config file options on the command line **/
#ifdef HAVE_BASENAME
	name = (char*)basename(argv[0]);
#else
	name = argv[0];
#endif
	while ((ch=getopt(argc,argv,"Vhdc:qp:")) > 0)
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

		case 'c':	strtcpy(CxGlobals.ConfigFileName, optarg, sizeof(CxGlobals.ConfigFileName));
				break;

		case 'd':	CxGlobals.QuietInit = 1;
				CxGlobals.Flags |= CX_F_SERVICE;
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'h':	usage(name);
				exit(0);
		
		case 'p':	strtcpy(CxGlobals.PidFile, optarg, sizeof(CxGlobals.PidFile));
				break;

		case '?':
		default:	usage(name);
				exit(1);
		}
	    }

	/** Become a background service? **/
	if (CxGlobals.Flags & CX_F_SERVICE)
	    {
	    go_background();
	    if (CxGlobals.PidFile[0])
		{
		/** Open the pid file **/
		fd = open(CxGlobals.PidFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (fd < 0)
		    {
		    fprintf(stderr, "Could not open pid file '%s': %s\n", CxGlobals.PidFile, strerror(errno));
		    return 1;
		    }

		/** Write our pid to the file **/
		snprintf(pidbuf, sizeof(pidbuf), "%d\n", getpid());
		if (write(fd, pidbuf, strlen(pidbuf)) < 0)
		    {
		    fprintf(stderr, "Unable to write to pid file '%s': %s\n", CxGlobals.PidFile, strerror(errno));
		    return 1;
		    }
		close(fd);
		}
	    }
	else
	    {
	    /** Zero it out if we didn't write to the file **/
	    CxGlobals.PidFile[0] = '\0';
	    }

	/** Init the multithreading module to start the first thread **/
	/** 'start' is the name of the function to be the first thread **/
	mtInitialize((CxGlobals.QuietInit)?MT_F_QUIET:0, start);

    return 0; /* never reached */
    }

