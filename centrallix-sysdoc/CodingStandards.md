Title:    C and JavaScript Coding Standards for the Centrallix project
Author:   Greg Beeley
Date:     27-June-2001
License:  Copyright (C) 2001 LightSys Technology Services.  See LICENSE.txt.
-------------------------------------------------------------------------------

OVERVIEW

    This document describes the community coding standards and expectations for
    the Centrallix project.  The goal of this document is to provide helpful
    guidelines to help make the project more maintainable in the long haul and
    to facilitate set some standards for this project to help all developers
    involved.  This is not meant to be a final document by any means - comments
    and suggestions are welcome, and since this document itself is in CVS... :)


CODING STYLE GUIDELINES

    The core of Centrallix uses my (Greg's) coding style, which is similar,
    but not identical to, the "whitesmith" style that GNU Emacs recognizes.  
    That style is designed primarily with readability in mind.

    Coding style is as much an art form as writing the code itself.  If you
    add a new module or file to Centrallix, don't feel you must follow the
    core coding style, although choosing to use the core coding style is a
    good thing <grin>.  In any case, be sure to use a style that is clear and
    consistent.  I'd recommend that we all use no fewer than four spaces when
    indenting - using two can make the code too difficult to read, although
    the '%' function in VI does help <grin>....  

    You can find more information on my coding style in the additional sysdoc
    documentation file "BeeleyCodingStyle.txt" that is included in this
    distribution.  I chose to separate that document from this one.
    
    No matter what style you choose to use, please do not use "indent" (or any
    other tool for that matter) to reformat any existing code (unless you wrote 
    the code AND no one else is going to be modifying it in CVS; and even then
    I would strongly discourage reformatting the code with indent).  It is one 
    thing to be sending out patches that have tons of "false diffs" in them 
    from reformatted files.  I'm not worried about that.  What does concern me
    is the possibility of each developer reformatting files when he/she works
    on them, thus severely impairing CVS's ability to merge conflicts in an
    intelligent way.  It will also become quite nontrivial to determine what
    changes need to be merged when manually merging such files.  And that would
    be a headache for all of us :)

    Furthermore, at some point I'd like to get one or two people on the project
    whose main contribution is to sift through the code looking for security
    problems.  The diffs can help them know where to look for recent changes to
    the code base.

    Along the same lines, it'd help us all to keep a file's coding style some-
    what consistent when making changes to a file.  I know, I know - I've done
    that (multiple styles in one file) before and am just as guilty of it as 
    the next person.  But that was on a project where I wasn't responsible to a
    group of developers.  In this case, we are <sigh>....


CONVENTIONS AND OTHER FUN STUFF

    1.  Conventions for struct/union definitions:

	Centrallix makes heavy use of typedefs for naming structures and unions
	and such.  A normal typedef declaration for a structure looks like 
	this:

        typedef struct _EL
	    {
	    struct _EL*	Next;
	    int		part1;
	    int		part2;
	    }
	    Element, *pElement;

	I intentionally have used the "pElement" syntax for showing that the 
	type is a pointer.  This avoids the neverending squabble over the more
	readable "char* ptr;" syntax and the more correct "char *ptr;" 
	syntax <grin>....

	You are encouraged to use the "pXxxyyy" style in your own Centrallix
	code modules.  That helps keep things consistent, but as with things
	before, if you already have a well-developed coding style, feel free
	to use that instead.  The indentation is up to you (as before).


    2.	Return values for C functions in Centrallix:

	There are many conventions and ideas on how to use return values 
	from functions.  Here, the established approach used in Centrallix
	is documented:

	The main purpose of the return value for a procedural function (a
	function which "accomplishes some task") is to indicate the status
	of the task the function performed (did it fail or succeed?) and
	if it failed, possibly why the task failed.  A pure function (which
	computes a value without accessing or modifying external resources)
	has a return value which is the result of the computation.  That
	distinction is important in Centrallix (procedural function results
	generally affect the flow of the program, whereas pure function
	results generally only affect program flow when the results are a 
	part of another expression).

	First, if a procedural function returns a pointer, it should return
	NULL if an error occurs.  In such an event, it should also, if
	need be, log an error using mssError() or mssErrorErrno().  Note
	that in some cases it is better for a procedure which generates a
	pointer to pass that pointer through a reference argument rather
	than through the return value, so that better status information
	is possible through the return value (other than just "failed").

	Second, if a procedural function normally returns a non-negative
	integer value of some kind, it should return a negative value if
	an error occurrs.  To simply indicate "failed", return negative
	one (-1).  In the future, other more error-specific values may be
	possible (such as the way Linux kernel functions internally handle
	errors).

	Third, if a procedural function does not naturally return a value
	at all, it should return 0 on success and a negative value on 
	failure (as in the above case) for consistency.  Boolean values
	indicating failure/success status should be avoided - they are to
	be used for the result of boolean computations in pure functions.


MODULE PREFIXES

    Each Centrallix module must have a prefix.  The prefix is a two-to-four
    letter abbreviation for the module.  Some registry functions require a
    prefix to be given, and the MSS module's error logging routines require
    a prefix as a function parameter.  A list of prefixes can be found in this
    documentation package.  For more information on using module prefixes, 
    consult the "Beeley's coding style" document.


SECURE CODING RECOMMENDATIONS

    Here are a few tips for improving the security of Centrallix by taking 
    care during development.

    1.  When using the sprintf() routines, take the following precautions:

        a.  Use snprintf() to specify the size of the target buffer.
	b.  ALWAYS ALWAYS supply a format string.  Never do a sprintf(buf,str)!
	c.  Don't let the user control the format string.
	d.  Specify maximum lengths for embedded %s strings as in %.128s for
	    a 128-character maximum string.

    2.  When copying strings,
        
	a.  Don't use strcpy() unless you are copying a constant text string
	    into the buffer.
	b.  strncpy() fills the remainder of the buffer with zeros, even when
	    unnecessary.  memccpy() is an alternative that does not.  Neither
	    function guarantees that the destination will be null-terminated.
	c.  Watch out for other memory copying routines that don't respect 
	    array boundaries - strcat is another one :)

    3.  When logging to syslog(),

        a.  MAKE SURE YOU SUPPLY A FORMAT STRING!
	b.  Specify maximum lengths for embedded %s strings, as in sprintf.
	c.  Don't let the user control the format string.

    4.  When reading from a file or from the network,

        a.  NEVER USE fscanf().  It is a pain.
	b.  NEVER USE gets() when running with privileges different than those
	    of the connecting/calling user (this is most of the time).
	c.  Never assume that the remote end, whether server or client, will
	    honor the protocol specification.  Check for errors and respond
	    accordingly.  Remember - someone could just telnet to the port
	    and type random data if they really wanted to :)
	d.  Don't blindly malloc() memory for data as it comes in without 
	    taking care to prevent denial-of-service due to memory exhaustion.
	    This is particularly true of network connections.

    5.  When creating/opening/writing files,

        a.  If the directory is world-writable, such as /tmp or /var/spool/mail
	    you MUST use lstat() or similar to check for symlink existence
	    first.  Then open the file and lstat() again to make sure you didnt
	    just follow a link.  That introduces a race condition - a better 
	    way is to use the O_NOFOLLOW open() argument, although this only 
	    works in recent Linux and BSD systems.
	b.  Generally refuse to write secure data to a file that has insecure
	    permissions.
	c.  For temporary files, don't use predictable names.  This also helps
	    prevent the symlink problem.  Another solution is to initially 
	    create a directory in /tmp that only you can write to (and to do
	    the appropriate checks when creating it), and then from that point
	    on, only create tmp files in that directory.

    6.  When keeping stateful information in a table/array/hashtable/etc.,

        a.  Make sure that a connecting user, particularly an unauthenticated
	    user, can't endlessly cause entries to be added to the table 
	    without some sort of mechanism to prune old entries and keep the
	    table down to size.


MTASK DEVELOPMENT ISSUES

    Centrallix uses the MTASK multithreading library, which is in the 
    Centrallix-lib package.  This has several implications for the actual
    implementation in C.

    1.  The stack size is limited.  The default currently set on MTASK is 
        about 31K.  Because of MTASK's way of doing the stack in a portable
	manner, the stack sizes for each thread are fixed at this configurable
	amount.  This implies:

	a.  No large variables on the stack.  If you need a 1024-byte array,
	    declare a pointer and nmMalloc() it.  nmMalloc caches blocks of
	    that size, so it should be fast enough.  By no means should you 
	    declare a variable that large on the stack.
	b.  Deep recursion must be avoided.  The EXP module has an example of
	    a parser that avoids unnecessary recursion.
	c.  Permitting remote-user control over recursion depth could result
	    in a security problem by causing one thread to overwrite a return
	    pointer for another thread.  There is an option in MTASK to 
	    mprotect() a 1K region between stacks - so that's an option if
	    we deem it necessary.

    2.  I am very open to moving the MTASK framework to be based on pthreads
        under the hood - we'd probably keep the MTASK api, but most modules
	would probably have to be overhauled to allow for re-entrancy and
	preemptive multithreading.  The programming model with the non-
	preemptive MTASK environment is much much easier to deal with.  MTASK
	was developed when the Linux kernel didn't support threading very 
	well, and also at a time when compatibility with SCO OpenServer and
	with SunOS was desired (MTASK has been used successfully on both
	unmodified).  The situation with threading has changed a good bit since
	then and I am open to suggestions/ideas.  One idea might be to keep the
	nonpreemptive model but to spin off critical and separate processing
	into separate pthread-type threads, such as for doing asynchronous I/O,
	script execution, and such.  However, moving to a full preemptive 
	threading model may very well be worth the effort.

    3.  MTASK will only do a thread switch when one of its various routines
        is called - you do not have to worry about thread swaps between 
	arbitrary instructions (unless you have a signal handler installed that
	calls MTASK's functions without first thLock()ing the scheduler).
	This makes the programming model much simpler, but it also means we
	have a bit of work to do before the whole system is ready for a pthread
	based environment.  Note that nonpreemptive threading is generally an
	OK thing when you have control over the behavior of the threads.  It
	is a bad thing in a general purpose OS when you do not have control
	over the behavior of the applications (a certain legacy OS comes to
	mind here....)

    4.  For those who are interested, MTASK is implemented using a very simple
        setjmp()/longjmp() mechanism.  Stack positioning is done via recursive
	calls to a function with a very large automatic variable (32k in this
	case).

    5.  Some C library functions must be used with caution when in a multi-
        threaded environment, even a nonpreemptive one.  strtok() is one such
	routine - make sure you strtok() your way through the whole string 
	before calling any of the MTASK functions, otherwise the context may
	switch.  This also applies to other library routines that return
	pointers to static storage - make sure you copy the data out and into
	appropriate memory before calling any MTASK library function.  Other-
	wise, you MUST thLock() the scheduler and then thUnlock() when done
	using strtok() or whatever other routine is needed.  Note that calling
	a syGetSem or similar function that blocks while the scheduler is
	thLock()ed will cause a deadlock, which currently exits the process.

    6.  DO NOT OPTIMIZE (-O) MTASK WHEN COMPILING.  It may cause the stack-
        separating local variable to get optimized out, which will cause the
	whole system to break :)

-------------------------------------------------------------------------------
