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
	int BUF_SIZE = 2000;
    int i;
    int iter;
    int flags;
    pLxSession lxs;
    int t;
    char* strval;
    char str[65536] = "'นปฐมกาล พระเจ้าทรงสร้างทุกสิ่งในฟ้าสวรรค์และโลก ขณะนั้นโลกยังไม่มีรูปทรงและว่างเปล่า " /* one really long line (398 UTF-8 chars). Each one is 3 bytes (but not the spaces)*/
					  "ความมืดปกคลุมอยู่เหนือพื้นผิวของห้วงน้ำ \xFFพระวิญญาณของพระเจ้าทรงเคลื่อนไหวอยู่เหนือน้ำนั้น " /* contains 1165 btes */
					  "และพระเจ้าตรัสว่า “จงเกิดความสว่าง” ความสว่างก็เกิดขึ้น พระเจ้าทรงเห็นว่าความสว่างนั้นดี "
					  "และทรงแยกความสว่างออกจากความมืด พระเจ้าทรงเรียกความสว่างว่า “วัน” และเรียกความมืดว่า "
					  "“คืน” เวลาเย็นและเวลาเช้าผ่านไป นี่เป็นวันที่หนึ่ง'"; 
	char tokstr[MLX_STRVAL];
	memcpy(tokstr, str+1, MLX_STRVAL-1);
	tokstr[MLX_STRVAL-1] = '\0';
	char buf[BUF_SIZE];  

	*tname = "mtlexer-24 test Copy Token with invalid utf-8 characters";

	mssInitialize("system", "", "", 0, "test");
	iter = 50000;
	for(i=0;i<iter;i++)
	    {
		/** setup **/
		setlocale(0, "en_US.UTF-8");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
		/* read in the first token. This should read the first 256 bytes */
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		strval = lxs->TokString;
		assert(strval != NULL);
		assert(strcmp(strval,tokstr) == 0);
		/* now peform a copy into a buffer twice the size. Should enable invalid UTF8 to bypass the 
		 * checks in next token, but should be caught by the check at the end of copy
		 */
		int result = mlxCopyToken(lxs, buf, BUF_SIZE);
		assert(result == strlen(str)-2); /* -2 since ' is not included */
		str[strlen(str)-1] = '\0';  /* change for test */
		assert(strcmp(buf, str+1) == 0);
		str[strlen(str)] = '\'';  /* revert back. Note string is 1 shorter now */
		assert(lxs->TokType == MLX_TOK_ERROR);
		mlxCloseSession(lxs);


		/** trying again without utf8 results in a normal token **/
		setlocale(0, "C");
	    flags = 0;
	    lxs = mlxStringSession(str, flags);
	    assert(lxs != NULL);
		/* read in the first token. This should read the first 256 bytes */
		t = mlxNextToken(lxs);
		assert(t == MLX_TOK_STRING);
		strval = lxs->TokString;
		assert(strval != NULL);
		assert(strcmp(strval,tokstr) == 0);
		/* now test again */
		result = mlxCopyToken(lxs, buf, BUF_SIZE);
		str[strlen(str)-1] = '\0';  /* change for test */
		assert(strcmp(buf, str+1) == 0);
		str[strlen(str)] = '\'';  /* revert back */
		assert(lxs->TokType == MLX_TOK_STRING);
	    }

    return iter;
    }

