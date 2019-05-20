/*
 * sha1.c
 *
 * Description:
 * This file implements the Secure Hashing Algorithm 1 as
 * defined in FIPS PUB 180-1 published April 17, 1995.
 *
 * Optimization of reference code through loop-unrolling by Sandeep raju (sandeep.rv@gmail.com)
 * Refer to below author's guide for unrolling algorithm details -
 * https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=4760668
 *
 * Portability Issues:
 * SHA-1 is defined in terms of 32-bit "words". This code
 * uses <stdint.h> (included via "sha1.h" to define 32 and 8
 * bit unsigned integer types. If your C compiler does not
 * support 32 bit unsigned integers, this code is not
 * appropriate.
 *
 * Caveats:
 * SHA-1 is designed to work with messages less than 2^64 bits
 * long. Although SHA-1 allows a message digest to be generated
 * for messages of any number of bits less than 2^64, this
 * implementation only works with messages with a length that is
 * a multiple of the size of an 8-bit character and total number
 * of bytes in full input message should be less than 2**29 bytes
 *
 *
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



#include "sha1.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits,word) \
 (((word) << (bits)) | ((word) >> (32-(bits))))
/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);
/*
 * SHA1Reset
 *
 * Description:
 * This function will initialize the SHA1Context in preparation
 * for computing a new SHA1 message digest.
 *
 * Parameters:
 * context: [in/out]
 * The context to reset.
 *
 * Returns:
 * sha Error Code.
 *
 */
int SHA1Reset(SHA1Context *context)
{
     if (!context)
     {
     return shaNull;
     }
     context->Length = 0;
     context->Message_Block_Index = 0;
     context->Intermediate_Hash[0] = 0x67452301;
     context->Intermediate_Hash[1] = 0xEFCDAB89;
     context->Intermediate_Hash[2] = 0x98BADCFE;
     context->Intermediate_Hash[3] = 0x10325476;
     context->Intermediate_Hash[4] = 0xC3D2E1F0;
     context->Computed = 0;
     context->Corrupted = 0;

     return shaSuccess;
     }

/*
 * SHA1Result
 *
 * Description:
 * This function will return the 160-bit message digest into the
 * Message_Digest array provided by the caller.
 * NOTE: The first octet of hash is stored in the 0th element,
 * the last octet of hash in the 19th element.
 *
 * Parameters:
 * context: [in/out]
 * The context to use to calculate the SHA-1 hash.
 * Message_Digest: [out]
 * Where the digest is returned.
 *
 * Returns:
 * sha Error Code.
 *
 */

int SHA1Result( SHA1Context *context,
 uint8_t Message_Digest[SHA1HashSize])
{
     int i;
     if (!context || !Message_Digest)
     {
        return shaNull;
     }
     if (context->Corrupted)
     {
        return context->Corrupted;
     }
     if (!context->Computed)
     {
         SHA1PadMessage(context);
         for(i=0; i<64; ++i)
         {
         /* message may be sensitive, clear it out */
         context->Message_Block[i] = 0;
         }
         context->Length = 0; /* and clear length */
         context->Computed = 1;
     }

    for(i = 0; i < SHA1HashSize; ++i)
    {
        Message_Digest[i] = context->Intermediate_Hash[i>>2]
        >> 8 * ( 3 - ( i & 0x03 ) );
    }
    return shaSuccess;
 }

 /*
  * SHA1Input
  *
  * Description:
  * This function accepts an array of octets as the next portion
  * of the message.
  *
  * Parameters:
  * context: [in/out]
  * The SHA context to update
  * message_array: [in]
  * An array of characters representing the next portion of
  * the message.
  * length: [in]
  * The length of the message in message_array
  *
  * Returns:
  * sha Error Code.
  *
  */
 int SHA1Input( SHA1Context *context,
  const uint8_t *message_array,
  unsigned length)
{
    unsigned int counter = length >> 6; /* Divide by 64 */
    unsigned int rem = length & 0x3F;
    unsigned int i,j = 0;

    if (!length)
    {
        return shaSuccess;
    }
    if (!context || !message_array)
    {
        return shaNull;
    }
    if (context->Computed)
    {
        context->Corrupted = shaStateError;
        return shaStateError;
    }

    if (context->Corrupted)
    {
        return context->Corrupted;
    }

    unsigned int *ptr = (unsigned int *)&message_array[0];
    for (i = 0; i < counter; i++)
    {
        unsigned int *msgblk = (unsigned int *)&context->Message_Block[0];
        for(j=0; j < 16; j++, ptr++, msgblk++)
        {
            *msgblk = *ptr;
        }

        SHA1ProcessMessageBlock(context);
    }

    unsigned char *ptrrem = (unsigned char *) ptr;
    for (i = 0; i < rem; i++, ptrrem++)
    {
        context->Message_Block[i] = *ptrrem;
    }

    context->Message_Block_Index = rem;
    context->Length = length << 3;

    return shaSuccess;
}


 /*
  * SHA1ProcessMessageBlock
  *
  * Description:
  * This function will process the next 512 bits of the message
  * stored in the Message_Block array.
  *
  * Parameters:
  * None.
  *
  * Returns:
  * Nothing.
  *
  * Comments:
  *
  *
  *  * Many of the variable names in this code, especially the
 * single character names, were used because those were the
 * names used in the publication.
 *
 *
 */

void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const uint32_t K[] = { /* Constants defined in SHA-1 */
    0x5A827999,
    0x6ED9EBA1,
    0x8F1BBCDC,
    0xCA62C1D6
    };
    int t; /* Loop counter */
    uint32_t W[80]; /* Word sequence */
    uint32_t A, B, C, D, E; /* Word buffers */
    uint32_t TA, TB; /* temp variables */
    /*
    * Initialize the first 16 words in the array W
    */
    for(t = 0; t < 16; t++)
    {
      W[t] = context->Message_Block[t * 4] << 24;
      W[t] |= context->Message_Block[t * 4 + 1] << 16;
      W[t] |= context->Message_Block[t * 4 + 2] << 8;
      W[t] |= context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
    {
      W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    /* Get initial value before starting 80 rounds */

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];


    /* Stage 1 */
    for(t = 0; t < 20; t = t + 2)
    {
        TB = SHA1CircularShift(5, A) + E + W[t] + K[0] + ((B & C) | ((~B) & D));
        TA = SHA1CircularShift(5, TB) + D + W[t+1] + K[0] + ((A & SHA1CircularShift(30, B)) | ((~A) & C));
        E = C;
        D = SHA1CircularShift(30, B);
        C = SHA1CircularShift(30, A);

        A = TA;
        B = TB;
    }

    /* Stage 2 */
    for(t = 20; t < 40; t = t + 2)
    {
        TB = SHA1CircularShift(5, A) + E + W[t] + K[1] + (B ^ C ^ D);
        TA = SHA1CircularShift(5, TB) + D + W[t+1] + K[1] + (A ^ SHA1CircularShift(30, B) ^ C);
        E = C;
        D = SHA1CircularShift(30, B);
        C = SHA1CircularShift(30, A);

        A = TA;
        B = TB;
    }

    /* Stage 3 */
    for(t = 40; t < 60; t = t + 2)
    {
        TB = SHA1CircularShift(5, A) + E + W[t] + K[2] + ((B & C) | (B & D) | (C & D)) ;
        TA = SHA1CircularShift(5, TB) + D + W[t+1] + K[2] + ((A & SHA1CircularShift(30, B)) | (A & C) | (SHA1CircularShift(30, B) & C)) ;
        E = C;
        D = SHA1CircularShift(30, B);
        C = SHA1CircularShift(30, A);

        A = TA;
        B = TB;
    }

    /* Stage 4 */
    for(t = 60; t < 80; t = t + 2)
    {
        TB = SHA1CircularShift(5, A) + E + W[t] + K[3] + (B ^ C ^ D);
        TA = SHA1CircularShift(5, TB) + D + W[t+1] + K[3] + (A ^ SHA1CircularShift(30, B) ^ C);
        E = C;
        D = SHA1CircularShift(30, B);
        C = SHA1CircularShift(30, A);

        A = TA;
        B = TB;
    }

    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;
    context->Message_Block_Index = 0;
  }


  /*
   * SHA1PadMessage
   *
   *
   * Description:
 * According to the standard, the message must be padded to an even
 * 512 bits. The first padding bit must be a 1. The last 64
 * bits represent the length of the original message. All bits in
 * between should be 0. This function will pad the message
 * according to those rules by filling the Message_Block array
 * accordingly. It will also call the ProcessMessageBlock function
 * provided appropriately. When it returns, it can be assumed that
 * the message digest has been computed.
 *
 * Parameters:
 * context: [in/out]
 * The context to pad
 * ProcessMessageBlock: [in]
 * The appropriate SHA*ProcessMessageBlock function
 * Returns:
 * Nothing.
 *
 */
void SHA1PadMessage(SHA1Context *context)
{
 /*
 * Check to see if the current message block is too small to hold
 * the initial padding bits and length. If so, we will pad the
 * block, process it, and then continue padding into a second
 * block.
 */
     if (context->Message_Block_Index > 55)
     {
     context->Message_Block[context->Message_Block_Index++] = 0x80;
     while(context->Message_Block_Index < 64)
     {
     context->Message_Block[context->Message_Block_Index++] = 0;
     }
     SHA1ProcessMessageBlock(context);
     while(context->Message_Block_Index < 56)
     {
     context->Message_Block[context->Message_Block_Index++] = 0;
     }
     }
     else
     {
     context->Message_Block[context->Message_Block_Index++] = 0x80;
     while(context->Message_Block_Index < 56)
     {

         context->Message_Block[context->Message_Block_Index++] = 0;
         }
     }

     /*
     * Store the message length as the last 8 octets
     */
     context->Message_Block[56] = 0;
     context->Message_Block[57] = 0;
     context->Message_Block[58] = 0;
     context->Message_Block[59] = 0;
     context->Message_Block[60] = context->Length >> 24;
     context->Message_Block[61] = context->Length >> 16;
     context->Message_Block[62] = context->Length >> 8;
     context->Message_Block[63] = context->Length;
     SHA1ProcessMessageBlock(context);
}




