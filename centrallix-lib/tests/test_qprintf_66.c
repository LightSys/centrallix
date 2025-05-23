#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>

long long
test(char** tname)
{
    int i, rval;
    int iter;
    unsigned char buf[39];

    *tname = "qprintf-66 %LL insertion in middle without overflow";
    iter = 200000;
    for(i=0;i<iter;i++)
    {
        buf[34] = 0xff;
        buf[35] = '\n';
        buf[36] = '\0';
        buf[37] = 0xff;
        buf[38] = '\0';
        buf[3] = '\n';
        buf[2] = '\0';
        buf[1] = 0xff;
        buf[0] = '\0';

        //Test value > INT_MAX
        long long testNum = 2200000000ll;

        qpfPrintf(NULL, buf + 4, 36, "Here is the ll: %LL...", testNum);
        qpfPrintf(NULL, buf+4, 36, "Here is the ll: %LL...", testNum);
        qpfPrintf(NULL, buf+4, 36, "Here is the ll: %LL...", testNum);
        rval = qpfPrintf(NULL, buf+4, 36, "Here is the ll: %LL...", testNum);

        assert(!strcmp(buf+4, "Here is the ll: 2200000000..."));
        //For the long long case, rval will be set to the return value of snprintf, i.e. length of string
        assert(rval == 29);
        assert(buf[34] == 0xff);
        assert(buf[35] == '\n');
        assert(buf[36] == '\0');
        assert(buf[37] == 0xff);
        assert(buf[38] == '\0');
        assert(buf[3] == '\n');
        assert(buf[2] == '\0');
        assert(buf[1] == 0xff);
        assert(buf[0] == '\0');
    }

    return iter*4;
}