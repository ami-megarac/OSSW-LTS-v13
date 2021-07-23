

#ifndef DECODE_H
#define DECODE_H

//#include "videopkt_soc.h"
#include "jtables.h"
#include "vq.h"
#include "def.h"

// #define BYTE unsigned char
// #define WORD unsigned short int
// 
// #define DWORD unsigned int
// #define SDWORD signed int
// 
// #define SBYTE signed char
// #define SWORD signed short int

#define BOOLEAN int 
// Used markers:
#define SOI 0xD8
#define EOI 0xD9
#define APP0 0xE0
#define SOF 0xC0
#define DQT 0xDB
#define DHT 0xC4
#define SOS 0xDA
#define DRI 0xDD
#define COM 0xFE
#define POWEROFTOW 17

#define BYTE_p(i) bp=buf[(i)++]
#define RIGHT_SHIFT_DECODE(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~(0L)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#define DESCALE(x,n)  RIGHT_SHIFT_DECODE((x) + (1L << ((n)-1)), n)
#define RANGE_MASK 1023L

#define LEN64		64
#define LEN256  256
#define LEN65536	65536

//used for bits right shift processing (32-k)
#define BITS32		32 

//used for updating rects in bitmap window 
#define MAX_INVALID_RECTS			256

//used for quantization table
#define QTTBLCOUNT	4
#define RANGE_LIMIT_TABLE_SIZE		((5 * 256) + 128)

/*struct COLOR_CACHE {
	unsigned long Color[4];
	unsigned char Index[4];
	unsigned char BitMapBits;
};*/

unsigned long *Buffer;
unsigned long _index;
unsigned long codebuf;
unsigned long newbuf;
unsigned long readbuf;
int txb;
int tyb;
int prev_txb;
int prev_tyb;
int tile_count;

// DC Huffman table number for Y,Cb, Cr
BYTE  YDC_nr,CbDC_nr,CrDC_nr; 

// AC Huffman table number for Y,Cb, Cr
BYTE  YAC_nr,CbAC_nr,CrAC_nr; 

BYTE  Y[LEN64],Cb[LEN64],Cr[LEN64];

BYTE  rlimit_table[RANGE_LIMIT_TABLE_SIZE];

//the buffer we use for storing the entire JPG file
BYTE  *buf; 

//current byte
BYTE  bp; 

//current byte position
DWORD byte_pos; 

SWORD DCY, DCCb, DCCr;
SWORD DCT_coeff[384];

int	   position ;
int    SCALEFACTOR;
int    SCALEFACTORUV;
int    ADVANCESCALEFACTOR;
int    ADVANCESCALEFACTORUV;
int    selector, advance_selector, old_selector;
int    Mapping;
int    txb, tyb, oldxb, oldyb;
int    SharpModeSelection;
int    codesize, newbits;
long   pixels;
BYTE   Mode420, Old_Mode_420;

struct rc4_state s;
struct COMPRESSHEADER yheader;

BOOLEAN DecodeRC4State ;

unsigned int iMax;

// WIDTH and HEIGHT are the modes your display used
unsigned long WIDTH;
unsigned long HEIGHT;
unsigned long real_WIDTH;
unsigned long real_HEIGHT;
unsigned long tmp_HEIGHT;
unsigned long tmp_WIDTH;

unsigned char DecodeKeys[LEN256];

//DC huffman tables , no more than 4 (0..3)
Huffman_table HTDC[QTTBLCOUNT]; 

//AC huffman tables                  (0..3)
Huffman_table HTAC[QTTBLCOUNT]; 

// tabelele de supraesantionare pt cele 4 blocuri
BYTE  tab_1[LEN64],tab_2[LEN64],tab_3[LEN64],tab_4[LEN64]; 

// Precalculated Cr, Cb tables
SWORD Cr_tab[LEN256],Cb_tab[LEN256]; 
SWORD Cr_Cb_green_tab[LEN65536];

// w1 = First word in memory; w2 = Second word
WORD  w1,w2; 

// the actual used value in Huffman decoding.
DWORD wordval ; 

DWORD mask[POWEROFTOW];
SWORD neg_pow2[POWEROFTOW];

// quantization tables, no more than 4 quantization tables (QT[0..3])
long QT[QTTBLCOUNT][LEN64]; 
int   m_CrToR[LEN256], m_CbToB[LEN256], m_CrToG[LEN256], m_CbToG[LEN256], m_Y[LEN256];

void updatereadbuf(int walks);
void MoveBlockIndex (short Mode420);
void dump_buffer(char *Buffer, int size);
#endif //DECODE_H
