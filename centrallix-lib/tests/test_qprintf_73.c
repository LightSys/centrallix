#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "qprintf.h"
#include <assert.h>
#include <locale.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    pQPSession session;

    setlocale(0, "en_US.UTF-8");
    qpfInitialize(); 

    *tname = "qprintf-73 test default flags setup with sessions";
    iter = 200000;

    for(i=0;i<iter;i++)
	{
	/*** default should match locale ***/
	session = qpfOpenSession();
	assert(session->Flags == QPF_F_ENFORCE_UTF8);
	qpfCloseSession(session);

	/*** this sets flags the same no matter what ***/
	session = qpfOpenSessionFlags(12345);
	assert(session->Flags == 12345);
	qpfCloseSession(session);

	session = qpfOpenSessionFlags(0);
	assert(session->Flags == 0);
	qpfCloseSession(session);

	}

    return iter*2;
    }
