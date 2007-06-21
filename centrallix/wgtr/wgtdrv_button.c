#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "wgtr.h"
#include "iface.h"

/*** wgtbtnVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtbtnVerify(pWgtrVerifySession s)
    {
    return 0;
    }


/*** wgtbtnNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtbtnNew(pWgtrNode node)
    {
	if(node->fl_width < 0) node->fl_width = 0;
	if(node->fl_height < 0) node->fl_height = 0;
	
	return wgtrImplementsInterface(node, "net/centrallix/button.ifc?cx__version=1.1");
	
    }


int
wgtbtnInitialize()
    {
    char* name = "Button Widget Driver";
    
	wgtrRegisterDriver(name, wgtbtnVerify, wgtbtnNew);
	wgtrAddType(name, "button");

	return 0;
    }
