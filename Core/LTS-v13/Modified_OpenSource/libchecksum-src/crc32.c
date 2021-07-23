/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2010, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************/
#include "crc32.h"
#include "checksum.h"

unsigned long CalculateCRC32(unsigned char *Buffer, unsigned long Size)
{
    unsigned long i,crc32 = 0xFFFFFFFF;

	/* Read the data and calculate crc32 */	
    for(i = 0; i < Size; i++)
		crc32 = ((crc32) >> 8) ^ CrcLookUpTable[(Buffer[i]) 
								^ ((crc32) & 0x000000FF)];
	return ~crc32;
}

#if defined (__x86_64__) || defined (WIN64) || defined (__aarch64__)
void BeginCRC32(unsigned int *crc32)
#else
void BeginCRC32(unsigned long *crc32)
#endif
{
	*crc32 = 0xFFFFFFFF;
	return;
}

#if defined (__x86_64__) || defined (WIN64) || defined (__aarch64__)
void DoCRC32(unsigned int *crc32, unsigned char Data)
#else
void DoCRC32(unsigned long *crc32, unsigned char Data)
#endif
{
	*crc32=((*crc32) >> 8) ^ CrcLookUpTable[Data ^ ((*crc32) & 0x000000FF)];
	return;
}

#if defined (__x86_64__) || defined (WIN64) || defined (__aarch64__)
void EndCRC32(unsigned int *crc32)
#else
void EndCRC32(unsigned long *crc32)
#endif
{
	*crc32 = ~(*crc32);
	return;
}

