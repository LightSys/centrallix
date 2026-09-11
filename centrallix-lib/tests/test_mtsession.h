/************************************************************************/
/* Centrallix Application Server System					*/
/* Centrallix Base Library						*/
/*									*/
/* Copyright (C) 2026 LightSys Technology Services, Inc.		*/
/*									*/
/* You may use these files and this library under the terms of the	*/
/* GNU Lesser General Public License, Version 2.1, contained in the	*/
/* included file "COPYING".						*/
/*									*/
/* Module:	test_mtsession.h					*/
/* Author:	Israel Fuller						*/
/* Creation:	September 9th, 2026					*/
/* Description:	Helpers shared by the mtsession tests, which all	*/
/* 		need an alternate password file to authenticate		*/
/* 		against.						*/
/************************************************************************/

#ifndef TEST_MTSESSION_H
#define TEST_MTSESSION_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** Tested module. **/
#include "mtsession.h"

/** Four salt bytes, the optimum size for mssGenCred(). **/
#define AUTH_SALT	"salt"

/** Big enough for any credential mssGenCred() produces. **/
#define CRED_SIZE	64

/** Big enough for a "username:credential" line, plus a few extra entries. **/
#define AUTH_FILE_SIZE	512

/*** Create an empty auth file with a name of its own in the temp directory.
 *** The caller removes it with tmpFileDeInit().
 ***
 *** @param path A buffer that receives the path of the new file.
 *** @param path_size The size of that buffer.
 *** @returns true if successful, false otherwise.
 ***/
static inline bool tmpFileInit(char* path, int path_size)
    {
    char* tmp_dir = getenv("TMPDIR");
    int fd;

	snprintf(path, path_size, "%s/test_mtsession_XXXXXX", tmp_dir ? tmp_dir : "/tmp");
	fd = mkstemp(path);
	if (fd < 0)
	    {
	    perror("tmpFileInit: could not create the temp file");
	    return false;
	    }
	close(fd);

    return true;
    }


/*** Replace the contents of a file.
 ***
 *** @param path The path of the file.
 *** @param contents The text to write, written verbatim.
 *** @returns true if successful, false otherwise.
 ***/
static inline bool tmpFileWrite(char* path, char* contents)
    {
    FILE* file = fopen(path, "w");

	if (!file)
	    {
	    perror("tmpFileWrite: could not open the file for writing");
	    return false;
	    }
	fputs(contents, file);
	if (fclose(file))
	    {
	    perror("tmpFileWrite: could not write the file");
	    return false;
	    }

    return true;
    }


/*** Remove a file created by tmpFileInit().
 ***
 *** @param path The path of the file.
 *** @returns true if successful, false otherwise.
 ***/
static inline bool tmpFileDeInit(char* path)
    {

	if (unlink(path))
	    {
	    perror("tmpFileDeInit: could not remove the temp file");
	    return false;
	    }

    return true;
    }


/*** Read the whole contents of a file into a buffer, NUL terminated.
 ***
 *** @param path The path of the file.
 *** @param buf A buffer to receive the contents.
 *** @param buf_size The size of that buffer, including the terminator.
 *** @returns The number of bytes read, or -1 on error.
 ***/
static inline int tmpFileRead(char* path, char* buf, int buf_size)
    {
    FILE* file = fopen(path, "r");
    int length;

	if (!file)
	    {
	    perror("tmpFileRead: could not open the file for reading");
	    return -1;
	    }
	length = fread(buf, 1, buf_size - 1, file);
	buf[length] = '\0';
	if (fclose(file))
	    {
	    perror("tmpFileRead: could not close the file");
	    return -1;
	    }

    return length;
    }


/*** Save stdout, then ignore all data written to it.
 ***
 *** @returns true if successful, false otherwise.
 ***/
static inline bool quietStart(int* saved_stdout)
    {
    int fd;

	fflush(stdout);
	*saved_stdout = dup(STDOUT_FILENO);
	if (*saved_stdout < 0)
	    {
	    perror("quietStart: could not save stdout");
	    return false;
	    }
	fd = open("/dev/null", O_WRONLY);
	if (fd < 0)
	    {
	    perror("quietStart: could not open /dev/null");
	    close(*saved_stdout);
	    return false;
	    }
	if (dup2(fd, STDOUT_FILENO) < 0)
	    {
	    perror("quietStart: could not redirect stdout");
	    close(*saved_stdout);
	    close(fd);
	    return false;
	    }
	close(fd);

    return true;
    }


/*** Restore stdout after quietStart().
 ***
 *** @returns true if successful, false otherwise.
 ***/
static inline bool quietEnd(int saved_stdout)
    {
    bool success = true;

	fflush(stdout);
	if (dup2(saved_stdout, STDOUT_FILENO) < 0)
	    {
	    perror("quietEnd: could not restore stdout");
	    success = false;
	    }
	close(saved_stdout);

    return success;
    }


/*** Build the credential that an auth file entry needs for a password.
 ***
 *** @param cred A buffer of at least CRED_SIZE bytes to receive it.
 *** @param password The password the credential must accept.
 *** @returns true if successful, false otherwise.
 ***/
static inline bool authCred(char* cred, char* password)
    {

	if (mssGenCred(AUTH_SALT, strlen(AUTH_SALT), password, cred, CRED_SIZE))
	    {
	    fprintf(stderr, "authCred: could not generate a credential\n");
	    return false;
	    }

    return true;
    }


/*** Write an auth file holding a single user.
 ***
 *** @param path The path of the file.
 *** @param username The user name for the entry.
 *** @param password The password that the entry accepts.
 *** @returns true if successful, false otherwise.
 ***/
static inline bool authFileWriteUser(char* path, char* username, char* password)
    {
    char cred[CRED_SIZE];
    char line[AUTH_FILE_SIZE];

	if (!authCred(cred, password)) return false;
	snprintf(line, sizeof(line), "%s:%s\n", username, cred);

    return tmpFileWrite(path, line);
    }

#endif
