
	/* The maximum number of bytes in a pathname argument. */
	const MNTPATHLEN = 1024;

	/* The maximum number of bytes in a name argument. */
	const MNTNAMLEN = 255;

	/* The size in bytes of the opaque file handle. */
	const FHSIZE = 32;

	typedef opaque fhandle[FHSIZE];
	union fhstatus switch (unsigned status) {
		case 0:
			fhandle directory;
		default:
			void;
	};

	typedef string dirpath<MNTPATHLEN>;
	typedef string name<MNTNAMLEN>;

	struct mountlist_item {
		name      hostname;
		dirpath   directory;
		mountlist_item *nextentry;
	};
	typedef mountlist_item *mountlist;

	struct groups_item {
		name grname;
		groups_item *grnext;
	};
	typedef groups_item *groups;

	struct exportlist_item {
		dirpath filesys;
		groups groups;
		exportlist_item *next;
	};
	typedef exportlist_item *exportlist;

	/*
	* Protocol description for the mount program
	*/
	program MOUNTPROG {
			/*
			* Version 1 of the mount protocol used with
			* version 2 of the NFS protocol.
			*/
			version MOUNTVERS {

					void
					MOUNTPROC_NULL(void) = 0;

					fhstatus
					MOUNTPROC_MNT(dirpath) = 1;

					mountlist
					MOUNTPROC_DUMP(void) = 2;

					void
					MOUNTPROC_UMNT(dirpath) = 3;

					void
					MOUNTPROC_UMNTALL(void) = 4;

					exportlist
					MOUNTPROC_EXPORT(void)  = 5;
			} = 1;
	} = 100005;


