#include <stdio.h>
#include <string.h>
#include "testhelpers/mssErrorHelper.h"
#include "cxlib/xstring.h"

int
mssErrorHelper_init()
{
  mssInitialize("altpasswd", "/usr/local/etc/centrallix/cxpasswd-test", "", 0, "test");
  return mssAuthenticate("cxtest", "cxtestpass");
}

int
mssErrorHelper_mostRecentErrorContains(char* message)
{
  pXString err = xsNew();
  mssStringError(err);
  char* foundPtr = strstr(xsString(err), message);
  xsFree(err);
  return !(!foundPtr);
}