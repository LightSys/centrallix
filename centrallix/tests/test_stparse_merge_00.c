#include <assert.h>
#include <string.h>
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"

#include "stparse.h"

typedef struct
    {
    char*	Text;
    int		Offset;
    }
    TextSource;

int
read_text(void* arg, char* buf, int maxcnt, int offset, int flags)
    {
    TextSource* src = (TextSource*)arg;
    int n = strlen(src->Text + src->Offset);

	if (n > maxcnt) n = maxcnt;
	memcpy(buf, src->Text + src->Offset, n);
	src->Offset += n;

    return n;
    }

pStructInf
parse(char* text)
    {
    TextSource src = { text, 0 };
    pStructInf inf;

	inf = stParseMsgGeneric(&src, read_text, 0);
	assert(inf != NULL);

    return inf;
    }

int
int_value(pStructInf group, char* name)
    {
    int n = -1;
    int rval;

	rval = stAttrValue(stLookup(group, name), &n, NULL, 0);
	assert(rval == 0);

    return n;
    }

char*
str_value(pStructInf group, char* name, int nval)
    {
    char* str = NULL;

	if (stAttrValue(stLookup(group, name), NULL, &str, nval) != 0) return NULL;

    return str;
    }

/*** stMergeInf() merges an included config over a base config: attributes
 *** replace values in place, groups merge recursively, and unmatched
 *** entries are appended.  Mismatched kinds or group types fail.
 ***/
long long
test(char** name)
    {
    pStructInf base, http;
    int rval;

    *name = "stparse_merge_00 stMergeInf() merges config trees";

    nmInitialize();
    mssInitialize("system", "", "", 0, "test", 0);

    /** Overrides, nested merges, and additions **/
    base = parse(
	"$Version=2$\n"
	"centrallix \"system/config\"\n"
	"    {\n"
	"    debug_log_file = \"/var/log/cx_debug_log\";\n"
	"    enable_send_credentials = 0;\n"
	"    net_http \"system/config\"\n"
	"\t{\n"
	"\tlisten_port = 800;\n"
	"\taccept_localhost_only = 1;\n"
	"\tdir_index = \"index.app\",\"index.html\";\n"
	"\t}\n"
	"    net_http \"system/config\" { listen_port = 1; }\n"
	"    }\n");
    rval = stMergeInf(base, parse(
	"$Version=2$\n"
	"kardia \"system/config\"\n"
	"    {\n"
	"    enable_send_credentials = 1;\n"
	"    net_http \"system/config\"\n"
	"\t{\n"
	"\taccept_localhost_only = 0;\n"
	"\tdir_index = \"index.app\";\n"
	"\tupload_dirs = \"/apps/kardia/files\";\n"
	"\t}\n"
	"    mtsession \"system/config\" { auth_method = \"altpasswd\"; }\n"
	"    }\n"));
    assert(rval == 0);
    assert(strcmp(base->Name, "centrallix") == 0);				// base keeps its name
    assert(base->nSubInf == 5);							// one group added
    assert(strcmp(base->SubInf[1]->Name, "enable_send_credentials") == 0);	// attribute keeps its place
    assert(int_value(base, "enable_send_credentials") == 1);			// attribute replaced
    assert(strcmp(str_value(base, "debug_log_file", 0), "/var/log/cx_debug_log") == 0); // untouched attribute kept
    assert(strcmp(str_value(stLookup(base, "mtsession"), "auth_method", 0), "altpasswd") == 0); // new group added
    assert(strcmp(base->SubInf[4]->Name, "mtsession") == 0);			// new group at the end
    http = stLookup(base, "net_http");
    assert(http == base->SubInf[2]);						// first duplicate merged
    assert(int_value(http, "listen_port") == 800);				// nested attribute kept
    assert(int_value(http, "accept_localhost_only") == 0);			// nested attribute replaced
    assert(strcmp(str_value(http, "dir_index", 0), "index.app") == 0);	// list replaced...
    assert(str_value(http, "dir_index", 1) == NULL);				// ...not appended to
    assert(strcmp(str_value(http, "upload_dirs", 0), "/apps/kardia/files") == 0); // nested attribute added
    assert(int_value(base->SubInf[3], "listen_port") == 1);			// second duplicate untouched
    stFreeInf(base);

    /** Group types must match **/
    base = parse("$Version=2$\ncentrallix \"system/config\" { net_http \"system/config\" { listen_port = 800; } }\n");
    rval = stMergeInf(base, parse("$Version=2$\ncentrallix \"system/config\" { net_http \"system/other\" { listen_port = 8080; } }\n"));
    assert(rval < 0);
    stFreeInf(base);
    base = parse("$Version=2$\ncentrallix \"system/config\" { listen_port = 800; }\n");
    rval = stMergeInf(base, parse("$Version=2$\ncentrallix \"system/other\" { listen_port = 8080; }\n"));
    assert(rval < 0);
    stFreeInf(base);

    /** An attribute and a group cannot merge **/
    base = parse("$Version=2$\ncentrallix \"system/config\" { net_http \"system/config\" { listen_port = 800; } }\n");
    rval = stMergeInf(base, parse("$Version=2$\ncentrallix \"system/config\" { net_http = 8080; }\n"));
    assert(rval < 0);
    stFreeInf(base);
    base = parse("$Version=2$\ncentrallix \"system/config\" { net_http = 8080; }\n");
    rval = stMergeInf(base, parse("$Version=2$\ncentrallix \"system/config\" { net_http \"system/config\" { listen_port = 800; } }\n"));
    assert(rval < 0);
    stFreeInf(base);

    return 0;
    }
