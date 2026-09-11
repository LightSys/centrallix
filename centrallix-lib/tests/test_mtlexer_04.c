#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "newmalloc.h"
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <stdbool.h>
#include "test_utils.h"

static char str[65536] = "";

static bool
doTest(void)
    {
    pLxSession lxs;

	lxs = mlxStringSession(str, MLX_F_EOL | MLX_F_EOF | MLX_F_IFSONLY);
	assert(lxs != NULL);
	mlxCloseSession(lxs);

    return true;
    }

long long
test(char** tname)
    {
    *tname = "mtlexer-04 open/close session";
    mssInitialize("system", "", "", 0, "test");
    return loopTest(doTest);
    }
