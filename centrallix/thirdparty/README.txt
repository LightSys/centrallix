                ===== Centrallix Third-Party Provided Code =====

The code in this directory subtree is provided with Centrallix because it is
not (yet) universally available on all platforms we are supporting.  This
README.txt file provides information about these third-party components and
how to prep those components for inclusion in Centrallix.


                               ===== JSON-C =====

The json-c library provides JSON parsing and generation functionality.  It is
available on some platforms (CentOS 7) but not on others, such as CentOS 5 and
CentOS 6.

The json-c library is built using the standard automake and autoconf tools.

We make allowance for automake to not be installed on the build host by
including the json-c distribution files in the 'dist' and 'tests/dist'
directories.  If autoreconf is not available on the system, then the main
Centrallix makefile copies in those distribution files rather than re-running
a nonexistent autoreconf.  Otherwise, autoreconf is run to ensure the build
files are all consistent for the platform.

TO PREP A NEW JSON-C:

    * Download the 'nodocs' version of json-c, and unpack it into this
      centrallix/thirdparty/ directory, making sure the version number is
      in the directory name.  

    * Move the following distribution-provided files into their corresponding
      'dist' subdirectories:

        Makefile.in	    ( --> dist/Makefile.in)
	aclocal.m4	    ( --> dist/aclocal.m4)
	config.h.in	    ( --> dist/config.h.in)
	configure	    ( --> dist/configure)
	tests/Makefile.in   ( --> tests/dist/Makefile.in)

    * Change the ownership and file modes accordingly to be owned by the build
      user and writable only by the file owner.

    * Add all of the files to Git.

    * The previous version(s) of json-c can be removed once compatibility is
      ensured.  The Centrallix makefile will automatically use the newest
      version of json-c available in thirdparty/ (assuming the version numbers
      continue to be consistent from one release to another).
