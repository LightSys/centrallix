#ifndef _MSS_ERROR_HELPER_H
#define _MSS_ERROR_HELPER_H

/** Authenticates with Centrallix so errors are captured */
int mssErrorHelper_init();

/** 
 * Returns 1 if the most recent call to mssError contains the string in "message", 0 otherwise. 
 * You must call mssErrorHelper_init() first so calls to mssError are captured.
 */
int mssErrorHelper_mostRecentErrorContains(char *message);

#endif /* not defined _MSS_ERROR_HELPER_H */