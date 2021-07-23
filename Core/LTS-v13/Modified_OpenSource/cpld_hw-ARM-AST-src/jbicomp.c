/****************************************************************************/
/*																			*/
/*	Module:			jbicomp.c												*/
/*																			*/
/*					Copyright (C) Altera Corporation 1997-2001				*/
/*																			*/
/*	Description:	Contains the code for compressing and uncompressing		*/
/*					Boolean array data.										*/
/*																			*/
/*					This algorithm works by searching previous bytes in the */
/*					data that match the current data. If a match is found,	*/
/*					then the offset and length of the matching data can		*/
/*					replace the actual data in the output.					*/
/*																			*/
/*	Revisions:		2.2  fixed /W4 warnings									*/
/*																			*/
/****************************************************************************/


#include "jbiexprt.h"
#include "jbicomp.h"

#define	SHORT_BITS				16
#define	CHAR_BITS				8
#define	DATA_BLOB_LENGTH		3
#define	MATCH_DATA_LENGTH		8192
#define JBI_ACA_REQUEST_SIZE	1024
#define JBI_ACA_BUFFER_SIZE		(MATCH_DATA_LENGTH + JBI_ACA_REQUEST_SIZE)

unsigned long jbi_in_length = 0L;
unsigned long jbi_in_index = 0L;	/* byte index into compressed array */
unsigned int jbi_bits_avail = CHAR_BITS;


unsigned int jbi_bits_required(unsigned int n)
{
	unsigned int result = SHORT_BITS;

	if (n == 0)
	{
		result = 1;
	}
	else
	{
		/* Look for the highest non-zero bit position */
		while ((n & (1 << (SHORT_BITS - 1))) == 0)
		{
			n <<= 1;
			--result;
		}
	}

	return (result);
}

unsigned int jbi_read_packed (unsigned char *buffer, unsigned int bits)
{
	unsigned int result = 0;
	unsigned int shift = 0;
	unsigned int databyte = 0;

	while (bits > 0)
	{
		databyte = buffer[jbi_in_index];

		result |= (((databyte >> (CHAR_BITS - jbi_bits_avail))
			& (0xFF >> (CHAR_BITS - jbi_bits_avail))) << shift);

		if (bits <= jbi_bits_avail)
		{
			result &= (0xFFFF >> (SHORT_BITS - (bits + shift)));
			jbi_bits_avail -= bits;
			bits = 0;
		}
		else
		{
			++jbi_in_index;
			shift += jbi_bits_avail;
			bits -= jbi_bits_avail;
			jbi_bits_avail = CHAR_BITS;
		}
	}

	return (result);
}

unsigned long jbi_uncompress(unsigned char *in, unsigned long in_length, unsigned char *out, unsigned long out_length, int version)
{
	unsigned long i, j, data_length = 0L;
	unsigned int offset, length;
	unsigned int match_data_length = MATCH_DATA_LENGTH;

	if (version > 0) --match_data_length;

	jbi_in_length = in_length;
	jbi_bits_avail = CHAR_BITS;
	jbi_in_index = 0L;
	for (i = 0; i < out_length; ++i) out[i] = 0;

	/* Read number of bytes in data. */
	for (i = 0; i < sizeof (in_length); ++i) 
	{
		data_length = data_length | ((unsigned long)
			jbi_read_packed(in, CHAR_BITS) << (i * CHAR_BITS));
	}

	if (data_length > out_length)
	{
		data_length = 0L;
	}
	else
	{
		i = 0;
		while (i < data_length)
		{
			/* A 0 bit indicates literal data. */
			if (jbi_read_packed(in, 1) == 0)
			{
				for (j = 0; j < DATA_BLOB_LENGTH; ++j)
				{
					if (i < data_length)
					{
						out[i] = (unsigned char) jbi_read_packed(in, CHAR_BITS);
						i++;
					}
				}
			}
			else
			{
				/* A 1 bit indicates offset/length to follow. */
				offset = jbi_read_packed(in, jbi_bits_required((short) (i > match_data_length ? match_data_length : i)));
				length = jbi_read_packed(in, CHAR_BITS);

				for (j = 0; j < length; ++j)
				{
					if (i < data_length)
					{
						out[i] = out[i - offset];
						i++;
					}
				}
			}
		}
	}

	return (data_length);
}
