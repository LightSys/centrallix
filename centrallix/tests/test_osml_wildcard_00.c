#include <assert.h>
#include "centrallix.h"
#include "application.h"
#include "obj.h"
#include "cxlib/xarray.h"
#include "cxlib/xstring.h"

char* queries[] = { "/tests/test1.json", "/tests/test*.json", "/test*/test*.json", "/tests/*", "/*/test1.json", "/test*", "/tests", NULL };

char* compare = 
"Expanding: /tests/test1.json\n"
"  0) /tests/test1.json\n"
"Expanding: /tests/test*.json\n"
"  0) /tests/test1.json\n"
"  1) /tests/test2.json\n"
"  2) /tests/test3.json\n"
"Expanding: /test*/test*.json\n"
"  0) /tests/test1.json\n"
"  1) /tests/test2.json\n"
"  2) /tests/test3.json\n"
"Expanding: /tests/*\n"
"  0) /tests/Datatypes.csv\n"
"  1) /tests/Datatypes.rpt\n"
"  2) /tests/Datatypes.spec\n"
"  3) /tests/Datatypes_csv.rpt\n"
"  4) /tests/NestedTables.rpt\n"
"  5) /tests/Nested_test.rpt\n"
"  6) /tests/README\n"
"  7) /tests/States.csv\n"
"  8) /tests/States.spec\n"
"  9) /tests/T01-Combined1.app\n"
"  10) /tests/T02-Combined2.app\n"
"  11) /tests/T03-Combined3.app\n"
"  12) /tests/TableBreakCell.rpt\n"
"  13) /tests/TestLevel0.csv\n"
"  14) /tests/TestLevel0.spec\n"
"  15) /tests/TestLevel1.csv\n"
"  16) /tests/TestLevel1.spec\n"
"  17) /tests/TestLevel2.csv\n"
"  18) /tests/TestLevel2.spec\n"
"  19) /tests/Testdata1.csv\n"
"  20) /tests/Testdata1.spec\n"
"  21) /tests/Testdata2.csv\n"
"  22) /tests/Testdata2.spec\n"
"  23) /tests/font_metric_test.rpt\n"
"  24) /tests/index.app\n"
"  25) /tests/multilevel1.json\n"
"  26) /tests/test1.json\n"
"  27) /tests/test2.json\n"
"  28) /tests/test3.json\n"
"  29) /tests/testquery.qy\n"
"Expanding: /*/test1.json\n"
"  0) /tests/test1.json\n"
"Expanding: /test*\n"
"  0) /tests\n"
"Expanding: /tests\n"
"  0) /tests\n"
;

long long
test(char** name)
    {
    pObjSession s;
    pXArray xa;
    pXString out;
    int i, j;
    
    *name = "osml_wildcard_00 Wildcard Match Query";
   
    strcpy(CxGlobals.ConfigFileName, "/home/devel/cxinst/etc/centrallix.conf");
    CxGlobals.QuietInit = 1;
    CxGlobals.ParsedConfig = NULL;
    CxGlobals.ModuleList = NULL;
    CxGlobals.ArgV = NULL;
    CxGlobals.Flags = 0;
    cxInitialize();

    s = objOpenSession("/");
    xa = xaNew(16);
    out = xsNew();

    for(i=0; queries[i]; i++)
	{
	xsConcatPrintf(out, "Expanding: %s\n", queries[i]);
	if (objWildcardQuery(xa, s, queries[i]) < 0)
	    {
	    xsConcatPrintf(out, "  FAILED\n");
	    }
	else
	    {
	    for(j=0; j<xa->nItems; j++)
		{
		xsConcatPrintf(out, "  %d) %s\n", j, (char*)xa->Items[j]);
		}
	    xaClear(xa, nmSysFree, 0);
	    }
	}

    objCloseSession(s);
    xaFree(xa);

    if (!strcmp(out->String, compare))
  	{
    	xsFree(out);
        return 0;
	}
    else
	{
	printf("Output:\n%s\nShould Be:\n%s\n", out->String, compare);
    	xsFree(out);
	return -1;
	}
}
