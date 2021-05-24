#include <stdio.h>
#include <unistd.h>
#include "testhelpers/stdoutHelper.h"

int stdoutHelper_stdoutCopy;
FILE *stdoutHelper_tmpfile;

int
stdoutHelper_startCapture()
{
  // Redirect stdout to a file
  fflush(stdout);
  stdoutHelper_stdoutCopy = dup(STDOUT_FILENO);
  stdoutHelper_tmpfile = fopen(".test-helper-stdout", "w+");
  dup2(fileno(stdoutHelper_tmpfile), STDOUT_FILENO);
}

char*
stdoutHelper_stopCaptureAndGetContents()
{
  // Stop redirecting stdout
  fflush(stdout);
  dup2(stdoutHelper_stdoutCopy, STDOUT_FILENO);
  close(stdoutHelper_stdoutCopy);

  // Read the contents of the file with stdout contents
  char* contents = "";
  fseek(stdoutHelper_tmpfile, 0, SEEK_END);
  long size = ftell(stdoutHelper_tmpfile);
  if (size > 0) {
    contents = malloc(size + 1);
    rewind(stdoutHelper_tmpfile);
    fread(contents, 1, size, stdoutHelper_tmpfile);
    contents[size] = 0;
  }

  // Close and remove the file and return its contents
  fclose(stdoutHelper_tmpfile);
  remove(".test-helper-stdout");
  return contents;
}