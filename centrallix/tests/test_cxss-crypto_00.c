#include <assert.h>
#include <stdio.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Crypto 00: Basic Init";

    /** Basic test of init and release */
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();
    cxssCryptoCleanup();

    return 0;
    }
