#include <assert.h>
#include "cxlib/mtsession.h"
#include "cxlib/newmalloc.h"
#include "prtmgmt_v3/prtmgmt_v3.h"

int
discard_output(void* arg, char* buf, int len, int offset, int flags)
    {
    return len;
    }

long long
test(char** name)
    {
    pPrtSession session;
    int page;
    double width, height;

    *name = "prt_margins_00 prtSetMargins() page margins";

    nmInitialize();
    mssInitialize("system", "", "", 0, "test", 0);
    prtInitialize();
    prt_strictfm_Initialize();
    prt_textod_Initialize();

    session = prtOpenSession("text/plain", discard_output, NULL, 0);
    assert(session != NULL);
    page = prtGetPageRef(session);
    assert(page >= 0);
    assert(prtGetPageGeometry(session, &width, &height) == 0);

    /** Margins that leave room on the page **/
    assert(prtSetMargins(page, 3, 3, 2.5, 2.5) == 0);

    /** Margins that meet or cross the page edges **/
    assert(prtSetMargins(page, -1, -1, width/2, width/2) < 0);
    assert(prtSetMargins(page, height/2, height/2, -1, -1) < 0);
    assert(prtSetMargins(page, -1, -1, width, 0) < 0);

    prtCloseSession(session);

    return 0;
    }
