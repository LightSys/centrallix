#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include "cxss/crypto.h"

long long
test(char** name)
    {
     *name = "CXSS Crypto 03: Gen random key randomness";


    const int n = 5000;
    const int b = 32;
    unsigned char keys [n][b];
    unsigned char bytes [n*b];
    int freq [256];

    /** init byte freq **/
    for(int i = 0 ; i < 256 ; i++)
        {
        freq[i] = 0;
        }

    /** Setup **/
    cxss_internal_InitEntropy(1280); /* must be into first */
    cxssCryptoInit();
    
    /** get n values **/ 
    for(int i = 0 ; i < n ; i++)
        {
        int result = cxssGenerate256bitRandomKey(&keys[i][0]);
        assert(result == CXSS_CRYPTO_SUCCESS);

        /** update byte freq and store bytes **/
        for(int j = 0 ; j < b ; j++)
            {
            bytes[i*b + j] = keys[i][j];
            freq[keys[i][j]]++; 
            }
        

        /** Test for duplicates**/
        for(int j = 0 ; j < i ; j++)
            {
            int isMatch = 1;
            for(int k = 0 ; k < b ; k++)
                {
                isMatch &= keys[i][k] == keys[j][k];
                }
            assert(!isMatch);
            } 
        }

    /** Calculate standard dev for frequency **/
    double mean = ((double) (n*b))/256.0; 
    long double avgDev = 0;
    for(int i = 0 ; i < 256 ; i++)
        {
        long double cur = (freq[i] - mean)*(freq[i] - mean);
        avgDev += cur;
        }
    avgDev /= 256;
    double stdDev = sqrt(avgDev);

    printf("Mean    : %f\n", mean);
    printf("Std Dev : %f\n", stdDev);
    printf("Dev/mean: %f%%\n", stdDev/mean*100);

    /** Make sure standard Deviation is good **/
    assert(stdDev/mean <= 0.1);
    

    cxssCryptoCleanup();

    return 0;
    }
