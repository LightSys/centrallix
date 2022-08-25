#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>
#include "qprintf.h"

void runTest(char** inputs, int ind, char* in, char* buf,  pLxSession* plxs);
void runNoUTF8Test(char** inputs, int ind, char* in, char* buf,  pLxSession* plxs);

long long
test(char** tname)
    {
    int i;
    int iter;
   
    pLxSession lxs;
    char * padding = "'12345678901234567890123456789012345678901234567890"
		      "12345678901234567890123456789012345678901234567890"
		      "12345678901234567890123456789012345678901234567890"
		      "12345678901234567890123456789012345678901234567890"
		      "12345678901234567890123456789012345678901234567890"; /* 250 bytes (ignore the starting ' and NULL), 5 left*/	
/* bytes in TokString */ /* none */      /* 1 byte */   /* 2 byte */    /* 3 byte */     /* all */
    char* twoByte[]   = {"12345рас'",   "1234рас'",     "",             "",              "123рас'"};  /* splits the 'р' */
	char* threeByte[] = {"12345分裂'",   "1234分裂'",    "123分裂'",     "",              "12分裂'"};   /* splits the '分' */
	char* fourByte[]  = {"12345𝄞𝅘𝅥𝅘𝅥𝅮'",    "1234𝄞𝅘𝅥𝅘𝅥𝅮'",     "123𝄞𝅘𝅥𝅘𝅥𝅮'",      "12𝄞𝅘𝅥𝅘𝅥𝅮'",        "1𝄞𝅘𝅥𝅘𝅥𝅮'"}; /* splits the '𝄞' */
	char in[300];
	char buf[300];
	memcpy(in, padding, 251);

	*tname = "mtlexer-26 test internal buffers splitting utf-8";

	mssInitialize("system", "", "", 0, "test");
	iter = 20000;
	for(i=0;i<iter;i++)
	    {
		setlocale(0, "en_US.UTF-8");
		/* test with character landing after split*/
		runTest(twoByte, 0, in, buf, &lxs);
		runTest(threeByte, 0, in, buf, &lxs);
		runTest(fourByte, 0, in, buf, &lxs);

		/* test with one byte landing before */
		runTest(twoByte, 1, in, buf, &lxs);
		runTest(threeByte, 1, in, buf, &lxs);
		runTest(fourByte, 1, in, buf, &lxs);

		/* test with two bytes landing before */
		runTest(threeByte, 2, in, buf, &lxs);
		runTest(fourByte, 2, in, buf, &lxs);

		/* test with three bytes landing before */
		runTest(fourByte, 3, in, buf, &lxs);

		/* test with all chars landing before */
		runTest(twoByte, 4, in, buf, &lxs);
		runTest(threeByte, 4, in, buf, &lxs);
		runTest(fourByte, 4, in, buf, &lxs);


		/** test without utf-8; will split chars **/
		setlocale(0, "C");
		runNoUTF8Test(twoByte, 0, in, buf, &lxs);
		runNoUTF8Test(threeByte, 0, in, buf, &lxs);
		runNoUTF8Test(fourByte, 0, in, buf, &lxs);

		/* test with one byte landing before */
		runNoUTF8Test(twoByte, 1, in, buf, &lxs);
		runNoUTF8Test(threeByte, 1, in, buf, &lxs);
		runNoUTF8Test(fourByte, 1, in, buf, &lxs);

		/* test with two bytes landing before */
		runNoUTF8Test(threeByte, 2, in, buf, &lxs);
		runNoUTF8Test(fourByte, 2, in, buf, &lxs);

		/* test with three bytes landing before */
		runNoUTF8Test(fourByte, 3, in, buf, &lxs);

		/* test with all chars landing before */
		runNoUTF8Test(twoByte, 4, in, buf, &lxs);
		runNoUTF8Test(threeByte, 4, in, buf, &lxs);
		runNoUTF8Test(fourByte, 4, in, buf, &lxs);
	    }

    return iter;
    }

void 
runTest(char** inputs, int ind, char* in, char* buf,  pLxSession* plxs)
	{
	int flags;
	int t;
	int len;
	/** setup **/
	flags = 0;
	memcpy(in+251, inputs[ind], strlen(inputs[ind]));
	*plxs = mlxStringSession(in, flags);
	assert(*plxs != NULL);
	/* read in the first token. This should read the first 255 bytes */
	t = mlxNextToken(*plxs);
	assert(t == MLX_TOK_STRING);
	assert((*plxs)->Flags & MLX_F_INSTRING);
	assert((*plxs)->TokString != NULL);
	assert(memcmp((*plxs)->TokString, in+1, 255-(ind%4)) == 0);
	assert((*plxs)->ValidateFn((*plxs)->TokString) == 0);
	
	/* now copy to buffer */
	len = mlxCopyToken((*plxs), buf, 300);
	assert(len == (250+strlen(inputs[ind])-1));
	assert(memcmp(in+1, buf, 250+strlen(inputs[ind]-2)) == 0);
	assert((*plxs)->TokType == MLX_TOK_STRING);
	assert(!((*plxs)->Flags & MLX_F_INSTRING));

	switch(ind)
		{
		case 0:
		case 4: 
			assert((*plxs)->TokString[255] == '\0');
			break; 
		case 1:
			assert((*plxs)->TokString[254] == '\0');
			break;
		case 2:
			assert((*plxs)->TokString[253] == '\0');
			break;
		case 3:
			assert((*plxs)->TokString[252] == '\0');
			break;
		default:
			assert(0);
			break;
		}

	mlxCloseSession(*plxs);
	return;
	}

void 
runNoUTF8Test(char** inputs, int ind, char* in, char* buf,  pLxSession* plxs)
	{
	int flags;
	int t;
	/** setup **/
	flags = 0;
	memcpy(in+251, inputs[ind], strlen(inputs[ind]));
	*plxs = mlxStringSession(in, flags);
	assert(*plxs != NULL);
	/* read in the first token. This should read the first 255 bytes */
	t = mlxNextToken(*plxs);
	assert(t == MLX_TOK_STRING);
	assert((*plxs)->Flags & MLX_F_INSTRING);
	assert((*plxs)->TokString != NULL);
	assert(memcmp((*plxs)->TokString, in+1, 255) == 0); /* leaves partial */
	if(ind%4 != 0) assert(chrNoOverlong((*plxs)->TokString) != 0);
	
	/* now copy to buffer */
	int len = mlxCopyToken((*plxs), buf, 300);
	assert(len == (250+strlen(inputs[ind])-1));
	assert(memcmp(in+1, buf, 250+strlen(inputs[ind]-2)) == 0);
	assert((*plxs)->TokType == MLX_TOK_STRING);
	assert(!((*plxs)->Flags & MLX_F_INSTRING));
	assert((*plxs)->TokString[255] == '\0');
	
	mlxCloseSession(*plxs);
	return;
	}	
