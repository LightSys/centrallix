#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>

long long
test(char** tname)
    {
    int i, j;
    int iter;
    int flags;
    pLxSession lxs;
	char* resWords[11] = {"実に神は、", "ひとり子をさえ", "惜しまず与える", "ほどに、", "この世界を愛して", 
		"くださいました。", "それは、", "神の御子\xFFを信じる者が、", "だれ一人滅びず、", "永遠のいのちを得るためです。", NULL};
	char* valid_resWords[11] = {"実に神は、", "ひとり子をさえ", "惜しまず与える", "ほどに、", "この世界を愛して", 
		"くださいました。", "それは、", "神の御子を信じる者が、", "だれ一人滅びず、", "永遠のいのちを得るためです。", NULL};
    char str[65536] = "'하나님이' '세상을' '무척' '사랑하셔서' '하나밖에' '\xFF없는' '외아들마저' '보내'";  

	*tname = "mtlexer-23 test reserve word lists with invalid utf-8 characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 200000;
	for(i=0;i<iter;i++)
	    {
		/** full test fails on the 8th **/
		setlocale(0, "en_US.UTF-8");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);

		int result = mlxSetReservedWords(lxs, resWords);
		assert(result == -1);
	    mlxCloseSession(lxs);


		/** valid set should pass **/
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);

		result = mlxSetReservedWords(lxs, valid_resWords);
		assert(result == 0);
		for(j = 0 ; j < 10 ; j++)
			{
			assert(strcmp(lxs->ReservedWords[j], valid_resWords[j]) == 0);
			}
	    mlxCloseSession(lxs);


		/** test without utf-8 locale (and therefore without a validate); should pass all junk **/ 
		setlocale(0, "C");
		lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);

		result = mlxSetReservedWords(lxs, resWords);
		assert(result == 0);
		for(j = 0 ; j < 10 ; j++)
			{
			assert(strcmp(lxs->ReservedWords[j], resWords[j]) == 0);
			}
	    mlxCloseSession(lxs);
	    }

    return iter;
    }

