#include <assert.h>
#include <string.h>

#include "centrallix.h"
#include "mime.h"

long long
test(char** tname)
    {
    XArray list;
    pEmailAddr addr;
    char quoted[] = "\"Jane Doe\" <jane@example.com>";
    char mixed[] = "\"Doe, Jane\" <jane@example.com>, Bob (the builder) <bob@example.com>";
    char group[] = "Team: \"Ann Lee\" <ann@example.com>, carl@example.com;";
    int i;

	/*** This test verifies that address lists keep addresses whose display
	 *** names are quoted or in a comment, including inside a group.
	 ***/

	*tname = "objdrv_mime_13 quoted display names in address lists";

	/** A quoted display name. **/
	xaInit(&list, 4);
	assert(libmime_ParseAddressList(quoted, &list) == 0); /* list parsed */
	assert(xaCount(&list) == 1); /* address kept */
	addr = (pEmailAddr)xaGetItem(&list, 0);
	assert(strcmp(addr->Display, "Jane Doe") == 0); /* display name */
	assert(strcmp(addr->Mailbox, "jane") == 0); /* mailbox */
	assert(strcmp(addr->Host, "example.com") == 0); /* host */
	for(i=0;i<xaCount(&list);i++)
	    libmime_FreeAddress((pEmailAddr)xaGetItem(&list, i));
	xaDeInit(&list);

	/** A comma inside quotes, then a comment. **/
	xaInit(&list, 4);
	assert(libmime_ParseAddressList(mixed, &list) == 0); /* list parsed */
	assert(xaCount(&list) == 2); /* both addresses kept */
	addr = (pEmailAddr)xaGetItem(&list, 0);
	assert(strcmp(addr->Display, "Doe, Jane") == 0); /* comma kept in quotes */
	assert(strcmp(addr->Mailbox, "jane") == 0); /* first mailbox */
	addr = (pEmailAddr)xaGetItem(&list, 1);
	assert(strcmp(addr->Display, "the builder") == 0); /* comment as display name */
	assert(strcmp(addr->Mailbox, "bob") == 0); /* second mailbox */
	for(i=0;i<xaCount(&list);i++)
	    libmime_FreeAddress((pEmailAddr)xaGetItem(&list, i));
	xaDeInit(&list);

	/** A group with a quoted member. **/
	xaInit(&list, 4);
	assert(libmime_ParseAddressList(group, &list) == 0); /* list parsed */
	assert(xaCount(&list) == 1); /* group kept */
	addr = (pEmailAddr)xaGetItem(&list, 0);
	assert(addr->Group != NULL); /* parsed as a group */
	assert(xaCount(addr->Group) == 2); /* both members kept */
	assert(strcmp(((pEmailAddr)xaGetItem(addr->Group, 0))->Display, "Ann Lee") == 0); /* quoted member */
	assert(strcmp(((pEmailAddr)xaGetItem(addr->Group, 1))->Mailbox, "carl") == 0); /* plain member */
	for(i=0;i<xaCount(&list);i++)
	    libmime_FreeAddress((pEmailAddr)xaGetItem(&list, i));
	xaDeInit(&list);

    return 0;
    }
