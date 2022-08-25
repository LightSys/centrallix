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
    *name = "stparse_00 .rpt file with invalid bytes";
    setlocale(0, "en_US.UTF-8");

    /** create a valid .rpt file **/
    char *rpt[] = {
    /* 0 */    "$Version=2$\n",                                                               
                "test \"system/report\"\n",
                "   {\n\0",
                "   title = \"This is my title\";\n\0",
                "   locationtypes \"report/query\"\n\0",
    /* 5 */     "       {\n\0",                                                                 
                "       sql = \"select :location_code, :description from /datasources/OMSS_DB/_LocationCode/rows\";\n\0",
                "       }\n\0",
                "   mycomment \"report/comment\"\n\0",
                "       {\n\0",                                                                 
    /* 10 */    "       text = \"This is my comment's text, in BOLD and UNDERLINED.\\n\";\n\0",
                "       autonewline=no;\n\0",
                "       style=bold,underline;\n\0",
                "       }\n\0",
                "   codelist \"report/form\"\n\0",
    /* 15 */    "       {\n\0",
                "       source = locationtypes;\n\0",
                "       rulesep = yes;\n\0",
                "       t1 \"report/comment\" { text = \"Code: \"; autonewline=no; }\n\0",
                "       c1 \"report/column\" { source = location_code; }\n\0",
    /* 20 */    "       t2 \"report/comment\" { text = \"Desc.: \"; autonewline=no; }\n\0",
                "       c2 \"report/column\" { source = description; }\n\0",
                "       }\n\0",
                "   codelist2 \"report/table\"\n\0",
                "       {\n\0",
    /* 25 */    "       source = locationtypes;\n\0",
                "       columns = location_code,description;\n\0",
                "       widths = 10,40;\n\0",
                "       style = italic;\n\0",
                "       titlebar = yes;\n\0",
    /* 30 */    "       }\n\0",
                "   mycol \"report/column\"\n\0",
                "       {\n\0",
                "       source = location_code;\n\0",
                "       style = bold;\n\0",
    /* 35 */    "       }\n\0",
                "   section1 \"report/section\"\n\0",
                "       {\n\0",
                "       title=\"My First Section\";\n\0",
                "       columns=2;\n\0",
    /* 40 */    "       colsep=4;\n\0",
                "       margins=4,4;\n\0",
                "       style=plain;\n\0",
                "       }\n\0",
                "   script \"system/script\"\n\0",
    /* 45 */    "       {{\n\0",
                "       code=\"var test = 12;\"\n\0",
                "       }}\n\0",
                "    }\n\0"
    };
    
    /** NOTE: The \xFF character is here used as a stand in for an invalid character. The robutsness of the validity 
     ** testing is confirmed performed elsewhere, so anything that will trigger the function is sufficient
     **/

    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    assert(testFile( "./tests/tempFiles/test01.rpt") != NULL);

    /* change name of first field to have an invalid char */
    rpt[1] = "te\xFFst \"system/report\"\n";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    pStructInf parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);

    /* make ths string invalid */
    rpt[1] = "test \"sys\xFFtem/report\"\n";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[1] = "test \"system/report\"\n"; /* fix so all other tests get past this line */

    /* test invlaid strings on a internal field */
    rpt[3] = "   title = \"This is my ti\xFFtle\";\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);

    rpt[3] = "   ti\xFFtle = \"This is my title\";\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[3] = "   title = \"This is my title\";\n\0";

    /* test child */
    rpt[6] = "       sql = \"sel\xFFect :location_code, :description from /datasources/OMSS_DB/_LocationCode/rows\";\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);

    rpt[6] = "       sql = \"select :location_code, :description from \xFF /datasources/OMSS_DB/_LocationCode/rows\";\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[6] = "       sql = \"select :location_code, :description from /datasources/OMSS_DB/_LocationCode/rows\";\n\0";

    /* test a deeper layer */
    rpt[20] = "       t2 \"report/comment\xFF\" { text = \"Desc.: \"; autonewline=no; }\n\0" ;
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[20] = "       t2 \"report/comment\" { text = \"Des\xFFc.: \"; autonewline=no; }\n\0" ;
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[20] = "       t2 \"report/comment\" { text = \"Desc.: \"; autonewline=no; }\n\0";

    /* test integer */
    rpt[27] = "       widths = 1\xFF0,40;\n\0" ;
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[27] = "       widths = 10,\xFF40;\n\0" ;
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[27] = "       widths = 10,40;\n\0";
                     
    /* test code */
    rpt[46] = "       code=\"var \xFF test = 12;\"\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile( "./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    rpt[46] = "       code=\"var test = 12;\"\n\0";

    rpt[47] = "    \xFF   }}\n\0";
    writeFile(rpt, "./tests/tempFiles/test01.rpt");
    parsed =  testFile("./tests/tempFiles/test01.rpt");
    assert(parsed == NULL);
    return 0;
    }

/** open write to file **/
void writeFile(char* rpt[], char * name)
    {
    int fd = open(name, O_CREAT | O_WRONLY | O_TRUNC);
    int result = write(fd, rpt[0], strlen(rpt[0]));
    assert(result = 20);
    close(fd); 

    int i = 0;
    fd = open(name, O_WRONLY | O_APPEND);
    do
        {
        i++;
        result = write(fd, rpt[i], strlen(rpt[i]));
        assert(result = 20);
        }   
    while (strcmp(rpt[i], "    }\n") != 0);

    close(fd); 

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