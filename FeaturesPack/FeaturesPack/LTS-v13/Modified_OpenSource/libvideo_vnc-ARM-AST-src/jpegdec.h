
#ifndef __JPEGDEC_H__
#define __JPEGDEC_H__ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef char CHAR;
typedef char int8;
typedef signed char int8s;
typedef unsigned char uint8;

#ifndef __KERNEL__
typedef unsigned char u8;
#endif

typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef unsigned char UINT8;

typedef short int16;
typedef short INT16;
typedef unsigned short uint16;

#ifndef __KERNEL__
typedef unsigned short u16;
#endif

typedef int int32;
typedef unsigned int uint32;
typedef unsigned short TWOBYTES;
typedef unsigned short UINT16;
typedef unsigned short WORD;

#ifndef __KERNEL__
typedef unsigned int u32;
#endif

typedef unsigned int FOURBYTES;
typedef unsigned long long uint64;

// 
// #define WORD unsigned short int
#define DWORD unsigned int
#define SDWORD signed int
// 
#define SBYTE signed char
#define SWORD signed short int

int load_JPEG_header(FILE *fp, DWORD *X_image, DWORD *Y_image);
void decode_JPEG_image();
int get_JPEG_buffer(WORD X_image,WORD Y_image, BYTE **address_dest_buffer);

#endif
