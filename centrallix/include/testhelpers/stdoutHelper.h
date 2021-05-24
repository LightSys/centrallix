#ifndef _STDOUT_HELPER_H
#define _STDOUT_HELPER_H

/** Begin redirecting and capturing stdout output. It will no longer output to the terminal (or wherever else you have it going) */
int stdoutHelper_startCapture();

/** Stop redirecting stdout and return the redirected contents that were captured */
char* stdoutHelper_stopCaptureAndGetContents();

#endif /* not defined _STDOUT_HELPER_H */