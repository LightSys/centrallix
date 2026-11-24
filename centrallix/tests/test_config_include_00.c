#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "stparse.h"

#include "centrallix.h"

char TestDir[] = "/tmp/cx_config_include_XXXXXX";

char*
test_path(char* name)
    {
    static char path[256];

	snprintf(path, sizeof(path), "%s/%s", TestDir, name);

    return path;
    }

void
write_file(char* name, char* text)
    {
    FILE* f;

	f = fopen(test_path(name), "w");
	assert(f != NULL);
	fputs(text, f);
	fclose(f);
    }

void
write_main_config(char* include_dir)
    {
    char text[512];

	snprintf(text, sizeof(text),
	    "$Version=2$\n"
	    "centrallix \"system/config\"\n"
	    "    {\n"
	    "    include_dir = \"%s/%s\";\n"
	    "    net_http \"system/config\"\n"
	    "\t{\n"
	    "\tlisten_port = 800;\n"
	    "\taccept_localhost_only = 1;\n"
	    "\tauth_realm = \"Centrallix\";\n"
	    "\t}\n"
	    "    }\n",
	    TestDir, include_dir);
	write_file("centrallix.conf", text);
    }

int
http_int(char* name)
    {
    int n = -1;

	assert(stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"), name), &n, NULL, 0) == 0);

    return n;
    }

char*
http_str(char* name)
    {
    char* str = NULL;

	assert(stAttrValue(stLookup(stLookup(CxGlobals.ParsedConfig, "net_http"), name), NULL, &str, 0) == 0);

    return str;
    }

void
free_config()
    {
	if (CxGlobals.ParsedConfig != NULL) stFreeInf(CxGlobals.ParsedConfig);
	CxGlobals.ParsedConfig = NULL;
    }

/*** cxLoadConfig() merges the visible "*.conf" files in include_dir over
 *** the main config file in name order, and fails if one of them sets
 *** include_dir or the directory is missing.
 ***/
long long
test(char** name)
    {
    *name = "config_include_00 cxLoadConfig() merges include_dir";

    nmInitialize();
    mssInitialize("system", "", "", 0, "test", 0);
    assert(mkdtemp(TestDir) != NULL);
    strcpy(CxGlobals.ConfigFileName, test_path("centrallix.conf"));
    CxGlobals.QuietInit = 1;

    /** Drop-ins override in name order; other files are ignored **/
    assert(mkdir(test_path("conf.d"), 0700) == 0);
    write_main_config("conf.d");
    write_file("conf.d/90-local.conf", "$Version=2$\nlocal \"system/config\" { net_http \"system/config\" { listen_port = 8443; } }\n");
    write_file("conf.d/50-kardia.conf",
	"$Version=2$\nkardia \"system/config\" { net_http \"system/config\" { listen_port = 800; accept_localhost_only = 0; auth_realm = \"Kardia\"; } }\n");
    write_file("conf.d/.99-hidden.conf", "$Version=2$\nhidden \"system/config\" { net_http \"system/config\" { auth_realm = \"Hidden\"; } }\n");
    write_file("conf.d/99-backup.conf~", "$Version=2$\nbackup \"system/config\" { net_http \"system/config\" { auth_realm = \"Backup\"; } }\n");
    write_file("conf.d/README", "Not a config file.\n");
    assert(cxLoadConfig() == 0);
    assert(http_int("listen_port") == 8443);				// later file wins
    assert(http_int("accept_localhost_only") == 0);			// earlier file applies
    assert(strcmp(http_str("auth_realm"), "Kardia") == 0);		// hidden and non-.conf files ignored
    free_config();

    /** Only the main config file can set include_dir **/
    write_file("conf.d/95-redirect.conf", "$Version=2$\nredirect \"system/config\" { include_dir = \"/etc\"; }\n");
    assert(cxLoadConfig() < 0);
    free_config();

    /** A missing include_dir fails **/
    write_main_config("missing.d");
    assert(cxLoadConfig() < 0);
    free_config();

    /** Clean up **/
    unlink(test_path("conf.d/95-redirect.conf"));
    unlink(test_path("conf.d/90-local.conf"));
    unlink(test_path("conf.d/50-kardia.conf"));
    unlink(test_path("conf.d/.99-hidden.conf"));
    unlink(test_path("conf.d/99-backup.conf~"));
    unlink(test_path("conf.d/README"));
    rmdir(test_path("conf.d"));
    unlink(test_path("centrallix.conf"));
    rmdir(TestDir);

    return 0;
    }
