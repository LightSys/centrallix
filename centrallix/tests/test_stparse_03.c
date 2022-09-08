#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include "include/stparse.h"

void writeFile(char* rpt[], char * name);
pStructInf testFile(char * name);

long long
test(char** name)  
    {
    *name = "stparse_03 .rpt file from centrallix-os";
    setlocale(0, "en_US.UTF-8");

    pStructInf parsed = testFile("../centrallix-os/tests/UTF8_test_invalid0.rpt");  
    assert(parsed == NULL);

    parsed = testFile("../centrallix-os/tests/UTF8_test_invalid_1.app"); 
    assert(parsed == NULL);

    return;
    }

/*** parse file and check completed correctly ***/
pStructInf testFile(char * name)
    {
    pFile fd2 = fdOpen(name, O_RDONLY, 0600);
    assert(fd2);
    pStructInf parsed = stParseMsg(fd2, 0);
    close((int) fd2);  
    return parsed;
    } 