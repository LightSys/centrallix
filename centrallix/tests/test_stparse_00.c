#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <locale.h>
#include "include/stparse.h"

long long
test(char** name)
    {
    *name = "stparse_00 reading valid .rpt file";
    setlocale(0, "en_US.UTF-8");

    /** create a valid .rpt file **/
    int fd;
    int result;
    char *rpt = "$Version=2$\n"
                "test \"system/report\"\n"
                "   {\n"
                "   title = \"This is my title\";\n"
                "   locationtypes \"report/query\"\n"
                "       {\n"
                "       sql = \"select :location_code, :description from "
                               "/datasources/OMSS_DB/_LocationCode/rows\";\n"
                "       }\n"
                "   mycomment \"report/comment\"\n"
                "       {\n"
                "       text = \"This is my comment's text, in BOLD and UNDERLINED.\\n\";\n"
                "       autonewline=no;\n"
                "       style=bold,underline;\n"
                "       }\n"
                "   codelist \"report/form\"\n"
                "       {\n"
                "       source = locationtypes;\n"
                "       rulesep = yes;\n"
                "       t1 \"report/comment\" { text = \"Code: \"; autonewline=no; }\n"
                "       c1 \"report/column\" { source = location_code; }\n"
                "       t2 \"report/comment\" { text = \"Desc.: \"; autonewline=no; }\n"
                "       c2 \"report/column\" { source = description; }\n"
                "       }\n"
                "   codelist2 \"report/table\"\n"
                "       {\n"
                "       source = locationtypes;\n"
                "       columns = location_code,description;\n"
                "       widths = 10,40;\n"
                "       style = italic;\n"
                "       titlebar = yes;\n"
                "       }\n"
                "   mycol \"report/column\"\n"
                "       {\n"
                "       source = location_code;\n"
                "       style = bold;\n"
                "       }\n"
                "   section1 \"report/section\"\n"
                "       {\n"
                "       title=\"My First Section\";\n"
                "       columns=2;\n"
                "       colsep=4;\n"
                "       margins=4,4;\n"
                "       style=plain;\n"
                "       }\n"
                "   script \"system/script\"\n"
                "       {{\n"
                "       code=\"var test = 12;\"\n"
                "       }}\n"
                "    }\n";
    fd = open("./tests/tempFiles/test01.rpt", O_CREAT | O_WRONLY | O_TRUNC);
    result = write(fd, rpt, strlen(rpt));
    assert(result = 20);
    close(fd); 

    /** parse file and check completed correctly **/
    pFile fd2 = fdOpen("./tests/tempFiles/test01.rpt", O_RDONLY, 0600);
    assert(fd2);
    pStructInf parsed = stParseMsg(fd2, 0);
    assert(parsed != NULL);
    close(fd2); 

    assert(strcmp(parsed->Name, "test") == 0);
    assert(strcmp(parsed->UsrType, "system/report") == 0);
    
    pStructInf cur = parsed->SubInf[0];
    assert(strcmp(cur->Name, "title") == 0);
    assert(strcmp(cur->Value->String, "This is my title") == 0);
    
    cur = parsed->SubInf[1];
    assert(strcmp(cur->Name, "locationtypes") == 0);
    assert(strcmp(cur->UsrType, "report/query") == 0);
        pStructInf curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "sql") == 0);
        assert(strcmp(curC->Value->String, "select :location_code, :description from "
                                    "/datasources/OMSS_DB/_LocationCode/rows") == 0);

    cur = parsed->SubInf[2];
    assert(strcmp(cur->Name, "mycomment") == 0);
    assert(strcmp(cur->UsrType, "report/comment") == 0);
        curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "text") == 0);
        assert(strcmp(curC->Value->String, "This is my comment's text, in BOLD and UNDERLINED.\n") == 0);
        curC = cur->SubInf[1];
        assert(strcmp(curC->Name, "autonewline") == 0);
        assert(strcmp(curC->Value->String, "no") == 0);
        curC = cur->SubInf[2];
        assert(strcmp(curC->Name, "style") == 0);
        assert(strcmp(((pExpression)curC->Value->Children.Items[0])->String, "bold") == 0);
        assert(strcmp(((pExpression)curC->Value->Children.Items[1])->String, "underline") == 0);
    
    cur=parsed->SubInf[3];
    assert(strcmp(cur->Name, "codelist") == 0);
    assert(strcmp(cur->UsrType, "report/form") == 0);
        curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "source") == 0);
        assert(strcmp(curC->Value->String, "locationtypes") == 0);
        curC = cur->SubInf[1];
        assert(strcmp(curC->Name, "rulesep") == 0);
        assert(strcmp(curC->Value->String, "yes") == 0);
        curC = cur->SubInf[2];
        assert(strcmp(curC->Name, "t1") == 0);
        assert(strcmp(curC->UsrType, "report/comment") == 0);
            pStructInf curGC = curC->SubInf[0];
            assert(strcmp(curGC->Name, "text") == 0);
            assert(strcmp(curGC->Value->String, "Code: ") == 0);
            curGC = curC->SubInf[1];
            assert(strcmp(curGC->Name, "autonewline") == 0);
            assert(strcmp(curGC->Value->String, "no") == 0);
        curC = cur->SubInf[3];
        assert(strcmp(curC->Name, "c1") == 0);
        assert(strcmp(curC->UsrType, "report/column") == 0);
            curGC = curC->SubInf[0];
            assert(strcmp(curGC->Name, "source") == 0);
            assert(strcmp(curGC->Value->String, "location_code") == 0);
        curC = cur->SubInf[4];
        assert(strcmp(curC->Name, "t2") == 0);
        assert(strcmp(curC->UsrType, "report/comment") == 0);
            curGC = curC->SubInf[0];
            assert(strcmp(curGC->Name, "text") == 0);
            assert(strcmp(curGC->Value->String, "Desc.: ") == 0);
            curGC = curC->SubInf[1];
            assert(strcmp(curGC->Name, "autonewline") == 0);
            assert(strcmp(curGC->Value->String, "no") == 0);
        curC = cur->SubInf[5];
        assert(strcmp(curC->Name, "c2") == 0);
        assert(strcmp(curC->UsrType, "report/column") == 0);
            curGC = curC->SubInf[0];
            assert(strcmp(curGC->Name, "source") == 0);
            assert(strcmp(curGC->Value->String, "description") == 0);
    
    cur=parsed->SubInf[4];
    assert(strcmp(cur->Name, "codelist2") == 0);
    assert(strcmp(cur->UsrType, "report/table") == 0);
        curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "source") == 0);
        assert(strcmp(curC->Value->String, "locationtypes") == 0);
        curC = cur->SubInf[1];
        assert(strcmp(curC->Name, "columns") == 0);
        assert(strcmp(((pExpression)curC->Value->Children.Items[0])->String, "location_code") == 0);
        assert(strcmp(((pExpression)curC->Value->Children.Items[1])->String, "description") == 0);
        curC = cur->SubInf[2];
        assert(strcmp(curC->Name, "widths") == 0);
        assert(((pExpression)curC->Value->Children.Items[0])->Integer == 10);
        assert(((pExpression)curC->Value->Children.Items[1])->Integer == 40);
        curC = cur->SubInf[3];
        assert(strcmp(curC->Name, "style") == 0);
        assert(strcmp(curC->Value->String, "italic") == 0);
        curC = cur->SubInf[4];
        assert(strcmp(curC->Name, "titlebar") == 0);
        assert(strcmp(curC->Value->String, "yes") == 0);
    
    cur=parsed->SubInf[5];
    assert(strcmp(cur->Name, "mycol") == 0);
    assert(strcmp(cur->UsrType, "report/column") == 0);
        curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "source") == 0);
        assert(strcmp(curC->Value->String, "location_code") == 0);
        curC = cur->SubInf[1];
        assert(strcmp(curC->Name, "style") == 0);
        assert(strcmp(curC->Value->String, "bold") == 0);

    cur=parsed->SubInf[6];
    assert(strcmp(cur->Name, "section1") == 0);
    assert(strcmp(cur->UsrType, "report/section") == 0);    
        curC = cur->SubInf[0];
        assert(strcmp(curC->Name, "title") == 0);
        assert(strcmp(curC->Value->String, "My First Section") == 0);
        curC = cur->SubInf[1];
        assert(strcmp(curC->Name, "columns") == 0);
        assert(curC->Value->Integer == 2);
        curC = cur->SubInf[2];
        assert(strcmp(curC->Name, "colsep") == 0);
        assert(curC->Value->Integer == 4);
        curC = cur->SubInf[3];
        assert(strcmp(curC->Name, "margins") == 0);
        assert(((pExpression)curC->Value->Children.Items[0])->Integer == 4);
        assert(((pExpression)curC->Value->Children.Items[1])->Integer == 4);
        curC = cur->SubInf[4];
        assert(strcmp(curC->Name, "style") == 0);
        assert(strcmp(curC->Value->String, "plain") == 0);
    
    cur=parsed->SubInf[7];
    assert(strcmp(cur->Name, "script") == 0);
    assert(strcmp(cur->UsrType, "system/script") == 0); 
    /** FIXME: doesn't seem like should have '\n' and "}}" inside it **/
    assert(strcmp(cur->ScriptText, "\n       code=\"var test = 12;\"\n       }}\n") == 0); 

    return 0;
    }