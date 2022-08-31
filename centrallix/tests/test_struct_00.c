#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "obj.h"

long long
test(char** name)
    {
    *name = "struct_00 test with valid file";
    setlocale(0, "en_US.UTF-8");

    /** create a valid .rpt file **/
    int fd;
    int i;
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
    /** Default global values **/
	strcpy(CxGlobals.ConfigFileName, "/usr/local/etc/centrallix.conf");
	CxGlobals.QuietInit = 1;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	CxGlobals.ArgV = NULL;
	CxGlobals.Flags = 0;
    
    cxInitialize(NULL);

    /** Search for STX driver **/
	pObjDriver stx = NULL; 
	for(i = 0 ; i < OSYS.Drivers.nItems ; i++)
        {
        if(strcmp(((pObjDriver)OSYS.Drivers.Items[i])->Name, "STX - Structure File Driver") == 0)
            {
            stx = (pObjDriver)OSYS.Drivers.Items[i];
            break;
            }
        }
    assert(stx);
    assert(stx->Open);

    pObject obj = nmSysMalloc(sizeof(Object));
    obj->Mode = O_RDWR | O_CREAT; 
    obj->Pathname = "";
    assert(obj > 0);
    //stx->Open();

    nmSysFree(obj);
    return 0;
    }