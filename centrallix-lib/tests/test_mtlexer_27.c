#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "mtsession.h"
#include "mtlexer.h"
#include <assert.h>
#include <locale.h>
#include <time.h>
#include <stdlib.h>

long long
test(char** tname)
    {
    int i;
    int iter;
    int ind, max;
    char cur;

	srand(time(NULL));

	*tname = "mtlexer-27 test internal utf-8 character split detection";

	iter = 50000;
	for(i=0;i<iter;i++)
	    {
		/** make sure all ascii are clear with 1 byte and a random number of bytes left. **/
		for(cur = 0 ; (unsigned char) cur <= 127 ; cur++)
			{
			ind = rand()/2; /* prevent overflows */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+1) == 0);
			max = (rand()/2) + ind + 1; /* can only overflow if both are MAX_INT... unlikely. */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+3) == 0);
			/* if no room, returns -1 */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind - rand()) == -1);
			}

		/** test 2 byte chars headers landing before end and split in half on end (returns 1) **/
		for(cur = 0xC0 ; (unsigned char)cur <=  0xDF ; cur++)
			{
			ind = rand()/2;
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+1) == 1); /* need to leave 1 byte empty */
			max = (rand()/2) + ind + 2;
			assert(mlx_internal_WillSplitUTF8(cur, ind, max) == 0); /* fits */
			}
		
		/** test 3 byte chars headers landing before end and split 2-1 and 1-2 on end (returns 1) **/
		for(cur = 0xE0 ; (unsigned char)cur <=  0xEF ; cur++)
			{
			ind = rand()/2;
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+1) == 1); /* need to leave 2 byte empty */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+2) == 1); /* need to leave 1 byte */
			max = (rand()/2) + ind + 3;
			assert(mlx_internal_WillSplitUTF8(cur, ind, max) == 0); /* fits */
			}
		
		/** test 4 byte chars headers landing before and split 3:1, 2:2, and 3:1 on end (returns 1) **/
		for(cur = 0xF0 ; (unsigned char)cur <= 0xF4 ; cur++)
			{
			ind = rand()/2;
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+1) == 1); /* need to leave 3 bytes empty */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+2) == 1); /* need to leave 2 bytes */
			assert(mlx_internal_WillSplitUTF8(cur, ind, ind+3) == 1); /* need to leave 1 byte */
			max = (rand()/2) + ind + 4;
			assert(mlx_internal_WillSplitUTF8(cur, ind, max) == 0); /* fits */
			}
	    }

    return iter;
    }

