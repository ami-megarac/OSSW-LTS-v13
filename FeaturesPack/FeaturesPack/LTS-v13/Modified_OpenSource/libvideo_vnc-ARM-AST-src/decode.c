

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

// #include "coreTypes.h"
#include "decode.h" 

#define VIDEO_HDR_BUF_SZ (16 * 1024)
#define VIDEO_DATA_BUF_SZ (4 * 1024 * 1024)
#define VIDEO_BUF_SZ (VIDEO_HDR_BUF_SZ + VIDEO_DATA_BUF_SZ)
#define VIDEO_HDR_VIDEO_SZ (4 * 1024)

#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define TRUE 1
#define FALSE 0

typedef struct {
	uint16_t  iEngVersion;
	uint16_t  wHeaderLen;

	// SourceModeInfo
	uint16_t  SourceMode_X;
	uint16_t  SourceMode_Y;
	uint16_t  SourceMode_ColorDepth;
	uint16_t  SourceMode_RefreshRate;
	uint8_t    SourceMode_ModeIndex;

	// DestinationModeInfo
	uint16_t  DestinationMode_X;
	uint16_t  DestinationMode_Y;
	uint16_t  DestinationMode_ColorDepth;
	uint16_t  DestinationMode_RefreshRate;
	uint8_t    DestinationMode_ModeIndex;

	// FRAME_HEADER
	uint32_t   FrameHdr_StartCode;
	uint32_t   FrameHdr_FrameNumber;
	uint16_t  FrameHdr_HSize;
	uint16_t  FrameHdr_VSize;
	uint32_t   FrameHdr_Reserved[2];
	uint8_t    FrameHdr_CompressionMode;
	uint8_t    FrameHdr_JPEGScaleFactor;
	uint8_t    FrameHdr_JPEGTableSelector;
	uint8_t    FrameHdr_JPEGYUVTableMapping;
	uint8_t    FrameHdr_SharpModeSelection;
	uint8_t    FrameHdr_AdvanceTableSelector;
	uint8_t    FrameHdr_AdvanceScaleFactor;
	uint32_t   FrameHdr_NumberOfMB;
	uint8_t    FrameHdr_RC4Enable;
	uint8_t    FrameHdr_RC4Reset;
	uint8_t    FrameHdr_Mode420;

	// INF_DATA
	uint8_t    InfData_DownScalingMethod;
	uint8_t    InfData_DifferentialSetting;
	uint16_t  InfData_AnalogDifferentialThreshold;
	uint16_t  InfData_DigitalDifferentialThreshold;
	uint8_t    InfData_ExternalSignalEnable;
	uint8_t    InfData_AutoMode;
	uint8_t    InfData_VQMode;

	// COMPRESS_DATA
	uint32_t   CompressData_SourceFrameSize;
	uint32_t   CompressData_CompressSize;
	uint32_t   CompressData_HDebug;
	uint32_t   CompressData_VDebug;

	uint8_t    InputSignal;
	uint16_t	Cursor_XPos;
	uint16_t	Cursor_YPos;
} __attribute__ ((packed)) frame_hdr_t;

unsigned long tmp_HEIGHT;
unsigned long tmp_WIDTH;

//DC huffman tables , no more than 4 (0..3)
Huffman_table HTDC[QTTBLCOUNT]; 

//AC huffman tables                  (0..3)
Huffman_table HTAC[QTTBLCOUNT]; 


int prev_yuv[1920*1200*4];

void Keys_Expansion (unsigned char *key);
void DecodeRC4_setup( struct rc4_state *s, unsigned char *key);
void RC4_crypt( struct rc4_state *s, unsigned char *data, int length );

WORD WORD_hi_lo(BYTE byte_high,BYTE byte_low)
{
	return ((((WORD)byte_high) << 8) | (WORD)byte_low);
}

/***************************************************************************
Function		: prepare_range_limit_table
Purpose			: Allocate and fill in the sample_range_limit table 
Return Type		: -
Return Value	: None
Arguments		: None
 ****************************************************************************/
void prepare_range_limit_table()
// Allocate and fill in the sample_range_limit table 
{
	int j;
	//  rlimit_table = (BYTE *)malloc(5 * 256L + 128) ;
	// First segment of "simple" table: limit[x] = 0 for x < 0 
	memset((void *)rlimit_table,0,LEN256);
	//rlimit_table += LEN256;	// allow negative subscripts of simple table 
	//Main part of "simple" table: limit[x] = x  
	for(j = 0; j < LEN256; j++)
		rlimit_table[j+256] = j;
	// End of simple table, rest of first half of post-IDCT table  
	for(j = LEN256; j < 640; j++)
		rlimit_table[j] = 255;

	// Second half of post-IDCT table 
	memset((void *)(rlimit_table + 640),0,384);
	for(j = 0; j < 128 ; j++)
		rlimit_table[j+1024] = j;
}


/***************************************************************************
Function		: lookKbits
Purpose			: gets the value from codebuf by right shifting by specified k value
Return Type		: WORD
Return Value	: revcode
Arguments		: k value to make right shift operation
 ****************************************************************************/

WORD lookKbits(BYTE k)
{
	WORD revcode;

	revcode= (WORD)(codebuf>>(32-k));

	return(revcode);

}


/***************************************************************************
Function		: skipKbits
Purpose			: Updating the codebuf values by specific right shift operation
Return Type		: -
Return Value	: None
Arguments		: k value to make right shift operation
 ****************************************************************************/
void skipKbits(BYTE k)
{
	unsigned long readbuf;

	if ((newbits-k)<=0) {
		readbuf = Buffer[_index];
		codebuf=(codebuf<<k)|((newbuf|(readbuf>>(newbits)))>>(32-k));
		newbuf=readbuf<<(k-newbits);
		newbits=32+newbits-k;
		_index ++;
	} else {
		codebuf=(codebuf<<k)|(newbuf>>(32-k));
		newbuf=newbuf<<k;
		newbits-=k;
	}
}

/***************************************************************************
Function		: getKbits
Purpose			: gets the value from codebuf by right shifting by specified k value
Return Type		: SWORD
Return Value	: signed_wordvalue
Arguments		: k value to make right shift operation
 ****************************************************************************/
short getKbits(BYTE k)
{
	short signed_wordvalue;

	//river
	signed_wordvalue=lookKbits(k);
	//signed_wordvalue=(WORD)(codebuf>>(32-k));
	if (((1L<<(k-1))& signed_wordvalue)==0)
		signed_wordvalue=signed_wordvalue+neg_pow2[k];

	skipKbits(k);
	return signed_wordvalue;
}

/***************************************************************************
Function		: init_QT
Purpose			: Initializing 4 quantization tables , each having 64 byte.
Return Type		: None
Return Value	: None
Arguments		: None
 ****************************************************************************/
void init_QT()
{
	int i, val;

	position=0;
	DecodeRC4State=FALSE;
	//YQ_nr=0;CbQ_nr=1;CrQ_nr=1; // quantization table number for Y, Cb, Cr
	YDC_nr=0;CbDC_nr=1;CrDC_nr=1; // DC Huffman table number for Y,Cb, Cr
	YAC_nr=0;CbAC_nr=1;CrAC_nr=1; // AC Huffman table number for Y,Cb, Cr
	///d_k=0;	// Bit displacement in memory, relative to the offset of w1
	// it's always <16

	for(i=1;i<POWEROFTOW;i++)
	{
		val= (int)(1-(pow(2,i)));
		neg_pow2[i]=val;
	}


	old_selector = 255;


	Old_Mode_420 = 1;
}

/***************************************************************************
Function		: set_quant_table
Purpose			: Set quantization table and zigzag reorder it
Return Type		: None
Return Value	: None
Arguments		: Byte pointer to basic_table , scale_factor & newtable
 ****************************************************************************/
void set_quant_table(BYTE *basic_table,BYTE scale_factor,BYTE *newtable)
// Set quantization table and zigzag reorder it
{
	BYTE i;
	long temp;
	for (i = 0; i < LEN64; i++)
	{
		temp = ((long) (basic_table[i] * 16)/scale_factor);
		// limit the values to the valid range 
		if (temp <= 0L)
			temp = 1L;
		if (temp > 255L)
			temp = 255L; // limit to baseline range if requested 
		newtable[zigzag[i]] = (BYTE) temp;
	}
}

/***************************************************************************
Function		: load_quant_table
Purpose			: Load quantization coefficients from JPG file, scale them for DCT and reorder
				  based on selector assigned from  VideoEngineInfo->FrameHeader.JPEGTableSelector;	
				  Filling 64 bytes by usning scalefactor into first quantization table
Return Type		: None
Return Value	: None
Arguments		: Byte pointer to Quantization table QT[0]
 ****************************************************************************/
void load_quant_table(long  *quant_table)
{
	float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
			1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
	BYTE j,row,col;
	BYTE tempQT[LEN64];
	//int  i;
	// Load quantization coefficients from JPG file, scale them for DCT and reorder
	// from zig-zag order
	switch (selector) {
	case 0:
		std_luminance_qt = Tbl_000Y;
		break;
	case 1:
		std_luminance_qt = Tbl_014Y;
		break;
	case 2:
		std_luminance_qt = Tbl_029Y;
		break;
	case 3:
		std_luminance_qt = Tbl_043Y;
		break;
	case 4:
		std_luminance_qt = Tbl_057Y;
		break;
	case 5:
		std_luminance_qt = Tbl_071Y;
		break;
	case 6:
		std_luminance_qt = Tbl_086Y;
		break;
	case 7:
		std_luminance_qt = Tbl_100Y;
		break;
	default:
		printf("\nload_quant_table : Unknown selector : %d\n", selector);
	}
	set_quant_table(std_luminance_qt, (BYTE)SCALEFACTOR, tempQT);

	for (j=0;j<=63;j++) quant_table[j]=tempQT[zigzag[j]];
	j=0;
	for (row=0;row<=7;row++)
		for (col=0;col<=7;col++) {
			quant_table[j]=(long)(quant_table[j] * (scalefactor[row]*scalefactor[col])) * 65536;
			j++;
		}
	byte_pos+=LEN64;
}

/***************************************************************************
Function		: load_quant_tableCb
Purpose			: Load quantization coefficients from JPG file, scale them for DCT and reorder
				  based on selector assigned from  VideoEngineInfo->FrameHeader.JPEGTableSelector;	
				  Filling 64 bytes by usning scalefactor into second quantization table
Return Type		: None
Return Value	: None
Arguments		: Byte pointer to Quantization table QT[1]
 ****************************************************************************/
void load_quant_tableCb(long *quant_table)
{
	float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
			1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
	BYTE j,row,col;
	BYTE tempQT[LEN64];
	//int  i;
	// Load quantization coefficients from JPG file, scale them for DCT and reorder
	// from zig-zag order
	if (Mapping == 1) {
		switch (selector) {
		case 0:
			std_chrominance_qt = Tbl_000Y;
			break;
		case 1:
			std_chrominance_qt = Tbl_014Y;
			break;
		case 2:
			std_chrominance_qt = Tbl_029Y;
			break;
		case 3:
			std_chrominance_qt = Tbl_043Y;
			break;
		case 4:
			std_chrominance_qt = Tbl_057Y;
			break;
		case 5:
			std_chrominance_qt = Tbl_071Y;
			break;
		case 6:
			std_chrominance_qt = Tbl_086Y;
			break;
		case 7:
			std_chrominance_qt = Tbl_100Y;
			break;
		default:
			printf("\nload_quant_table : Unknown selector : %d\n", selector);
		}
	}
	else {
		switch (selector) {
		case 0:
			std_chrominance_qt = Tbl_000UV;
			break;
		case 1:
			std_chrominance_qt = Tbl_014UV;
			break;
		case 2:
			std_chrominance_qt = Tbl_029UV;
			break;
		case 3:
			std_chrominance_qt = Tbl_043UV;
			break;
		case 4:
			std_chrominance_qt = Tbl_057UV;
			break;
		case 5:
			std_chrominance_qt = Tbl_071UV;
			break;
		case 6:
			std_chrominance_qt = Tbl_086UV;
			break;
		case 7:
			std_chrominance_qt = Tbl_100UV;
			break;
		default:
			printf("\nload_quant_table : Unknown selector : %d\n", selector);
		}
	}
	set_quant_table(std_chrominance_qt, (BYTE)SCALEFACTORUV, tempQT);

	for (j=0;j<=63;j++) quant_table[j]=tempQT[zigzag[j]];
	j=0;
	for (row=0;row<=7;row++)
		for (col=0;col<=7;col++) {
			quant_table[j]= (long)(quant_table[j] * (scalefactor[row]*scalefactor[col])) * 65536;
			j++;
		}
	byte_pos+=LEN64;
}

/***************************************************************************
Function		: load_advance_quant_table
Purpose			: Load quantization coefficients from JPG file, scale them for DCT and reorder
				  based on selector assigned from  VideoEngineInfo->FrameHeader.JPEGTableSelector;	
				  Filling 64 bytes by usning scalefactor into third quantization table
				  //  Note: Added for Dual_JPEG
Return Type		: None
Return Value	: None
Arguments		: Byte pointer to Quantization table QT[2]
 ****************************************************************************/
void load_advance_quant_table(long *quant_table)
{
	float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
			1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
	BYTE j,row,col;
	BYTE tempQT[LEN64];
	//int  i;
	// Load quantization coefficients from JPG file, scale them for DCT and reorder
	// from zig-zag order
	switch (advance_selector) {
	case 0:
		std_luminance_qt = Tbl_000Y;
		break;
	case 1:
		std_luminance_qt = Tbl_014Y;
		break;
	case 2:
		std_luminance_qt = Tbl_029Y;
		break;
	case 3:
		std_luminance_qt = Tbl_043Y;
		break;
	case 4:
		std_luminance_qt = Tbl_057Y;
		break;
	case 5:
		std_luminance_qt = Tbl_071Y;
		break;
	case 6:
		std_luminance_qt = Tbl_086Y;
		break;
	case 7:
		std_luminance_qt = Tbl_100Y;
		break;
	default:
		printf("\nload_quant_table : Unknown selector : %d\n", advance_selector);
	}
	//  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
	set_quant_table(std_luminance_qt, (BYTE)ADVANCESCALEFACTOR, tempQT);

	for (j=0;j<=63;j++) quant_table[j]=tempQT[zigzag[j]];
	j=0;
	for (row=0;row<=7;row++)
		for (col=0;col<=7;col++) {
			quant_table[j]=(long)(quant_table[j] * (scalefactor[row]*scalefactor[col])) * 65536;
			j++;
		}
	byte_pos+=LEN64;
}

/***************************************************************************
Function		: load_advance_quant_tableCb
Purpose			: Load quantization coefficients from JPG file, scale them for DCT and reorder
				  based on selector assigned from  VideoEngineInfo->FrameHeader.JPEGTableSelector;	
				  Filling 64 bytes by usning scalefactor into fourth quantization table
				  //  Note: Added for Dual_JPEG
Return Type		: None
Return Value	: None
Arguments		: Byte pointer to Quantization table QT[3]
 ****************************************************************************/
void load_advance_quant_tableCb(long *quant_table)
{
	float scalefactor[8]={1.0f, 1.387039845f, 1.306562965f, 1.175875602f,
			1.0f, 0.785694958f, 0.541196100f, 0.275899379f};
	BYTE j,row,col;
	BYTE tempQT[LEN64];
	//int  i;
	// Load quantization coefficients from JPG file, scale them for DCT and reorder
	// from zig-zag order
	if (Mapping == 1) {
		switch (advance_selector) {
		case 0:
			std_chrominance_qt = Tbl_000Y;
			break;
		case 1:
			std_chrominance_qt = Tbl_014Y;
			break;
		case 2:
			std_chrominance_qt = Tbl_029Y;
			break;
		case 3:
			std_chrominance_qt = Tbl_043Y;
			break;
		case 4:
			std_chrominance_qt = Tbl_057Y;
			break;
		case 5:
			std_chrominance_qt = Tbl_071Y;
			break;
		case 6:
			std_chrominance_qt = Tbl_086Y;
			break;
		case 7:
			std_chrominance_qt = Tbl_100Y;
			break;
		default:
			printf("\nload_quant_table : Unknown selector : %d\n", advance_selector);
		}
	}
	else {
		switch (advance_selector) {
		case 0:
			std_chrominance_qt = Tbl_000UV;
			break;
		case 1:
			std_chrominance_qt = Tbl_014UV;
			break;
		case 2:
			std_chrominance_qt = Tbl_029UV;
			break;
		case 3:
			std_chrominance_qt = Tbl_043UV;
			break;
		case 4:
			std_chrominance_qt = Tbl_057UV;
			break;
		case 5:
			std_chrominance_qt = Tbl_071UV;
			break;
		case 6:
			std_chrominance_qt = Tbl_086UV;
			break;
		case 7:
			std_chrominance_qt = Tbl_100UV;
			break;
		default:
			printf("\nload_quant_table : Unknown selector : %d\n", advance_selector);
		}
	}
	//  Note: pass ADVANCE SCALE FACTOR to sub-function in Dual-JPEG
	set_quant_table(std_chrominance_qt, (BYTE)ADVANCESCALEFACTORUV, tempQT);

	for (j=0;j<=63;j++) quant_table[j]=tempQT[zigzag[j]];
	j=0;
	for (row=0;row<=7;row++)
		for (col=0;col<=7;col++) {
			quant_table[j]=(long)(quant_table[j] * (scalefactor[row]*scalefactor[col])) * 65536;
			j++;
		}
	byte_pos+=LEN64;
}

/***************************************************************************
Function		: load_Huffman_table
Purpose			: Updating HT values from nrcode & value
Return Type		: None
Return Value	: None
Arguments		: Huffman_table(HT)  pointer to HTDC[0..3],Byte pointer (nrcode) to std_dc_luminance_nrcodes, Byte pointer(value) to std_dc_luminance_values
 ****************************************************************************/
void load_Huffman_table(Huffman_table *HT, BYTE *nrcode, BYTE *value, WORD *Huff_code)
{
	BYTE k,j, i;
	DWORD code, code_index;

	for (j=1;j<=16;j++) {
		HT->Length[j]=nrcode[j];
	}
	for (i=0, k=1;k<=16;k++)
		for (j=0;j<HT->Length[k];j++) {
			HT->V[WORD_hi_lo(k,j)]=value[i];
			i++;
		}

	code=0;
	for (k=1;k<=16;k++) {
		HT->minor_code[k] = (WORD)code;
		for (j=1;j<=HT->Length[k];j++) code++;
		HT->major_code[k]=(WORD)(code-1);
		code*=2;
		if (HT->Length[k]==0) {
			HT->minor_code[k]=0xFFFF;
			HT->major_code[k]=0;
		}
	}

	HT->Len[0] = 2;  i = 2;

	for (code_index = 1; code_index < 65535; code_index++) {
		if (code_index < Huff_code[i]) {
			HT->Len[code_index] = (unsigned char)Huff_code[i + 1];
		}
		else {
			i = i + 2;
			HT->Len[code_index] = (unsigned char)Huff_code[i + 1];
		}
	}

}

void process_Huffman_data_unit(BYTE DC_nr, BYTE AC_nr,short *previous_DC, WORD position)
{
	// Process one data unit. A data unit = 64 DCT coefficients
	// Data is decompressed by Huffman decoding, then the array is dezigzag-ed
	// The result is a 64 DCT coefficients array: DCT_coeff
	BYTE nr,k;
	register WORD tmp_Hcode;
	BYTE size_val,count_0;
	WORD *min_code;
	BYTE *huff_values;
	BYTE byte_temp;

	// Start Huffman decoding
	// First the DC coefficient decoding
	min_code=HTDC[DC_nr].minor_code;
	//   maj_code=HTDC[DC_nr].major_code;
	huff_values=HTDC[DC_nr].V;


	nr=0;// DC coefficient
	k = HTDC[DC_nr].Len[(WORD)(codebuf>>16)];
	//river
	//	 tmp_Hcode=lookKbits(k);
	tmp_Hcode = (WORD)(codebuf>>(32-k));
	skipKbits(k);
	size_val=huff_values[WORD_hi_lo(k,(BYTE)(tmp_Hcode-min_code[k]))];
	if (size_val==0) DCT_coeff[position + 0]=*previous_DC;
	else {
		DCT_coeff[position + 0]=*previous_DC+getKbits(size_val);
		*previous_DC=DCT_coeff[position + 0];
	}

	// Second, AC coefficient decoding
	min_code=HTAC[AC_nr].minor_code;
	//   maj_code=HTAC[AC_nr].major_code;
	huff_values=HTAC[AC_nr].V;

	nr=1; // AC coefficient
	do {
		k = HTAC[AC_nr].Len[(WORD)(codebuf>>16)];
		tmp_Hcode = (WORD)(codebuf>>(32-k));
		skipKbits(k);

		byte_temp=huff_values[WORD_hi_lo(k,(BYTE)(tmp_Hcode-min_code[k]))];
		size_val=byte_temp&0xF;
		count_0=byte_temp>>4;
		if (size_val==0) {
			if (count_0 != 0xF) {
				break;
			}
			nr+=16;
		}
		else {
			nr+=count_0; //skip count_0 zeroes
			DCT_coeff[position + dezigzag[nr++]]=getKbits(size_val);
		}
	} while (nr < 64);
}

/***************************************************************************
Function		: precalculate_Cr_Cb_tables (called from init_jpg_table )
Purpose			: initializing Cr_tab,Cb_tab
				  Initializing Cr_Cb_green_tab from Cr_tab,Cb_tab (256 * 256)
Return Type		: None
Return Value	: None
Arguments		: None
 ****************************************************************************/
void precalculate_Cr_Cb_tables()
{
	WORD k;
	WORD Cr_v,Cb_v;

	for(k=0;k<=255;k++)
		Cr_tab[k]=(SWORD)((k-128.0)*1.402);

	for(k=0;k<=255;k++)
		Cb_tab[k]=(SWORD)((k-128.0)*1.772);

	for (Cr_v=0;Cr_v<=255;Cr_v++)
		for (Cb_v=0;Cb_v<=255;Cb_v++)
			Cr_Cb_green_tab[((WORD)(Cr_v)<<8)+Cb_v]=(int)(-0.34414*(Cb_v-128.0)-0.71414*(Cr_v-128.0));

}

/***************************************************************************
Function		: InitColorTable
Purpose			: Initializing all color tables m_CrToR,m_CbToB,m_CrToG,m_CbToG
Return Type		: None
Return Value	: None
Arguments		: None
 ****************************************************************************/
void InitColorTable( void )
{
	int i, x;
	int nScale	= 1L << 16;		//equal to power(2,16)
	int nHalf	= nScale >> 1;
	//        double          temp, temp1;

#define FIX_G(x) ((int) ((x) * nScale + 0.5))

	/* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
	/* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
	/* Cr=>R value is nearest int to 1.597656 * x */
	/* Cb=>B value is nearest int to 2.015625 * x */
	/* Cr=>G value is scaled-up -0.8125 * x */
	/* Cb=>G value is scaled-up -0.390625 * x */
	for (i = 0, x = -128; i < LEN256; i++, x++)
	{
		m_CrToR[i] = (int) ( FIX_G(1.597656) * x + nHalf ) >> 16;
		m_CbToB[i] = (int) ( FIX_G(2.015625) * x + nHalf ) >> 16;
		m_CrToG[i] = (int) (- FIX_G(0.8125) * x  + nHalf ) >> 16;
		m_CbToG[i] = (int) (- FIX_G(0.390625) * x + nHalf) >> 16;
	}
	for (i = 0, x = -16; i < LEN256; i++, x++)
	{
		m_Y[i] = (int) ( FIX_G(1.164) * x + nHalf ) >> 16;
	}

	//#define USER_ENABLED_GAMMA
#ifdef USER_ENABLED_GAMMA
#error should not be here!

#define GAMMA1_GAMMA2_SEPERATE  (128.0)
#define GAMMA1PARAMETER     (1.0)
#define GAMMA2PARAMETER     (1.0)
	//For Color Text Enchance Y Re-map
	for (i = 0; i < (GAMMA1_GAMMA2_SEPERATE); i++) {
		temp = (double)i / GAMMA1_GAMMA2_SEPERATE;
		temp1 = 1.0 / GAMMA1PARAMETER;
		m_Y[i] = (BYTE)(GAMMA1_GAMMA2_SEPERATE * pow (temp, temp1));
		if (m_Y[i] > 255) m_Y[i] = 255;
	}
	for (i = (GAMMA1_GAMMA2_SEPERATE); i < LEN256; i++) {
		m_Y[i] = (BYTE)((GAMMA1_GAMMA2_SEPERATE) + (LEN256 - GAMMA1_GAMMA2_SEPERATE) * ( pow((double)((i - GAMMA1_GAMMA2_SEPERATE) / (LEN256 - (GAMMA1_GAMMA2_SEPERATE))), (1.0 / GAMMA2PARAMETER)) ));
		if (m_Y[i] > 255) m_Y[i] = 255;
	}
#endif  /* USER_ENABLED_GAMMA */
}

void init_jpg_table ()
{
	init_QT();
	InitColorTable ();
	prepare_range_limit_table();
	load_Huffman_table(&HTDC[0], std_dc_luminance_nrcodes, std_dc_luminance_values, DC_LUMINANCE_HUFFMANCODE);
	load_Huffman_table(&HTAC[0], std_ac_luminance_nrcodes, std_ac_luminance_values, AC_LUMINANCE_HUFFMANCODE);
	load_Huffman_table(&HTDC[1], std_dc_chrominance_nrcodes, std_dc_chrominance_values, DC_CHROMINANCE_HUFFMANCODE);
	load_Huffman_table(&HTAC[1], std_ac_chrominance_nrcodes, std_ac_chrominance_values, AC_CHROMINANCE_HUFFMANCODE);
}

/***************************************************************************
Function		: init_JPG_decoding (called from decode)
Purpose			: Initializing all quantization table QT[0..3]
Return Type		: int
Return Value	: return value 1 
Arguments		: None
 ****************************************************************************/
int init_JPG_decoding()
{
	byte_pos=0;
	load_quant_table(QT[0]);
	load_quant_tableCb(QT[1]);
	//  Note: Added for Dual-JPEG
	load_advance_quant_table(QT[2]);
	load_advance_quant_tableCb(QT[3]);
	return 1;
}

/***************************************************************************
Function		: IDCT_transform
Purpose			: 
Return Type		: None
Return Value	: None
Arguments		: short pointer to DCT_coeff (coef), 
				  BYTE pointer to ptr (data) points OutBuffer (RGB structure pointer0, 
				  BYTE value of QT_TableSelection (nBlock) select Quantization table
 ****************************************************************************/
void IDCT_transform(short *coef, BYTE *data, BYTE nBlock)
{

#define FIX_1_082392200  ((int)277)		// FIX(1.082392200) 
#define FIX_1_414213562  ((int)362)		// FIX(1.414213562) 
#define FIX_1_847759065  ((int)473)		// FIX(1.847759065) 
#define FIX_2_613125930  ((int)669)		// FIX(2.613125930) 

#define MULTIPLY(var,cons)  ((int) ((var)*(cons))>>8 )

	int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	int tmp10, tmp11, tmp12, tmp13;
	int z5, z10, z11, z12, z13;
	int workspace[LEN64];		// buffers data between passes 

	short* inptr = coef;
	long* quantptr;

	int* wsptr = workspace;
	unsigned char* outptr;
	unsigned char* r_limit = rlimit_table+128;
	int ctr, dcval, DCTSIZE = 8;

	quantptr = (long *)QT[nBlock];

	//Pass 1: process columns from input (inptr), store into work array(wsptr)

	for (ctr = 8; ctr > 0; ctr--) {
		// Due to quantization, we will usually find that many of the input
		// coefficients are zero, especially the AC terms.  We can exploit this
		// by short-circuiting the IDCT calculation for any column in which all
		// the AC terms are zero.  In that case each output is equal to the
		// DC coefficient (with scale factor as needed).
		// With typical images and quantization tables, half or more of the
		// column DCT calculations can be simplified this way.


		if ((inptr[DCTSIZE*1] | inptr[DCTSIZE*2] | inptr[DCTSIZE*3] |
				inptr[DCTSIZE*4] | inptr[DCTSIZE*5] | inptr[DCTSIZE*6] |
				inptr[DCTSIZE*7]) == 0)
		{
			// AC terms all zero 
			dcval = (int)(( inptr[DCTSIZE*0] * quantptr[DCTSIZE*0] ) >> 16);

			wsptr[DCTSIZE*0] = dcval;
			wsptr[DCTSIZE*1] = dcval;
			wsptr[DCTSIZE*2] = dcval;
			wsptr[DCTSIZE*3] = dcval;
			wsptr[DCTSIZE*4] = dcval;
			wsptr[DCTSIZE*5] = dcval;
			wsptr[DCTSIZE*6] = dcval;
			wsptr[DCTSIZE*7] = dcval;

			inptr++;			// advance pointers to next column 
			quantptr++;
			wsptr++;
			continue;
		}

		// Even part 

		tmp0 = (int)(inptr[DCTSIZE*0] * quantptr[DCTSIZE*0]) >> 16;
		tmp1 = (int)(inptr[DCTSIZE*2] * quantptr[DCTSIZE*2]) >> 16;
		tmp2 = (int)(inptr[DCTSIZE*4] * quantptr[DCTSIZE*4]) >> 16;
		tmp3 = (int)(inptr[DCTSIZE*6] * quantptr[DCTSIZE*6]) >> 16;

		tmp10 = tmp0 + tmp2;	// phase 3 
		tmp11 = tmp0 - tmp2;

		tmp13 = tmp1 + tmp3;	// phases 5-3 
		tmp12 = MULTIPLY(tmp1 - tmp3, FIX_1_414213562) - tmp13; // 2*c4 

		tmp0 = tmp10 + tmp13;	// phase 2 
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		// Odd part 

		tmp4 = (int)(inptr[DCTSIZE*1] * quantptr[DCTSIZE*1]) >> 16;
		tmp5 = (int)(inptr[DCTSIZE*3] * quantptr[DCTSIZE*3]) >> 16;
		tmp6 = (int)(inptr[DCTSIZE*5] * quantptr[DCTSIZE*5]) >> 16;
		tmp7 = (int)(inptr[DCTSIZE*7] * quantptr[DCTSIZE*7]) >> 16;

		z13 = tmp6 + tmp5;		// phase 6 
		z10 = tmp6 - tmp5;
		z11 = tmp4 + tmp7;
		z12 = tmp4 - tmp7;

		tmp7  = z11 + z13;		// phase 5 
		tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); // 2*c4 

		z5	  = MULTIPLY(z10 + z12, FIX_1_847759065); // 2*c2 
		tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5; // 2*(c2-c6) 
		tmp12 = MULTIPLY(z10, - FIX_2_613125930) + z5; // -2*(c2+c6) 

		tmp6 = tmp12 - tmp7;	// phase 2 
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		wsptr[DCTSIZE*0] = (int) (tmp0 + tmp7);
		wsptr[DCTSIZE*7] = (int) (tmp0 - tmp7);
		wsptr[DCTSIZE*1] = (int) (tmp1 + tmp6);
		wsptr[DCTSIZE*6] = (int) (tmp1 - tmp6);
		wsptr[DCTSIZE*2] = (int) (tmp2 + tmp5);
		wsptr[DCTSIZE*5] = (int) (tmp2 - tmp5);
		wsptr[DCTSIZE*4] = (int) (tmp3 + tmp4);
		wsptr[DCTSIZE*3] = (int) (tmp3 - tmp4);

		inptr++;			// advance pointers to next column 
		quantptr++;
		wsptr++;
	}

	// Pass 2: process rows from work array, store into output array. 
	// Note that we must descale the results by a factor of 8 == 2**3, 
	// and also undo the PASS1_BITS scaling. 

	//#define RANGE_MASK 1023; //2 bits wider than legal samples
#define PASS1_BITS  0
#define IDESCALE(x,n)  ((int) ((x)>>n) )

	wsptr = workspace;
	for (ctr = 0; ctr < DCTSIZE; ctr++) {
		outptr = data + ctr * 8;

		// Rows of zeroes can be exploited in the same way as we did with columns.
		// However, the column calculation has created many nonzero AC terms, so
		// the simplification applies less often (typically 5% to 10% of the time).
		// On machines with very fast multiplication, it's possible that the
		// test takes more time than it's worth.  In that case this section
		// may be commented out.
		//
		// Even part 

		tmp10 = ((int) wsptr[0] + (int) wsptr[4]);
		tmp11 = ((int) wsptr[0] - (int) wsptr[4]);

		tmp13 = ((int) wsptr[2] + (int) wsptr[6]);
		tmp12 = MULTIPLY((int) wsptr[2] - (int) wsptr[6], FIX_1_414213562)
					- tmp13;

		tmp0 = tmp10 + tmp13;
		tmp3 = tmp10 - tmp13;
		tmp1 = tmp11 + tmp12;
		tmp2 = tmp11 - tmp12;

		// Odd part 

		z13 = (int) wsptr[5] + (int) wsptr[3];
		z10 = (int) wsptr[5] - (int) wsptr[3];
		z11 = (int) wsptr[1] + (int) wsptr[7];
		z12 = (int) wsptr[1] - (int) wsptr[7];

		tmp7 = z11 + z13;		// phase 5 
		tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562); // 2*c4 

		z5    = MULTIPLY(z10 + z12, FIX_1_847759065); // 2*c2 
		tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5; // 2*(c2-c6) 
		tmp12 = MULTIPLY(z10, - FIX_2_613125930) + z5; // -2*(c2+c6) 

		tmp6 = tmp12 - tmp7;	// phase 2 
		tmp5 = tmp11 - tmp6;
		tmp4 = tmp10 + tmp5;

		// Final output stage: scale down by a factor of 8 and range-limit 

		outptr[0] = r_limit[IDESCALE((tmp0 + tmp7), (PASS1_BITS+3))
		                    & 1023L];
		outptr[7] = r_limit[IDESCALE((tmp0 - tmp7), (PASS1_BITS+3))
		                    & 1023L];
		outptr[1] = r_limit[IDESCALE((tmp1 + tmp6), (PASS1_BITS+3))
		                    & 1023L];
		outptr[6] = r_limit[IDESCALE((tmp1 - tmp6), (PASS1_BITS+3))
		                    & 1023L];
		outptr[2] = r_limit[IDESCALE((tmp2 + tmp5), (PASS1_BITS+3))
		                    & 1023L];
		outptr[5] = r_limit[IDESCALE((tmp2 - tmp5), (PASS1_BITS+3))
		                    & 1023L];
		outptr[4] = r_limit[IDESCALE((tmp3 + tmp4), (PASS1_BITS+3))
		                    & 1023L];
		outptr[3] = r_limit[IDESCALE((tmp3 - tmp4), (PASS1_BITS+3))
		                    & 1023L];

		wsptr += DCTSIZE;		// advance pointer to next row 
	}
}

/***************************************************************************
Function		: updatereadbuf
Return Type		: None
Return Value	: None
Arguments		: integer value to walks (0x2)
****************************************************************************/
void updatereadbuf(int walks)
{
	unsigned long readbuf;
	//char readstring[256];

	if ((newbits-walks)<=0) {
		readbuf = Buffer[_index];
		_index++;
		codebuf=(codebuf<<walks)|((newbuf|(readbuf>>(newbits)))>>(32-walks));
		newbuf=readbuf<<(walks-newbits);
		newbits=32 + newbits-walks;
	} else {
		codebuf=(codebuf<<walks)|(newbuf>>(32-walks));
		newbuf= (newbuf<<walks);
		newbits-=walks;
	}
}

/***************************************************************************
Function		: YUVToRGB
Purpose			: it is generating output RGB buffer
Return Type		: None
Return Value	: None
Arguments		: Integer value of txb to txb
				  Integer value of tyb to tyb
				  unsigned char pointer to byTileYuv (pYCBCr)
				  unsigned char pointer to outBuf (pBgr) - pBgr is pointer by RGB structure.
 ****************************************************************************/
void YUVToRGB  (
		int txb, int tyb,
		unsigned char * pYCbCr, //in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
		unsigned char * pBgr    //out, BGR fofermat, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
)
{
	int i, j, pos, m, n;
	unsigned char cb, cr, *py, *pcb, *pcr, *py420[4];
	int y;
	int nBlocksInMcu = 6;
	unsigned int pixel_x, pixel_y;
	int jMax;

	if (Mode420 == 0) {//YUV 444
		py = pYCbCr;
		pcb	  = pYCbCr + 64;
		pcr   = pcb + 64;

		pixel_x = txb*8;
		pixel_y = tyb*8;
		pos = (pixel_y*real_WIDTH)+pixel_x;


		for(j = 0; j < 8; j++) {
			for(i = 0; i < 8; i++) {
				m = ( (j << 3 ) + i);
				y = py[m];
				cb = pcb[m];
				cr = pcr[m];
				n = (pos + i) * 3;

				prev_yuv[n] = y;
				prev_yuv[n+1] = cb;
				prev_yuv[n+2] = cr;

				if(n < (long)(real_WIDTH * real_HEIGHT * 3))
				{
					pBgr [n] = rlimit_table[ m_Y[y] + m_CbToB[cb] ];
					pBgr [n + 1] = rlimit_table[ m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
					pBgr [n + 2] = rlimit_table[ m_Y[y] + m_CrToR[cr] ];
				}

			}
			pos += real_WIDTH;
		}
	}
	else {//YUV 420
		for( i = 0; i < nBlocksInMcu-2; i++ )
			py420[i] = pYCbCr + i * LEN64;

		pcb   = pYCbCr + (nBlocksInMcu-2) * LEN64;
		pcr   = pcb + LEN64;

		//  Get tile starting pixel position
		pixel_x = txb*16;
		pixel_y = tyb*16;
		pos = (pixel_y*real_WIDTH)+pixel_x;

		// For 800x600 support
		jMax = 16;
		if ((HEIGHT == 608) && (tyb == 37))
			jMax = 8;

		for( j=0; j < jMax; j++ )//vertical axis
		{
			for( i=0; i<16; i++ )   //horizontal axis
			{
				//  block number is ((j/8) * 2 + i/8)={0, 1, 2, 3}
				y = *( py420[(j>>3) * 2 + (i>>3)] ++ );
				m = ( (j>>1) << 3 ) + (i>>1);
				cb = pcb[m];
				cr = pcr[m];

				n = (pos + i) * 3;
				if(n < (long)(real_WIDTH * real_HEIGHT * 3))
				{
					pBgr [n] = rlimit_table[ m_Y[y] + m_CbToB[cb] ];
					pBgr [n + 1] = rlimit_table[ m_Y[y] + m_CbToG[cb] + m_CrToG[cr] ];
					pBgr [n + 2] = rlimit_table[ m_Y[y] + m_CrToR[cr] ];
				}

			}
			pos += real_WIDTH;
		}
	}
}

/***************************************************************************
Function		: YUVToRGB
Purpose			: it is generating output RGB buffer
Return Type		: None
Return Value	: None
Arguments		: Integer value of txb to txb
				  Integer value of tyb to tyb
				  unsigned char pointer to byTileYuv (pYCBCr)
				  unsigned char pointer to outBuf (pBgr) - pBgr is pointer by RGB structure.
 ****************************************************************************/
void YUVToRGB_Pass2  (
		int txb, int tyb,
		unsigned char * pYCbCr, //in, Y: 256 or 64 bytes; Cb: 64 bytes; Cr: 64 bytes
		unsigned char * pBgr    //out, BGR fofermat, 16*16*3 = 768 bytes; or 8*8*3=192 bytes
)
{
	int i, j, pos, m, n;
	unsigned char cb, cr, *py, *pcb, *pcr;
	int y;
	unsigned int pixel_x, pixel_y;

	if (Mode420 == 0) {//YUV 444
		py = pYCbCr;
		pcb	  = pYCbCr + 64;
		pcr   = pcb + 64;

		pixel_x = txb*8;
		pixel_y = tyb*8;
		pos = (pixel_y*real_WIDTH)+pixel_x;


		for(j = 0; j < 8; j++) {
			for(i = 0; i < 8; i++) {
				m = ( (j << 3 ) + i);
				n = (pos + i) * 3;

				y = prev_yuv[n] + (py[m] - 128);
				cb = prev_yuv[n + 1] + (pcb[m] - 128);
				cr = prev_yuv[n + 2] + (pcr[m] - 128);

				/* aovid out of range */
				if (y < 0)
					y = 0;
				if (y > 255)
					y = 255;
				if(n < (long)(real_WIDTH * real_HEIGHT * 3))
				{
					pBgr [n] = rlimit_table[ m_Y[y] + m_CbToB[cb] ];
					pBgr [n + 1] = rlimit_table[ m_Y[y] + m_CbToG[cb] + m_CrToG[cr]];
					pBgr [n + 2] = rlimit_table[ m_Y[y] + m_CrToR[cr] ];
				}

			}
			pos += real_WIDTH;
		}
	}
}

/***************************************************************************
Function		: Decompress 
Purpose			: Decompressing the outBuffer using Quatization table
Return Type		: None
Return Value	: None
Arguments		: Integer value of txb to txb
				  Integer value of tyb to tyb
				  char pointer to outBuffer
				  BYTE value of quatization table index to select quatization table
 ****************************************************************************/
int Decompress (int txb, int tyb, unsigned char *outBuf, BYTE QT_TableSelection)
{
	unsigned char    *ptr;
	unsigned char    byTileYuv[768];

	memset (DCT_coeff, 0, 384 * 2);
	ptr = byTileYuv;

	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY, 0);
	IDCT_transform(DCT_coeff, ptr, QT_TableSelection);
	ptr += LEN64;

	if (Mode420 == 1) {
		process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY, 64);
		IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection);
		ptr += 64;

		process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY, 128);
		IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection);
		ptr += 64;

		process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY, 192);
		IDCT_transform(DCT_coeff + 192, ptr, QT_TableSelection);
		ptr += 64;

		process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb, 256);
		IDCT_transform(DCT_coeff + 256, ptr, QT_TableSelection + 1);
		ptr += 64;

		process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr, 320);
		IDCT_transform(DCT_coeff + 320, ptr, QT_TableSelection + 1);
	}
	else {
		process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb, 64);
		IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection + 1);
		ptr += 64;

		process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr, 128);
		IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection + 1);
	}

	YUVToRGB (txb, tyb, byTileYuv, outBuf);

	return  TRUE;
}

/***************************************************************************
Function		: Decompress 
Purpose			: Decompressing the outBuffer using Quatization table
Return Type		: None
Return Value	: None
Arguments		: Integer value of txb to txb
				  Integer value of tyb to tyb
				  char pointer to outBuffer
				  BYTE value of quatization table index to select quatization table
 ****************************************************************************/
int Decompress_Paas2 (int txb, int tyb, unsigned char *outBuf, BYTE QT_TableSelection)
{
	unsigned char    *ptr;
	unsigned char    byTileYuv[768];

	memset (DCT_coeff, 0, 384 * 2);
	ptr = byTileYuv;

	process_Huffman_data_unit(YDC_nr,YAC_nr,&DCY, 0);
	IDCT_transform(DCT_coeff, ptr, QT_TableSelection);
	ptr += LEN64;

	process_Huffman_data_unit(CbDC_nr,CbAC_nr,&DCCb, 64);
	IDCT_transform(DCT_coeff + 64, ptr, QT_TableSelection + 1);
	ptr += 64;

	process_Huffman_data_unit(CrDC_nr,CrAC_nr,&DCCr, 128);
	IDCT_transform(DCT_coeff + 128, ptr, QT_TableSelection + 1);

	YUVToRGB_Pass2 (txb, tyb, byTileYuv, outBuf);

	return  TRUE;
}


int VQ_Decompress (int txb, int tyb, unsigned char *outBuf, BYTE QT_TableSelection, struct COLOR_CACHE *VQ)
{
	unsigned char    *ptr, i;
	unsigned char    byTileYuv[192];
	int    Data;

	QT_TableSelection = 0;
	Data =  QT_TableSelection;

	ptr = byTileYuv;
	if (VQ->BitMapBits == 0) {
		for (i = 0; i < 64; i++) {
			ptr[0] = (unsigned char)((VQ->Color[VQ->Index[0]] & 0xFF0000) >> 16);
			ptr[64] = (unsigned char)((VQ->Color[VQ->Index[0]] & 0x00FF00) >> 8);
			ptr[128] = (unsigned char)(VQ->Color[VQ->Index[0]] & 0x0000FF);
			ptr += 1;
		}
	}
	else {
		for (i = 0; i < 64; i++) {
			Data = (int)lookKbits(VQ->BitMapBits);
			ptr[0] = (unsigned char)((VQ->Color[VQ->Index[Data]] & 0xFF0000) >> 16);
			ptr[64] = (unsigned char)((VQ->Color[VQ->Index[Data]] & 0x00FF00) >> 8);
			ptr[128] = (unsigned char)(VQ->Color[VQ->Index[Data]] & 0x0000FF);
			ptr += 1;
			skipKbits(VQ->BitMapBits);
		}
	}
	YUVToRGB (txb, tyb, byTileYuv, (unsigned char *)outBuf);

	return  TRUE;
}

/***************************************************************************
Function		: MoveBlockIndex 
Purpose			: Just increasing pixels by 256 to access the next 256 index
Return Type		: None
Return Value	: None
Arguments		: Mode420 - 1 if YUV 420 mode is set, 0 otherwise.
****************************************************************************/
void MoveBlockIndex (short Mode420)
{
	if (Mode420 == 0) {//YUV 444
		txb++;
		if (txb >= (int)(tmp_WIDTH/8)) {
			tyb++;
			if (tyb >= (int)(tmp_HEIGHT/8))
				tyb = 0;
			txb = 0;
		}
	}
	else {//YUV 420
		txb++;
		if (txb >= (int)(tmp_WIDTH/16)) {
			tyb++;
			if (tyb >= (int)(tmp_HEIGHT/16))
				tyb = 0;
			txb = 0;
		}
	}

	pixels += LEN256;

	/* For 800x600 support */
	iMax = LEN256;
	if ((HEIGHT == 608) && (oldyb == 37))
		iMax = 128;

}

void VQ_Initialize (struct COLOR_CACHE *VQ)
{
	int i;

	for (i = 0; i < 4; i++) {
		VQ->Index[i] = i;
	}
	VQ->Color[0] = 0x008080;
	VQ->Color[1] = 0xFF8080;
	VQ->Color[2] = 0x808080;
	VQ->Color[3] = 0xC08080;
}

/**
 * Thid function decodes the video buffer data from video drive and gets the tile change information
 * required by the VNC server.
 * Return Type		: None
 * Return Value		: None
 * Arguments		: tile_info - The tile info buffer to be updated with tile count and x,y coodinate values. - out
 * 					  video_frame_hdr - video frame header buffer - in
 * 					  video_buffer video data buffer.
 * 
 */
void decode_tile_coordinates(unsigned char **tile_info, frame_hdr_t *video_frame_hdr, void *video_buffer){
	unsigned long CompressBufferSize;
	unsigned char *mybuff = NULL;
	struct COLOR_CACHE    Decode_Color;

	int	i;
	int pos = sizeof(unsigned short);
	unsigned char *tmp_tile_info;

	VQ_Initialize (&Decode_Color);

	WIDTH = video_frame_hdr->SourceMode_X;
	HEIGHT = video_frame_hdr->SourceMode_Y;
	if(WIDTH != real_WIDTH || HEIGHT != real_HEIGHT){
		real_WIDTH = video_frame_hdr->SourceMode_X;
		real_HEIGHT = video_frame_hdr->SourceMode_Y;

		/*Allocate memory for the tile_info buffer.
		* From the host video, (width * height)/(tile_width + tile_height) gives
		* the maximum number of tile changes. Each tile x, tile y change will be
		* stored as an unsigned char(1 byte). Total number of tile changes will
		* be stored as unsigned short(2 bytes).
		*/
		if(*tile_info == NULL)
		{
			*tile_info = malloc((((real_WIDTH * real_HEIGHT)/(TILE_WIDTH + TILE_HEIGHT)) * sizeof(unsigned char))+sizeof(unsigned short));
		}
		//Update the reallocated memory for tile_info buffer, when the resolution changes.
		else{
			free(*tile_info);
			*tile_info = malloc((((real_WIDTH * real_HEIGHT)/(TILE_WIDTH + TILE_HEIGHT)) * sizeof(unsigned char))+sizeof(unsigned short));
		}
	}

	Mode420 = video_frame_hdr->FrameHdr_Mode420;
	//  AST2100 JPEG block is 16x16(pixels) base
	if (Mode420 == 1) {
		if (WIDTH % 16) {
			WIDTH = WIDTH + 16 - (WIDTH % 16);
		}
		if (HEIGHT % 16) {
			HEIGHT = HEIGHT + 16 - (HEIGHT % 16);
		}
	}
	else {
		if (WIDTH % 8) {
			WIDTH = WIDTH + 8 - (WIDTH % 8);
		}
		if (HEIGHT % 8) {
			HEIGHT = HEIGHT + 8 - (HEIGHT % 8);
		}
	}

	tmp_WIDTH = video_frame_hdr->DestinationMode_X;
	tmp_HEIGHT = video_frame_hdr->DestinationMode_Y;
	CompressBufferSize = (video_frame_hdr->CompressData_CompressSize/4);

	Buffer = (unsigned long *)(video_buffer);

	//  RC4 decoding. Keys_Expansion, RC4_setup and RC4_crypt please reference video.c
	if (video_frame_hdr->FrameHdr_RC4Enable == 1) {
		if (DecodeRC4State == FALSE) {  // If decodeRC4State == FALSE, it means that it will not do keys reset.
			// It means that it uses previous left x and y to do cryption
			Keys_Expansion (DecodeKeys);
			DecodeRC4_setup (&s, DecodeKeys);
		}
		RC4_crypt (&s, (unsigned char *)Buffer, CompressBufferSize * 4);
	}

	if (video_frame_hdr->FrameHdr_Mode420 == 1) { // YUV 420
		if (tmp_WIDTH % 16 != 0) {
			tmp_WIDTH = tmp_WIDTH + 16 - (tmp_WIDTH % 16);
		}
		if (tmp_HEIGHT % 16 != 0) {
			tmp_HEIGHT = tmp_HEIGHT + 16 - (tmp_HEIGHT % 16);
		}
	}
	else { // YUV 444
		if (tmp_WIDTH % 8 != 0) {
			tmp_WIDTH = tmp_WIDTH + 8 - (tmp_WIDTH % 8);
		}
		if (tmp_HEIGHT % 8 != 0) {
			tmp_HEIGHT = tmp_HEIGHT + 8 - (tmp_HEIGHT % 8);
		}
	}

	yheader.qfactor = video_frame_hdr->FrameHdr_JPEGScaleFactor;
	yheader.height = video_frame_hdr->DestinationMode_Y;
	yheader.width = video_frame_hdr->DestinationMode_X;
	SCALEFACTOR=16;
	SCALEFACTORUV=16;
	ADVANCESCALEFACTOR= 16;
	ADVANCESCALEFACTORUV= 16;
	selector = video_frame_hdr->FrameHdr_JPEGTableSelector;
	advance_selector = video_frame_hdr->FrameHdr_AdvanceTableSelector;
	Mapping = video_frame_hdr->FrameHdr_JPEGYUVTableMapping;
	SharpModeSelection = video_frame_hdr->FrameHdr_SharpModeSelection;

	init_jpg_table();
	init_JPG_decoding();

	tmp_tile_info = *tile_info  + pos;
	Buffer = (unsigned long *)(video_buffer);
	codebuf = Buffer[0];
	newbuf = Buffer[1];
	_index = 2;

	txb = tyb = prev_txb = prev_tyb = tile_count = 0;
	newbits=32;
	DCY = DCCb = DCCr = 0;
	if(mybuff == NULL){
		mybuff = (unsigned char*)malloc(real_WIDTH*real_HEIGHT*3);
	}

	do {
		if (((codebuf>>28)&BLOCK_HEADER_MASK)==JPEG_NO_SKIP_CODE) {
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decompress (txb, tyb, mybuff, 0);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==JPEG_SKIP_CODE) {
			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decompress (txb, tyb, mybuff, 0);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if(((codebuf>>28)&BLOCK_HEADER_MASK) == JPEG_NO_SKIP_PASS2_CODE){
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decompress_Paas2 (txb, tyb, mybuff, 2);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if(((codebuf>>28)&BLOCK_HEADER_MASK) == JPEG_SKIP_PASS2_CODE){

			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decompress_Paas2 (txb, tyb, mybuff, 2);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_NO_SKIP_1_COLOR_CODE) {
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decode_Color.BitMapBits = 0;

			for (i = 0; i < 1; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_SKIP_1_COLOR_CODE) {
			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decode_Color.BitMapBits = 0;

			for (i = 0; i < 1; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_NO_SKIP_2_COLOR_CODE) {
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decode_Color.BitMapBits = 1;

			for (i = 0; i < 2; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_SKIP_2_COLOR_CODE) {
			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decode_Color.BitMapBits = 1;

			for (i = 0; i < 2; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_NO_SKIP_4_COLOR_CODE) {
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decode_Color.BitMapBits = 2;

			for (i = 0; i < 4; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==VQ_SKIP_4_COLOR_CODE) {
			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decode_Color.BitMapBits = 2;

			for (i = 0; i < 4; i++) {
				Decode_Color.Index[i] = (unsigned char)((codebuf>>29)&VQ_INDEX_MASK);
				if (((codebuf>>31)&VQ_HEADER_MASK)==VQ_NO_UPDATE_HEADER) {
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
				else {
					Decode_Color.Color[Decode_Color.Index[i]] = ((codebuf>>5)&VQ_COLOR_MASK);
					updatereadbuf (VQ_UPDATE_LENGTH);
				}
			}
			VQ_Decompress (txb, tyb, mybuff, 0, &Decode_Color);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==LOW_JPEG_NO_SKIP_CODE) {
			updatereadbuf (BLOCK_AST2100_START_LENGTH);
			Decompress (txb, tyb, mybuff, 2);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==LOW_JPEG_SKIP_CODE) {
			txb = (codebuf & 0x0FF00000) >> 20;
			tyb = (codebuf & 0x0FF000) >> 12;

			updatereadbuf (BLOCK_AST2100_SKIP_LENGTH);
			Decompress (txb, tyb, mybuff, 2);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}
		else if (((codebuf>>28)&BLOCK_HEADER_MASK)==FRAME_END_CODE) {
			//Frame end reached. Quit the loop.
			break;
		}
		else{
			updatereadbuf (VQ_NO_UPDATE_LENGTH);
			MoveBlockIndex (video_frame_hdr->FrameHdr_Mode420);
		}

		//Update the tile info buffer with tile x,y values only if there is any change.
		if((prev_txb != txb) || (prev_tyb != tyb))
		{
			//tile y value is added to tile info buffer first.
			memcpy(tmp_tile_info ,&tyb,sizeof(unsigned char));
			tmp_tile_info++;
			pos+= sizeof(unsigned char);
			//tile x value is added to tile info buffer.
			memcpy(tmp_tile_info ,&txb,sizeof(unsigned char));
			pos+= sizeof(unsigned char);
			tmp_tile_info++;

			prev_txb = txb;
			prev_tyb = tyb;
			tile_count++;
			//If there is a tile change in more that 50% of the frame area, return tile count like whole screen has updated.
			//This will save considerable amount of processing time. 
			if(tile_count > (((real_WIDTH * real_HEIGHT)/(TILE_WIDTH + TILE_HEIGHT))*0.5))
			{
				//If we set the tile count more than 1000, the in the vnceserver, it will be treated as a full screen change. 
				tile_count = 1001;
				break;
			}
		}
	} while (_index < CompressBufferSize);
	//tile count value is added to tile info buffer.
	memcpy(*tile_info,&tile_count,sizeof(unsigned short));

	if(mybuff != NULL){
		free(mybuff);
		mybuff = NULL;
	}
}


/***************************************************************************
Function		: RC4_crypt
Purpose			: 
				//  Crypt function for RC4. Algorithm is as following:
				// 
				//  x = (x + 1) mod 256
				//  y = (y + s->m[x]) mod 256
				//  Swap s->m[x] and s->m[y]
				//  t = (s->m[x] + s->m[y]) mod 256
				//  K = s->m[t]
				//  K is the XORed with the plaintext to produce the ciphertext or the
				//  ciphertext to produce the plaintext
				// 
Return Type		: None
Return Value	: None
Arguments		: pointer to rc4_state, unsigned char pointer to data, integer value to length
 ****************************************************************************/
void RC4_crypt( struct rc4_state *s, unsigned char *data, int length )
{ 

	//int i, x, y, a, b, c, *m, temp;
	int i, x, y, a, b, *m;

	x = s->x;
	y = s->y;
	m = s->m;

	for( i = 0; i < length; i++ )
	{
		x = (unsigned char) ( x + 1 ); a = m[x];
		y = (unsigned char) ( y + a );
		m[x] = b = m[y];
		m[y] = a;
		data[i] ^= m[(unsigned char) ( a + b )];
	}

	s->x = x;
	s->y = y;
}

/***************************************************************************
Function		: DecodeRC4_setup
Purpose			: Setup function for RC4. The same routine with encode
Return Type		: None
Return Value	: None
Arguments		: pointer to rc4_state, unsigned char pointer to key, pointer to PVIDEO_ENGINE_INFO
 ****************************************************************************/
void DecodeRC4_setup( struct rc4_state *s, unsigned char *key)
{
	int i, j, k, a, *m;

	s->x = 0;
	s->y = 0;
	m = s->m;

	for( i = 0; i < LEN256; i++ )
	{
		m[i] = i;
	}

	j = k = 0;

	for( i = 0; i < LEN256; i++ )
	{
		a = m[i];
		j = (unsigned char) ( j + a + key[k] );
		m[i] = m[j]; m[j] = a;
		k++;
	}
}

/***************************************************************************
Function		: Keys_Expansion
Purpose			: Key expansion function for RC4. RC4 repeat keys to 256 bytes.
Return Type		: None
Return Value	: None
Arguments		: unsigned char pointer to key
 ****************************************************************************/
void Keys_Expansion (unsigned char *key)
{
	unsigned short    i, StringLength;

	StringLength = strlen ((char *)key);
	for (i = 0; i < LEN256; i++) {
		key[i] = key[i % StringLength];
	}
}
