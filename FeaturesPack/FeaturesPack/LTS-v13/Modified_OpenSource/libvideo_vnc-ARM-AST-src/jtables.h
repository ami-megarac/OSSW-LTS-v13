
#ifndef JTABLES_H
#define JTABLES_H

#include "jpegdec.h"

extern BYTE zigzag[];

extern BYTE dezigzag[];

extern BYTE *std_luminance_qt;
extern BYTE *std_chrominance_qt;

// Standard Huffman tables (cf. JPEG standard section K.3) */

extern BYTE std_dc_luminance_nrcodes[];
extern BYTE std_dc_luminance_values[];

extern BYTE std_dc_chrominance_nrcodes[];
extern BYTE std_dc_chrominance_values[];

extern BYTE std_ac_luminance_nrcodes[];
extern BYTE std_ac_luminance_values[];

extern BYTE std_ac_chrominance_nrcodes[];
extern BYTE std_ac_chrominance_values[];


extern WORD DC_LUMINANCE_HUFFMANCODE[];

extern WORD DC_CHROMINANCE_HUFFMANCODE[];

extern WORD AC_LUMINANCE_HUFFMANCODE[];

extern WORD AC_CHROMINANCE_HUFFMANCODE[];

//[]=========================
extern BYTE Tbl_100Y[];
extern BYTE Tbl_100UV[];

//[]=========================
extern BYTE Tbl_086Y[];
extern BYTE Tbl_086UV[];

//[]=========================
extern BYTE Tbl_071Y[];
extern BYTE Tbl_071UV[];
//[]=========================
extern BYTE Tbl_057Y[];
extern BYTE Tbl_057UV[];

//[]=========================
extern BYTE Tbl_043Y[];
extern BYTE Tbl_043UV[];

//[]=========================

extern BYTE Tbl_029Y[];
extern BYTE Tbl_029UV[];

//[]=========================
extern BYTE Tbl_014Y[];
extern BYTE Tbl_014UV[];
//[]=========================
extern BYTE Tbl_000Y[];
extern BYTE Tbl_000UV[];

typedef struct {
   BYTE Length[17];  // k =1-16 ; L[k] indicates the number of Huffman codes of length k
   WORD minor_code[17];  // indicates the value of the smallest Huffman code of length k
   WORD major_code[17];  // similar, but the highest code
   BYTE V[65536];  // V[k][j] = Value associated to the j-th Huffman code of length k
	// High nibble = nr of previous 0 coefficients
	// Low nibble = size (in bits) of the coefficient which will be taken from the data stream
   BYTE Len[65536];
} Huffman_table;

#endif //JTABLES_H
