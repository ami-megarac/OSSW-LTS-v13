/***************************************************************
****************************************************************
**                                                            **
**    (C)Copyright 2009-2015, American Megatrends Inc.        **
**                                                            **
**            All Rights Reserved.                            **
**                                                            **
**        6145-F, Northbelt Parkway, Norcross,                **
**                                                            **
**        Georgia - 30071, USA. Phone-(770)-246-8600.         **
**                                                            **
****************************************************************/

#ifndef AMI_FGE_H
#define AMI_FGE_H


#define GRC_BASE_OFFSET			0x00	//0x800
#define CRTC0_OFFSET			0x60	//0x480
#define CRTCEXT_OFFSET			0x80	//0x4C0
#define CRTC_SEQ_OFFSET			0x20	
#define CRTC_GCTL_OFFSET		0x30
#define ATTR0_OFFSET			0x00	//0x4E0
#define GRC_PALETTE_OFFSET		0x400
#define GRC_REGS_STRUCT_OFFSET	0x400
#define DISP_CNTRL_OFFSET		0x54
#define PCIDID_OFFSET			0x48
#define PCISID_OFFSET			0x4C
#define VGACRC8_CA 				0xC8

#define REG_BIT0	1 << 0 
#define REG_BIT1	1 << 1 
#define REG_BIT2	1 << 2 
#define REG_BIT3	1 << 3 
#define REG_BIT4	1 << 4 
#define REG_BIT5	1 << 5 
#define REG_BIT6	1 << 6 
#define REG_BIT7	1 << 7 
#define REG_BIT8	1 << 8 
#define REG_BIT9	1 << 9 
#define REG_BIT10	1 << 10 
#define TEXTLIMIT	0x400000	// 4Mb

#define PILOT_ENGINE_DEV_NUM 1
#define PILOT_ENGINE_DEV_NAME	"videocap"
#define PILOT_ENGINE_DRV_NAME	"ast_videocap"

#define CRTC_CYCLE_DISABLE	(1 << 16)
#define LCL_DISP_DISABLE	(1 << 17)

#ifdef CONFIG_SPX_FEATURE_LMEM_SSP_RESERVED_SIZE
#define LMEM_SSP_RESERVED_SIZE  0x4000      //16KB  // reserved for SSP Instruction memory and SSP Data Memory 
#else
#define LMEM_SSP_RESERVED_SIZE  0x0
#endif
 
#define FRAMEBUFFER			(0x80000000)	// DDR begin. Refer Memory Space Allocation Table in datasheet
#define FRAMEBUFFER_SIZE	SZ_16M //0x1000000/// 0x01600000	// 16Mb -> wrong this 22MB 

#define PILOT_FG_BASE					0x1E7C8000
#define PILOT_FG_BASE_SIZE 				SZ_4K
#define GRCE_BASE						0x1E6EC000
#define GRCE_BASE_SIZE					SZ_4K
#define BSE_BASE						PILOT_FG_BASE + 0x200 // FG_BASE + 0x200
#define BSE_BASE_SIZE					SZ_256
#define TFE_BASE						PILOT_FG_BASE + 0x100 // + LMEM_SSP_RESERVED_SIZE
#define TFE_BASE_SIZE					SZ_4M //SZ_256 // - LMEM_SSP_RESERVED_SIZE
#define LMEMSTART						0x1E7C0000//0x1000F000	// tfe descriptors must reside here
#define TFE_DEST						0x80800000	//0x08400000	// second half of video buffer
#define TFE_DEST_SIZE					SZ_4M //0x00400000    	// 4MB
#define TFE_DEST_OFFSET					0x00400000
#define SYS_CLK_CNTRL					0x40100100
#define LMEM_SIZE						SZ_8K //0x00002000 //0x00008000
// #define BSELMEMSTART					0x1000C800	// 0x1000C800 to 0x1000CFFF for BSE descriptors
#define BSELMEM_SIZE					0x00000800 // 2K size for BSE descriptors
#define BSELMEM_TEMP_BUFFER_BASE 		0x0000//0xB000
#define DP_BASE							0x1e6eb000
// defines for Pilot-III PCI Device ID & PCI Vendor ID
#define P3_PCI_DEVICE_ID	0x0522
#define P3_PCI_VENDOR_ID	0x102B
#define P3_PCI_HIDE_MASK	0xFFFFFFFF
#define AST_VIDEOCAP_REG_SIZE			SZ_128K
//TOP
#define TOP_REG_OFFSET			(0x0)
#define TOP_REG_CTL				(TOP_REG_OFFSET + 0x00)
#define TOP_REG_STS				(TOP_REG_OFFSET + 0x04)
#define LMEM_BASE_REG_3			(TOP_REG_OFFSET + 0x2c)
#define LMEM_LIMIT_REG_3		(TOP_REG_OFFSET + 0x3c)
#define LMEM11_P0				(TOP_REG_OFFSET + 0x4c)
#define LMEM12_P0				(TOP_REG_OFFSET + 0x48)//doc uses 0x48 // SDK uses 50
#define LMEM12_P1				(TOP_REG_OFFSET + 0x80)
#define LMEM11_P1				(TOP_REG_OFFSET + 0x84)
#define LMEM12_P2				(TOP_REG_OFFSET + 0xA0)
#define LMEM11_P2				(TOP_REG_OFFSET + 0xA4)

// defines for video_mode_flags
#define MGA_MODE	1 << 0		// 1 = mga mode 0 = vga mode
#define TEXT_MODE	1 << 1		// 1 = text mode 0 = graphics mode
#define TWO56_COLOR	1 << 2		// 256 color mode
#define MODE13		1 << 3		// VGA mode 13
#define MODE12		1 << 4		// VGA mode 12
#define NON_MATROX	1 << 5		// Non-Matrox driver case

// defines for capture_status
#define NO_SCREEN_CHANGE	1 << 0
#define MODE_CHANGE		1 << 1
#define UNSUPPORTED_MODE	1 << 2
#define BAD_COLOR_DEPTH		1 << 3
#define NO_VIDEO		1 << 4
#define BAD_COMPRESSION		1 << 6

typedef struct
{
	unsigned long captured_byte_count ;
	unsigned long video_buffer_offset ;
	unsigned long rowmap0 ;
	unsigned long rowmap1 ;
	unsigned long colmap0 ;
	unsigned long colmap1 ;
	unsigned short width ;		// characters in vga mode - pixels in mga mode
	unsigned short height ;		// height in lines. In text mode, divide by character height
	unsigned short hdispend;
	unsigned short vdispend;
	unsigned char depth ;
	unsigned char columns_per_tile ;
	unsigned char rows_per_tile ;
	unsigned char video_mode_flags ;
	unsigned char char_height ;
	unsigned char left_x ;
	unsigned char right_x ;
	unsigned char top_y ;
	unsigned char bottom_y ;
	unsigned char text_snoop_status ;
	unsigned char capture_status ;
	unsigned short tile_cap_mode;
	unsigned short stride;
	unsigned char TextBuffOffset;
	unsigned char act_depth;
	unsigned char bandwidth_mode;
} VIDEO_MODE_PARAMS ;

// FGB Top Registers
typedef struct
{
	unsigned long TOPCTL ;
	unsigned long TOPSTS ;
	unsigned long A2SCTL ;
	unsigned long S2ACTL ;
	unsigned long A2SDBL0 ;
	unsigned long A2SDBL1 ;
	unsigned long S2ADBL0 ;
	unsigned long S2ADBL1 ;
} FGB_TOP_REG_STRUCT ;

// Tile Snoop Engine Registers
typedef struct
{
	unsigned long TSCMD; //0x400
	unsigned long TSSTS; //0x404
	unsigned long TSCSTS0 ; //0x408
	unsigned long TSCSTS1 ; //0x40C
	unsigned long TSRSTS0 ; // 0x410
	unsigned long TSRSTS1 ;//0x414
	unsigned long TSTCSTS ;//0x418
	unsigned long TSFBSA ;//0X41C
	unsigned long TSICR ; //0X420
	unsigned long TSTMUL ;//0X424
} TSE_REG_STRUCT ;

// Tile Fetch Engine Registers
typedef struct
{
	unsigned long TFCTL;
	unsigned long TFSTS;
	unsigned long TFDTB;
	unsigned long TFCHK;
	unsigned long TFRBC;
	unsigned long RLELMT;
} TFE_REG_STRUCT ;

// Bit Slice Engine Registers
typedef struct
{
	unsigned long BSCMD ;
	unsigned long BSSTS ;
	unsigned long BSDTB ;
	unsigned long BSDBS ;
	unsigned long BSBPS0;
	unsigned long BSBPS1;
	unsigned long BSBPS2;
} BSE_REG_STRUCT ;

// Tile Fetch Engine Descriptor
typedef struct
{
	unsigned long Control;
        unsigned long Width_Height_Reg;
        unsigned long Srcaddrs;
        unsigned long Dstaddrs;
} Dma_Desc_ptr;

typedef struct
{
	unsigned char CRTC0 ; //0x60  // 0x480
	unsigned char CRTC1 ; //0x61 // 0x481
	unsigned char CRTC2 ; // 62 0x482
	unsigned char CRTC3 ; // 63 0x483
	unsigned char CRTC4 ; // 0x64 0x484
	unsigned char CRTC5 ; // 0x65 0x485
	unsigned char CRTC6 ; // 0x66 0x486
	unsigned char CRTC7 ; // 0x67 0x487
	unsigned char CRTC8 ; // 0x68 0x488
	unsigned char CRTC9 ; // 0x69 0x489
	unsigned char CRTCA ; // 0x6A 0x48A
	unsigned char CRTCB ; // 0x6B 0x48B
	unsigned char CRTCC ; // 0x6C 0x48C
	unsigned char CRTCD ; // 0x6D 0x48D
	unsigned char CRTCE ; // 0x6E 0x48E
	unsigned char CRTCF ; // 0x6F 0x48F
	unsigned char CRTC10 ; // 0x70 0x490
	unsigned char CRTC11 ; //0x71 0x491
	unsigned char CRTC12 ; //0x72  0x492
	unsigned char CRTC13 ; //0x73  0x493
	unsigned char CRTC14 ; //0x74  0x494
	unsigned char CRTC15 ; //0x75 0x495
	unsigned char CRTC16 ; //0x76 0x496
	unsigned char CRTC17 ; //0x77 0x497
	unsigned char CRTC18 ; //0x78 0x498
	unsigned char dummy1 ; //0x79 0x499
	unsigned char dummy2 ; //0x7A 0x49A
	unsigned char dummy3 ; //0x7B 0x49B
	unsigned char dummy4 ; //0x7C 0x49C
	unsigned char dummy5 ; //0x7D 0x49D
	unsigned char dummy6 ; //0x7E 0x49E
	unsigned char dummy7 ; //0X7F 0x49F
	unsigned char dummy8 ; //0x80 0x4A0
	unsigned char dummy9 ; //0x81 0x4A1
	unsigned char CRTC22 ; //x82 0x4A2
	unsigned char dummyA ; //x83 0x4A3
	unsigned char CRTC24 ; //x84 0x4A4
	unsigned char dummyB ; //x85 0x4A5
	unsigned char CRTC26 ; //x86 0x4A6
} CRTC0_REG_STRUCT ;

typedef struct
{
	unsigned char CRTCEXT0; //0x80
	unsigned char CRTCEXT1; //0x81
	unsigned char CRTCEXT2; //0x82
	unsigned char CRTCEXT3; //0x83
	unsigned char CRTCEXT4; //0x84
	unsigned char CRTCEXT5; //0x85
	unsigned char CRTCEXT6; //0x86
	unsigned char CRTCEXT7; //0x87
	unsigned char CRTCEXT8; //0x88
	unsigned char CRTCEXT9; //0x89
	unsigned char CRTCEXTA; //0x8A
	unsigned char CRTCEXTB; //0x8B
	unsigned char CRTCEXTC; //0x8C
	unsigned char CRTCEXTD; //0x8D
	unsigned char CRTCEXTE; //0x8E
	unsigned char CRTCEXTF; //0x8F
	unsigned char CRTCEXT10; //0x90
	unsigned char CRTCEXT11; //0x91
	unsigned char CRTCEXT12; //0x92
	unsigned char CRTCEXT13; //0x93
	unsigned char CRTCEXT14; //0x94
	unsigned char CRTCEXT15; //0x95
	unsigned char CRTCEXT16; //0x96
	unsigned char CRTCEXT17; //0x97
	unsigned char CRTCEXT18; //0x98
}CRTCEXT_REG_STRCT;
typedef struct 
{
	u16 wWidth;
	u16 wHeight;
} Resolution;

typedef struct 
{
	unsigned char SEQ0 ;
	unsigned char SEQ1 ;
	unsigned char SEQ2 ;
	unsigned char SEQ3 ;
	unsigned char SEQ4 ;
	unsigned char SEQ5 ;
	unsigned char SEQ6 ;
	unsigned char SEQ7 ;
}
CRTC_SEQ_REG_STRUCT;


typedef struct
{
	unsigned char GCTL0 ;// 0x30
	unsigned char GCTL1 ;// 0x31
	unsigned char GCTL2 ;// 0x32
	unsigned char GCTL3 ;// 0x33
	unsigned char GCTL4 ;// 0x34
	unsigned char GCTL5 ;// 0x35
	unsigned char GCTL6 ;// 0x36
	unsigned char GCTL7 ;// 0x37
	unsigned char GCTL8 ;// 0x38
}
CRTC_GCTL_REG_STRUCT;

typedef struct // CHANGE NOT NEEDED 
{ 
	unsigned char ATTR0 ;	// 0x4E0
	unsigned char ATTR1 ;	// 0x4E1
	unsigned char ATTR2 ;	// 0x4E2
	unsigned char ATTR3 ;	// 0x4E3
	unsigned char ATTR4 ;	// 0x4E4
	unsigned char ATTR5 ;	// 0x4E5
	unsigned char ATTR6 ;	// 0x4E6
	unsigned char ATTR7 ;	// 0x4E7
	unsigned char ATTR8 ;	// 0x4E8
	unsigned char ATTR9 ;	// 0x4E9
	unsigned char ATTRA ;	// 0x4EA
	unsigned char ATTRB ;	// 0x$EB
	unsigned char ATTRC ;	// 0x4EC
	unsigned char ATTRD ;	// 0x4ED
	unsigned char ATTRE ;	// 0x4EE
	unsigned char ATTRF ;	// 0x4EF
	unsigned char ATTR10 ;	// 0x4F0
	unsigned char ATTR11 ;	// 0x4F1
	unsigned char ATTR12 ;	// 0x4F2
	unsigned char ATTR13 ;	// 0x4F3
	unsigned char ATTR14 ;	// 0x4F4
	
} ATTR_REG_STRUCT ; 

typedef struct // NO CHANGE NEEDED
{
	unsigned char XCURCOL0RED ;	// 0x400
	unsigned char XCURCOL0GREEN ;	// 0x401
	unsigned char XCURCOL0BLUE ;	// 0x402
	unsigned char dummy16 ;		// 0x403
	unsigned char XCURCOL1RED ;	// 0x404
	unsigned char XCURCOL1GREEN ;	// 0x405
	unsigned char XCURCOL1BLUE ;	// 0x406
	unsigned char dummy17 ;		// 0x407
	unsigned char XCURCOL2RED ;	// 0x406
	unsigned char XCURCOL2GREEN ;	// 0x409
	unsigned char XCURCOL2BLUE ;	// 0x40A
	unsigned char dummy1 ;		// 0x40B
	unsigned char XCURCOL3RED ;	// 0x40C
	unsigned char XCURCOL3GREEN ;	// 0x40D
	unsigned char XCURCOL3BLUE ;	// 0x40E
	unsigned char dummy2 ;		// 0x40F
	unsigned char XCURCOL4RED ;	// 0x410
	unsigned char XCURCOL4GREEN ;	// 0x411
	unsigned char XCURCOL4BLUE ;	// 0x412
	unsigned char dummy3 ;		// 0x413
	unsigned char XCURCOL5RED ;	// 0x414
	unsigned char XCURCOL5GREEN ;	// 0x415
	unsigned char XCURCOL5BLUE ;	// 0x416
	unsigned char dummy4 ;		// 0x417
	unsigned char XCURCOL6RED ;	// 0x418
	unsigned char XCURCOL6GREEN ;	// 0x419
	unsigned char XCURCOL6BLUE ;	// 0x41A
	unsigned char dummy5 ;		// 0x41B
	unsigned char XCURCOL7RED ;	// 0x41C
	unsigned char XCURCOL7GREEN ;	// 0x41D
	unsigned char XCURCOL7BLUE ;	// 0x41E
	unsigned char dummy6 ;		// 0x41F
	unsigned char XCURCOL8RED ;	// 0x420
	unsigned char XCURCOL8GREEN ;	// 0x421
	unsigned char XCURCOL8BLUE ;	// 0x422
	unsigned char dummy7 ;		// 0x423
	unsigned char XCURCOL9RED ;	// 0x424
	unsigned char XCURCOL9GREEN ;	// 0x425
	unsigned char XCURCOL9BLUE ;	// 0x426
	unsigned char dummy8 ;		// 0x427
	unsigned char XCURCOL10RED ;	// 0x428
	unsigned char XCURCOL10GREEN ;	// 0x429
	unsigned char XCURCOL10BLUE ;	// 0x42A
	unsigned char dummy9 ;		// 0x42B
	unsigned char XCURCOL11RED ;	// 0x42C
	unsigned char XCURCOL11GREEN ;	// 0x42D
	unsigned char XCURCOL11BLUE ;	// 0x42E
	unsigned char dummy10 ;		// 0x42F
	unsigned char XCURCOL12RED ;	// 0x430
	unsigned char XCURCOL12GREEN ;	// 0x431
	unsigned char XCURCOL12BLUE ;	// 0x432
	unsigned char dummy11 ;		// 0x433
	unsigned char XCURCOL13RED ;	// 0x434
	unsigned char XCURCOL13GREEN ;	// 0x435
	unsigned char XCURCOL13BLUE ;	// 0x436
	unsigned char dummy12 ;		// ox437
	unsigned char XCURCOL14RED ;	// 0x438
	unsigned char XCURCOL14GREEN ;	// 0x439
	unsigned char XCURCOL14BLUE ;	// 0x43A
	unsigned char dummy13 ;		// 0x43B
	unsigned char XCURCOL15RED ;	// 0x43C
	unsigned char XCURCOL15GREEN ;	// 0x43D
	unsigned char XCURCOL15BLUE ;	// 0x43E
	unsigned char dummy14 ;		// 0x43F
	unsigned char CURPOSXL ;	// 0x440
	unsigned char CURPOSXH ;	// 0x441
	unsigned char CURPOSYL ;	// 0x442
	unsigned char CURPOSYH ;	// 0x443
	unsigned char XCURADDL ;	// ox444
	unsigned char XCURADDH ;	// 0x445
	unsigned char XCURCTL ;		// 0x446
	unsigned char dummy15 ;		// 0x447
	unsigned char XCOLMSK ;		// 0x448
	unsigned char XCOLMSK0RED ;	// 0x449
	unsigned char XCOLMSK0GREEN ;	// 0x44A
	unsigned char XCOLMSK0BLUE ;	// 0x44B
	unsigned char XCOLKEY ;		// 0x44C
	unsigned char XCOLKEY0RED ;	// 0x44D
	unsigned char XCOLKEY0GREEN ;	// 0x44E
	unsigned char XCOLKEY0BLUE ;	// 0x44F
	unsigned char XMULCTRL ;	// 0x450
	unsigned char XGENCTRL ;	// 0x451
	unsigned char XMISCCTRL ;	// 0x452
	unsigned char XZOOMCTRL ;	// 0x453
	unsigned char MISC ;		// 0x454
	unsigned char FEAT ;		// 0x455
	unsigned char PIXRDMSK ;	// 0x456
	unsigned char XKEYOPMODE ;	// 0x457
} GRC_REGS_STRUCT ;

typedef struct // change not needed	
{
	// unsigned long GRCCTL0;         0x58		// 0x500
	unsigned long GRCCTL1;         //0x58// 0x504
	unsigned long GRCSTS;	       //0x5c 0x508	
	// unsigned long GRCICNT;         // 0x50C
}GRC_STRUCT;

#define GRC_OFFSET                  0x58          //  (0x500)
#define GRC_IRQ_MASK				(0x000003ff)
#define SNOOP_FIQ_MASK				(0x00000200)
#define GRC_INTR				(0x00000041)
#define XCURPOS_INTR				(0x00000200)
#define XCURCTL_INTR				(0x00000100)
#define XCURCOL_INTR				(0x00000080)
#define CRTC_N_EXTCRTC_INTR			(0x00000030)
#define XCURSOR_INTR				(0x00000380)
#define CURS_INTR				(0x00000010)
#define SEQ1_OFFSET				(0x4C8)
#define SEQ1_POWOFF				(0x00000020)
#define SEQ1_INTR				(0x00000004)
#define SEQ1_BLNKSCRN				(0x30)

// Tile Snoop Control Register Assignments
#define BYTES_PER_PIXEL_MASK			(0x0000000C)
#define COLUMNS_PER_TILE_MASK			(0x000000C0)
#define ROWS_PER_TILE_MASK			(0x00000030)

#define TILE_SNOOP_ENABLE			(0x00000001)
#define EXTENDED_GRAPHICS_MODE			(0x00000002)
#define ONE_BYTE_PER_PIXEL			(0x00000000)
#define TWO_BYTES_PER_PIXEL			(0x00000004)
#define FOUR_BYTES_PER_PIXEL			(0x00000008)
#define EIGHT_BYTES_PER_PIXEL			(0x0000000C)
#define ROWS_PER_TILE_16			(0x00000000)
#define ROWS_PER_TILE_32			(0x00000010)
#define ROWS_PER_TILE_64			(0x00000020)
#define ROWS_PER_TILE_128			(0x00000030)
#define COLUMNS_PER_TILE_16			(0x00000000)
#define COLUMNS_PER_TILE_32			(0x00000040)
#define COLUMNS_PER_TILE_64			(0x00000080)
#define COLUMNS_PER_TILE_128			(0x000000C0)
#define TSE_IRQ_ENABLE				(0x00000100)
#define INIT_SCREEN_0				(0x00002000)
#define INIT_SCREEN_1				(0x00004000)
#define SNOOP_COPY_OWNER			(0x00008000)
#define TSE_OFFSET				16

// Tile Snoop Status Register Assignments
#define TILE_COUNTER_EXPIRED_SCREEN_0		1<<0
#define TILE_COUNTER_EXPIRED_SCREEN_1		1<<1
#define ASCII_FIELD_MODIFIED			1<<2
#define ATTR_FIELD_MODIFIED			1<<3
#define FONT_FIELD_MODIFIED			1<<4

// Tile Fetch Control Register Assignments
#define VGA_TEXT_ALIGNMENT_IS_8_BYTES		(0x00000000)
#define VGA_TEXT_ALIGNMENT_IS_16_BYTES		(0x00000008)

// Tile Fetch Status Register Assignments
#define TFE_INTERRUPT				(0x00000002)

//TFE Register and Descriptor  DEFINES
#define MODE0_NORMAL            0<<13
#define MODE1_4BITPLNR          1<<13
#define MODE2_4BITPKD           2<<13
#define MODE3_ATTRASCII         3<<13
#define MODE4_ASCIIONLY         4<<13
#define MODE5_FONTFMODE         5<<13
                                                                                    
#define TFE_MODE0_NORMAL                0<<13
#define TFE_MODE1_4BITPLNR              1<<13
#define TFE_MODE2_4BITPKD               2<<13
#define TFE_MODE3_ATTRASCII             3<<13
#define TFE_MODE4_ASCIIONLY             4<<13
#define TFE_MODE5_FONTMODE             	5<<13
#define TFE_MODE6_SPLITBYTES		6<<13
#define TFE_I                   1<<0
#define TFE_LAST                0<<1
#define TFE_NOTLAST             1<<1
#define TFE_SRC_DDR             0<<2
#define TFE_SRC_LMEM            1<<2
#define TFE_DST_DDR             0<<3
#define TFE_DST_LMEM            1<<3
#define TFE_RLE_EN              1<<4
#define FE_CHKSUM_EN            1<<5
#define TFE_RLE_OVRFLW          1<<7
#define TFE_RLE_NOOVRFLW        0<<7
                                                                                    
#define TFE_ENABLE              1<<0
#define TFE_IRQ_ENABLE          1<<1
#define TFE_FIQ_ENABLE          1<<2 
#define TFE_8_16_JUMP           1<<3 
#define TFE_BUSY                1<<0

#define TFE_FETCH_ASCII			1<<0
#define TFE_FETCH_ATTRASCII		1<<1
#define TFE_FETCH_FONT			1<<2
#define TFE_FETCH_4BIT_PACKED		1<<3
#define TFE_FETCH_4BIT_PLANAR		1<<4
#define TFE_FETCH_8_16_JUMP		1<<5
#define TFE_SPLIT_BYTES			1<<6

// BSCMD Register
#define BSE_ENABLE			1<<0
#define BSE_IRQ_ENABLE			1<<1
#define BSE_FIQ_ENABLE			1<<2
#define BSE_DESC_LMEM			1<<3
#define BSE_DESC_DDR			0<<3
#define BSE_LAST        	        0<<1
#define BSE_NOTLAST             	1<<1

// BSSTS Register
#define BSE_BUSY			1<<0
//SCU418
#define VGAVS_ENBL 			(1<<31)
#define VGAHS_ENBL 			(1<<30)
//SCU0C0
#define VGA_CRT_DISBL 		(1<<17)
//SCU0D0
#define PWR_OFF_VDAC 		(1<<3)
#endif

