#include <assert.h>
#include <stdio.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Crypto 00: Basic Init";

    /** Basic test of init and release */
    cxss_internal_InitEntropy(1280);
    cxssCryptoInit();
    cxssCryptoCleanup();

    return 0;
    }
