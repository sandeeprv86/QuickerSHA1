/*
 * main.c
 *
 * Description:
 * This file will exercise the SHA-1 code performing the three
 * tests documented in FIPS PUB 180-1 plus one which calls
 * SHA1Input with an exact multiple of 512 bits, plus a few
 * error test checks.
 *
 * Portability Issues:
 * None.
 *
 * Optimization of reference code through loop-unrolling by Sandeep raju (sandeep.rv@gmail.com)
 * Refer to below author's guide for unrolling algorithm details -
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=4760668
 *
 *
 */

/*
 Copyright (C) The Internet Society (2001). All Rights Reserved.
 This document and translations of it may be copied and furnished to
 others, and derivative works that comment on or otherwise explain it
 or assist in its implementation may be prepared, copied, published
 and distributed, in whole or in part, without restriction of any
 kind, provided that the above copyright notice and this paragraph are
 included on all such copies and derivative works. However, this
 document itself may not be modified in any way, such as by removing
 the copyright notice or references to the Internet Society or other
 Internet organizations, except as needed for the purpose of
 developing Internet standards in which case the procedures for
 copyrights defined in the Internet Standards process must be
 followed, or as required to translate it into languages other than
 English.
 The limited permissions granted above are perpetual and will not be
 revoked by the Internet Society or its successors or assigns.
 This document and the information contained herein is provided on an
 "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
 TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
 HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
 MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sha1.h"

/*
 * Define patterns for testing
 */

//char *testarray = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
char *testarray = "abcdefghijklmnopqrstuvwxyz";

int main()
{
     SHA1Context sha;
     int i, err;
     uint8_t Message_Digest[20];
    /*
    * Perform SHA-1 tests
    */
    printf( "\n Input: '%s'\n", testarray);

    err = SHA1Reset(&sha);
    if (err)
    {
        fprintf(stderr, "SHA1 Reset Error %d.\n", err );
        return -1;
    }

    err = SHA1Input(&sha,(const unsigned char *) testarray, strlen(testarray));
    if (err)
    {
        fprintf(stderr, "SHA1 Input Error %d.\n", err );
        return -1;
    }

    err = SHA1Result(&sha, Message_Digest);
    if (err)
    {
        fprintf(stderr, "SHA1 Result Error %d, could not compute message digest.\n", err);
    }
    else
    {
        printf("\t");
        for(i = 0; i < 20 ; ++i)
        {
          printf("%02X ", Message_Digest[i]);
        }
        printf("\n");
    }

    return 0;
}
