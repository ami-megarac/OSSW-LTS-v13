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

/****************************************************************
 * @file 	cap90xx.c
 * @authors	Vinesh Christoper 		<vineshc@ami.com>,
 *		Baskar Parthiban 		<bparthiban@ami.com>,
 *		Varadachari Sudan Ayanam 	<varadacharia@ami.com>
 * @brief   	9080 Fpga Capture screen logic core function
 *			definitions
 ****************************************************************/

#ifndef MODULE				/* If not defined in Makefile */
#define MODULE
#endif

#ifndef __KERNEL__			/* If not defined in Makefile */
#define __KERNEL__
#endif

#ifndef EXPORT_SYMTAB		/* If not defined in Makefile */
#define EXPORT_SYMTAB
#endif


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <asm/delay.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <asm/io.h>
#include "cap90xx.h"
#include "dbgout.h"
#include "videocap.h"
#include "fge.h"
#include "capture.h"
#include "videodata.h"	
#include "profile.h"
#define MG9080_CONTROL_FLAGS		(                                                   \
		MG9080_CCR_ENABLE  | MG9080_ALOVICA_ENABLE		\
		)

// globals
unsigned char 						g_IsModeChange 	= FALSE;
volatile unsigned int					g_tse_sts = 0;
unsigned int						g_bpp15 = 0;
unsigned int						g_xcur_all = 1;
unsigned char						g_video_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;
unsigned char						g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;

// locals
volatile static int					m_cap_ready = IS_IT_TSE;
volatile static int 					m_pal_ready = IS_IT_PAL;
volatile static int					m_xcur_ready = IS_IT_XCURPOS;
volatile static int					g_tfe_ready = 0;
volatile static int					g_bse_ready = 0;
volatile unsigned int					g_ScreenBlank = 0;

unsigned char tse_rr_base_copy[512];
unsigned long rowsts[2], colsts[2]; 
unsigned int AllRows = 0;
unsigned int AllCols = 0;

// local functions
static void copy_video_mode_params(VIDEO_MODE_PARAMS *src, VIDEO_MODE_PARAMS *dest);
static int compare_video_mode_params( VIDEO_MODE_PARAMS *vm1, VIDEO_MODE_PARAMS *vm2);
static void disable_bse_intr(void);
static void enable_tse_intr(void);
static void disable_tse_intr(void);
static void disable_tfe_intr(void);
static void enable_pal_intr(void);
static void disable_pal_intr(void);
static void enable_xcursor_intr(void);
static void disable_xcursor_intr(void);
static int check_blank_screen(void);
static int Get_TS_Map_Info(void);
static void check_for_new_session(unsigned int cap_flags);

static void get_video_mode_params_using_index ( VIDEO_MODE_PARAMS *vm );

static Resolution supportedResoultion[0x3B - 0x30 + 1] = {
													{ 800, 600 },
													{ 1024, 768 },
													{ 1280, 1024 },
													{ 1600, 1200 },
													{ 1920, 1200 },
													{ 1280, 800 },
													{ 1440, 900 },
													{ 1680, 1050 },
													{ 1920, 1080 },
													{ 1360, 768 },
													{ 1600, 900 },
													{ 1152, 864 }
												};
//Supported Resolution which are lesser than 800x600
static Resolution lowResolutionList[0x52 - 0x50 + 1] = {
													{ 320, 240 },
													{ 400, 300 },
													{ 512, 384 }
												};

/* Wait queue */
DECLARE_WAIT_QUEUE_HEAD(intr_wq);
DECLARE_WAIT_QUEUE_HEAD(pal_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(tfe_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(bse_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(xcur_intr_wq);

/* 
   FGB interrupt handler
*/
irqreturn_t pilot_fgb_intr(int irq, void *dev_id)
{
	volatile unsigned int grc_ctl = 0;
	volatile unsigned int grc_sts = 0;

	/*
	 * BIT SLICE ENGINE INTERRUPT HANDLING SECTION
	 */
	if (fgb_bse_regs->BSCMD & BSE_IRQ_ENABLE)
	{
		if (fgb_bse_regs->BSSTS & BSE_IRQ_ENABLE)
		{
			disable_bse_intr();

			g_bse_ready = IS_IT_BSE;
			wake_up_interruptible(&bse_intr_wq);
		}
	}

	/* 
	 * TILE FETCH ENGINE INTERRUPT HANDLING SECTION
	 */
	if (fgb_tfe_regs->TFCTL & TFE_IRQ_ENABLE)
	{
		if (fgb_tfe_regs->TFSTS & TFE_IRQ_ENABLE)
		{
			disable_tfe_intr();

			g_tfe_ready = IS_IT_TFE;
			wake_up_interruptible(&tfe_intr_wq);
		}
	}

	/* 
	 * PALETTE & XCURSOR INTERRUPT HANDLING SECTION
	 */
	grc_ctl = grc_ctl_regs->GRCCTL1;
	if (grc_ctl & (GRC_INTR | XCURSOR_INTR)) 
	{
		grc_sts = grc_ctl_regs->GRCSTS;
		
		// This is the case for Palette
		if (grc_sts & GRC_INTR) 
		{
			disable_pal_intr();

			m_pal_ready = IS_IT_PAL;
			wake_up_interruptible(&pal_intr_wq);
		}

		// This is the case for Hw Cursor
		if (grc_sts & XCURSOR_INTR)
		{
			disable_xcursor_intr();

			if (grc_sts & XCURPOS_INTR)
			{
				m_xcur_ready = IS_IT_XCURPOS;
			}
			
			if (grc_sts & XCURCTL_INTR)
			{
				m_xcur_ready = IS_IT_XCURCTL;
			}

			if (grc_sts & XCURCOL_INTR)
			{
				m_xcur_ready = IS_IT_XCURCOL;
			}

			wake_up_interruptible(&xcur_intr_wq);
		}

		if( grc_sts & CRTC_N_EXTCRTC_INTR ){
			// printk("\n resolution or full video update \n");
			//no need to call get_video_mode_params as for every screen/tile capture we are calling it. 
		}
	}

	/*
	 * TSE INTERRUPT HANDLING SECTION
	 */
	if (fgb_tse_regs->TSCMD & TSE_IRQ_ENABLE) 
	{
		if(  fgb_tse_regs->TSSTS & ( 0x03/* 1 <<0 | 1 << 1 */ ) )
		{
			disable_tse_intr();
			g_tse_sts = fgb_tse_regs->TSSTS;

			if (g_tse_sts & 0x00000001) 
			{
				fgb_tse_regs->TSSTS |= (1 << 0);
			}

			if (g_tse_sts & 0x00000002) 
			{
				fgb_tse_regs->TSSTS |= (1 << 1);
			}

			m_cap_ready = IS_IT_TSE;
			wake_up_interruptible(&intr_wq);
		}
	}


	return IRQ_HANDLED;
}

void disable_bse_intr()
{
	fgb_bse_regs->BSCMD &= ~(BSE_IRQ_ENABLE);
	fgb_bse_regs->BSSTS |= BSE_IRQ_ENABLE;
}

void disable_tfe_intr()
{
	fgb_tfe_regs->TFCTL &= ~(TFE_IRQ_ENABLE);
	fgb_tfe_regs->TFSTS |= TFE_IRQ_ENABLE;
}

void enable_tse_intr() 
{
	fgb_tse_regs->TSCMD |= TSE_IRQ_ENABLE;
	fgb_tse_regs->TSCMD &= ~((0x00000200));//TSE_FIQ
	fgb_tse_regs->TSICR |= 0xCB700; ////50MHz clock ~1/60 sec
}

void disable_tse_intr() 
{
	fgb_tse_regs->TSCMD &= (~TSE_IRQ_ENABLE | (0x00000200) );

}

void enable_pal_intr()
{
	grc_ctl_regs->GRCSTS |= GRC_INTR; // ATTR00-0F  and  PLT_RAM
	grc_ctl_regs->GRCCTL1 |= GRC_INTR;
}

void disable_pal_intr()
{
	grc_ctl_regs->GRCCTL1 &= ~(GRC_INTR);
}

void enable_xcursor_intr()
{
	grc_ctl_regs->GRCCTL1 |= XCURSOR_INTR;
}

void disable_xcursor_intr()
{
	grc_ctl_regs->GRCCTL1 &= ~(XCURSOR_INTR);
} 

void get_current_palette(u8 *pal_address)
{
	unsigned char *palette ;
	int x,i,line;

	palette = (unsigned char *) ( grc_base + GRC_PALETTE_OFFSET );

	/**
	 * Verifying for the access of the user-space area thats passed here
	 * Copying the palette data from the pilot-fg base to the allocated region
	 **/
	x=line=i=0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0))
	if ( access_ok(VERIFY_WRITE, (char *)pal_address, 1024) )
#else
	if ( access_ok((char *)pal_address, 1024) )
#endif
	{
		if ( copy_to_user((void *)pal_address, (void *)palette, 1024) )
		{
			printk("Cannot copy palette data to the user end\n");
		}
	}
}

/* This fn is for Waiting and capturing the Palette
 * 1.  Enabling the Interrupt and Waiting until interrupt arrives and makes the condition TRUE
 * 2.  if condition is made TRUE, then call the Palette handling function
 */
int capture_palt(unsigned int cap_flags, u8 *palt_address)
{
	int lcl_ready;
	//int status;

	/* Enable Pal Interrupt and then wait indefinitely until a Palette interrupt arrives */
	/* First time alone, palette will be obtained unconditionally. Then on, only interrupt based */
	enable_pal_intr();

	/**
	 * Going to wait for an interrupt here
	 * Setting appropriate flags
	 **/
	wait_event_interruptible(pal_intr_wq, (m_pal_ready == IS_IT_PAL));
	lcl_ready = m_pal_ready;
	m_pal_ready = 0;

	if (lcl_ready != IS_IT_PAL)
	{
		return lcl_ready;
	}

	/**
	 * Going to get the current Palette 
	 * Passing the user-space address that we allocated in adviser
	 * Will be copying the palette data from pilot-fg to the allocated area
	 * This was done to avoid the junk color displayed occasionally
	 **/
	get_current_palette(palt_address);

	return lcl_ready;
}


/**
 * capture_screen
 * Tile Column Size support. Because fractional number of columns are difficult to deal with,
 * the following restrictions apply to the valid vesa modes.
 * Horizontal Resolution	Column Size supported
 * 640			16
 * 720			16
 * 800			16-32
 * 848			16
 * 1024			All
 * 1152			All
 * 1280			All
 * 1360			16
 * 1400			Not Supported
 * 1440			16-32
 * 1600			16-32-64
 * Get current screen resolution and if changed, reinit snoop engine. and return
 * Here flip ownership of screen.
 * Read Tile Row and Column registers and Process.
 * Do Tile Fetch
 * Get current screen resolution and if changed, discard data and reinit snoop engine
 **/

int capture_screen(unsigned int cap_flags,capture_param_t* cap_param)
{
//	int lcl_ready = 0;
	int status;

	enable_tse_intr();

	/**
	 * This condition occurs whenever a new session comes in
	 * Hence, for New Sessions alone, we are directly fetching the data
	 **/
	check_for_new_session(cap_flags);

	wait_event_interruptible_timeout(intr_wq, (m_cap_ready == IS_IT_TSE), (5 * HZ));
	m_cap_ready = 0;

	get_video_mode_params(&before_video_capture);

	if (cap_flags & CAPTURE_NEW_SESSION)
	{
		copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
		copy_video_mode_params(&before_video_capture, &status_video_mode);
	}

	cap_flags &= ~(CAPTURE_NEW_SESSION);

	if(compare_video_mode_params(&before_video_capture, &status_video_mode)) 
	{
		g_IsModeChange = TRUE ;
		copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
		copy_video_mode_params(&before_video_capture, &status_video_mode);
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = MODE_CHANGE ;
		return 1 ;
	}
	copy_video_mode_params(&before_video_capture, &status_video_mode);
	copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
	g_IsModeChange = FALSE;
	
	if ( check_blank_screen() )
	{
		return CAPTURE_ERROR;
	}

	if (cap_flags & CAPTURE_CLEAR_BUFFER) 
	{
		if (current_video_mode.width <= 1) 
		{ 
			// possible hard reset of PILOTII
			current_video_mode.capture_status = NO_VIDEO ;
			return CAPTURE_ERROR;
		}
	} 

	// get column info
	AllRows = (current_video_mode.height / TileColumnSize);
	AllCols = (current_video_mode.width / TileRowSize);

	if ((current_video_mode.height % TileColumnSize) > 0) AllRows += 1;
	if ((current_video_mode.width % TileRowSize) > 0) AllCols += 1;

	if (Get_TS_Map_Info()) 
	{
		return CAPTURE_ERROR;
	}

	if(current_video_mode.video_mode_flags & TEXT_MODE)
	{
		status = capture_text_screen(cap_flags);
	} 
	else if(current_video_mode.video_mode_flags & MODE13)
	{
		status = capture_mode13_screen(cap_flags);
	}
	else {
		status = capture_tile_screen(cap_flags);
	}

	get_video_mode_params(&after_video_capture);
	if(compare_video_mode_params(&before_video_capture, &after_video_capture)) 
	{
		g_IsModeChange = TRUE ;
		copy_video_mode_params(&after_video_capture, &current_video_mode) ; // make current
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = MODE_CHANGE ;
		return 1 ;
	}

	if ( check_blank_screen() )
	{
		return CAPTURE_ERROR;
	}

	if (CAPTURE_HANDLED != status) 
	{
		return status;
	} 

	current_video_mode.capture_status = 0 ;

	return CAPTURE_HANDLED;
}

void check_for_new_session(unsigned int cap_flags)
{
	if ( cap_flags & CAPTURE_NEW_SESSION )
	{
		m_cap_ready = IS_IT_TSE;
		m_pal_ready = IS_IT_PAL;
		m_xcur_ready = IS_IT_XCURCTL;
		g_xcur_all = 1;
	}

	return;
}

int Get_TS_Map_Info(void)
{
	u8 byTSCMDBytesPerPixel = 0;
	u8 bySnoopBit;
	u8 byBytesPerPixels = 0;
	u16 wStride = 0;
	u32 TSFBSA = 0;
	// u32 color_depth = 0;// convert bbp value(1,2,3,4) to 8, 16, 24, 32.
	if((current_video_mode.video_mode_flags & TEXT_MODE) || (current_video_mode.video_mode_flags & MODE13) ){
		fgb_tse_regs->TSCMD = 0x1; // Bit 0(1): TSE enabled; Bit 1(0): VGAText mode	
		current_video_mode.text_snoop_status = fgb_tse_regs->TSSTS ; // clear 
		fgb_tse_regs->TSTMUL = (1024 * 1024);
		return 0;
	}


	byBytesPerPixels = current_video_mode.depth;   
	switch ( byBytesPerPixels ) {
		case 1: byTSCMDBytesPerPixel = 0;
			break;
		case 2:
			// for tiles capture only
			byTSCMDBytesPerPixel = 1;
			break;
		case 3:
		case 4: byTSCMDBytesPerPixel = 2;
			break;
	}
	wStride = current_video_mode.stride;
	// Capture 3BytesPP as 4BytesPP
	if( byBytesPerPixels == 3 ) {
		// for capture tile rectangle we capture as 32bpp
		// wStride = (current_video_mode.stride * 3 ) >> byTSCMDBytesPerPixel;
		 wStride = (current_video_mode.stride * 3 ) >> 2;
		// if ((wStride >> 5) > 64) // not in Sdk. So commenting out. 
		// {
		// 	byTSCMDBytesPerPixel = 2;
		// 	wStride = (current_video_mode.stride * 3) >> byTSCMDBytesPerPixel;
		// }
	}
	// else if( byBytesPerPixels == 4) { 
	//if byBytesPerPixels == 3 then bpp == 32
	// Stride = current_video_mode.stride >> 1 should be execcuted, only if bpp = 4 
	{
		wStride = current_video_mode.stride;
	}

	/* EIP 317646.
	 * ref from pilot4SDK2016_1130 or later.
	 * file: hardwareEngines.c line:427.
	 * update value stored in TSFBSA register in TSE.
	 */
	TSFBSA = current_video_mode.video_buffer_offset;

	if(TSFBSA != fgb_tse_regs->TSFBSA)
	{
		fgb_tse_regs->TSFBSA = TSFBSA;
	}

	// set upper limit based on screen
	fgb_tse_regs->TSTMUL = 0x008CA000; //9MB //0x900000; //Pilot-IV Tile Snoop Upper Limit Register

	// flip the bit
	//Snoop Screen Copy Owner
	if( fgb_tse_regs->TSCMD & 0x00008000 ) {
		// Graphics updates to copy 0, processor can view copy1
		bySnoopBit = 0;
	}
	else
	{
		//Graphics updates to copy 1, processor can view copy0
		bySnoopBit = 1; 
	}
	// enable TSE

	// enable TSE interrrupt
	fgb_tse_regs->TSCMD |= TSE_IRQ_ENABLE;
	// set up TSE
	fgb_tse_regs->TSCMD |= TILE_SNOOP_ENABLE;

	if ( bpp4_mode == 0x12){
		current_video_mode.stride = 640;
		wStride = 320;
		fgb_tse_regs->TSCMD = (wStride<< 16) | (bySnoopBit << 15) | (1<<6) | (1<<4) |(1<<2) | (0x3);		
	}
	else if(bpp4_mode == 0x2F){// 2f
		current_video_mode.stride = 800;
		wStride = 400;
		fgb_tse_regs->TSCMD = (wStride<< 16) | (bySnoopBit << 15) | (1<<6) | (1<<4)|( 1<<2) | (0x3);		
	}
	else {
		fgb_tse_regs->TSCMD = (wStride << 16) | (bySnoopBit << 15) | (1<<6) /*32 colum per tile*/ | (1<<4) /*32 row per tile*/ |( byTSCMDBytesPerPixel<<2) | (0x3)/*Bytes Per Pixel (BPP)*/;
	}

	// after snoop engine is done

	// for Rectangle capture
	rowsts[0] = fgb_tse_regs->TSRSTS0;
	rowsts[1] = fgb_tse_regs->TSRSTS1;
	colsts[0] = fgb_tse_regs->TSCSTS0;
	colsts[1] = fgb_tse_regs->TSCSTS1;

	// for tiles only capture
	// yet another quick work around
	if ((byTSCMDBytesPerPixel == 2) && (byBytesPerPixels == 3))
	{

	}
	else {
		//In SDK, sizeof(array) will used. It is same as 512. u64 arrary[64]
		//TSRR127 -TSRR000  (3FFh - 200 == 1FF(511))
		memcpy((char *)&tse_rr_base_copy[0], (char *)fgb_tse_rr_base, 512);
	}

	if ((byBytesPerPixels == 3) && (byTSCMDBytesPerPixel == 1)) { 
		Convert2Bppto3Bpp((unsigned long *)&tse_rr_base_copy[0], ((current_video_mode.stride * 3) >> byTSCMDBytesPerPixel), current_video_mode.vdispend);
	}

	return 0;
}



void get_video_mode_params(VIDEO_MODE_PARAMS *vm)
{
	unsigned short temp16;
	unsigned short crtc13,crtc14,crtc17;
	unsigned long crtcBO;
	unsigned short vgar_8c;
	unsigned short new_color_mode = 0;

	// video flags
	vm->video_mode_flags = 0;
	// vm->depth
	vgar_8c = *(volatile u8*)(grc_base+0x8C);
	new_color_mode = vgar_8c >> 4;
	// get bytes per pixel
	vm->depth = get_bpp_by_new_color_mode(new_color_mode, vm );

	/***
	//SLES OS 800*600 75Hz return dowuuble the height size,n
	 * Matrox has pointed out that when determining the screen height conv2t4( CRTC09 bit 7 ) should also be checked.
		After calculating the screen height, if conv2t4 is 0 then it is correct and if it is 1 then you will have to divide the height by 2.
	 * 
	 */
	{
		get_video_mode_params_using_index(vm);
	}

	// get stride
	crtc13 = (unsigned short)(crtc->CRTC13);
	crtc14 = (unsigned short)(crtc->CRTC14);
	crtc17 = (unsigned short)(crtc->CRTC17);
	crtcBO = *(volatile u8*)(grc_base+0xB0);

	temp16 = (crtcBO << 8) | crtc13;
	if (crtc14 & 0x40)
		vm->stride = temp16 << 3; //DW mode
	else if (crtc17 & 0x40)
		vm->stride = temp16 << 1; //byte mode
	else
		vm->stride = temp16 << 2; //word mode

	if (vm->depth != 0 ) {
		u8 byBppPowerOfTwo = 0;
		if (byBitsPerPixel == 32)
			byBppPowerOfTwo = 2;
		else if (byBitsPerPixel == 16 )
			byBppPowerOfTwo = 1;
		else if (byBitsPerPixel == 8)
			byBppPowerOfTwo = 0;
		else
			byBppPowerOfTwo = 3;// 4bpp

		//convert it to logic width in pixel
		if (byBitsPerPixel > 4  /*depth > 4*/) {
			vm->stride >>= byBppPowerOfTwo;
		}
		else{
			vm->stride <<= byBppPowerOfTwo;
		}
	}

	if (vm->stride < vm->width)
		vm->stride = vm->width;
	vm->char_height = (crtc->CRTC9 & 0x1F) + 1 ;
	
	/*
	 * EIP 317646.
	 * ref from pilot4SDK2016_1130 or later.
	 * file: hardwareEngines.c line:424.
	 * check offset from register CrtcC, CrtcD, CrtcExt0.
	 */

	vm->video_buffer_offset = get_screen_offset(vm);	
}

unsigned int get_screen_offset(VIDEO_MODE_PARAMS *vm ) //video_buffer_offset
{
	unsigned int video_buffer_offset = 0;
	if (vm->video_mode_flags == MGA_MODE ) {
		video_buffer_offset =  ( ioread8((void *__iomem)( grc_base + CRTC0_OFFSET + 0x0C ) ) << 8 );
		video_buffer_offset |= ioread8( (void *__iomem) ( grc_base + CRTC0_OFFSET + 0x0D ) );
		video_buffer_offset |= ( ( ioread8( (void *__iomem) grc_base + CRTCEXT_OFFSET + 0x2F ) ) << 16 );
		video_buffer_offset *= vm->depth;
	}
	return video_buffer_offset;
}


unsigned char get_bpp_by_new_color_mode ( unsigned char  byNewColorMode, VIDEO_MODE_PARAMS *vm )
{
	unsigned char bpp = 0xFF;
	switch (byNewColorMode) {
	case 0: //EGA - 0
		vm->video_mode_flags = MODE12; //4pp mode12/mode6A //2
		byBitsPerPixel = 4;
		bpp = 5; //assumption. 
		break;

	case 1: //VGA - 1
		vm->video_mode_flags = MODE13; //mode 13 //2 
		bpp = 1;
		byBitsPerPixel = 8;
		break;
	
	case 2: // 15bpp -VGA/MGA ??? 2
		//TODO: ADD Code for 15 BPP
		vm->video_mode_flags = MGA_MODE;
		bpp = 2;
		byBitsPerPixel = 16;
		g_bpp15 = 1;
		break;

	case 3: // 16BPP - VGA/MGA ??? 3 
		vm->video_mode_flags = MGA_MODE; //3 
		byBitsPerPixel = 16;
		bpp = 2;
		break;

	case 4: // 32BPP -  VGA/MGA ?? 4
		vm->video_mode_flags = MGA_MODE; //3
		byBitsPerPixel = 32;
		bpp = 4;
		break;

	case 14: //TEXT MODE - 14
		vm->video_mode_flags = TEXT_MODE; //
		byBitsPerPixel = 0;
		break;

	case 15: //CGA MODE ???? 15
		break;

	default:
		byBitsPerPixel = 8;
		bpp = 1;
		break;
	}
	return bpp;

}

unsigned char get_bpp ( unsigned char XMulCtrl)
{
	unsigned char bpp = 0xFF;

	bpp = 0xFF ;
	switch(XMulCtrl) {
		case 0x0: 
			if (crtc->CRTC17 == 0xE3) {
				bpp = 5; // 4bpp //mode12???
				byBitsPerPixel = 4;
				if (crtc->CRTC3 == 0x82)
					bpp4_mode = 0x12;
				else if (crtc->CRTC3 == 0x80)
					bpp4_mode = 0x2F;
			}
			else 
			{
				bpp4_mode = 0x00;
				bpp = 1; // 8bpp
				byBitsPerPixel = 8;
			}
			break;

		case 0x1: // 15 bpp
			/**
			 * 15bpp was set the same case as 16bpp
			 * The RGB model for 15bpp is different from 16bpp in client end.
			 * Had to set the same bpp = 2 value for 15bpp as well for capturing purposes
			 * Hence, have set a flag to indicate 15bpp. If true, then sending flag to client
			 * Client will set the RGB model according to flag
			 **/
			bpp = 2;
			g_bpp15 = 1;
			byBitsPerPixel = 16;
			bpp4_mode = 0x00;
			break;

		case 0x2: // sixteen bits
			bpp = 2;
			bpp4_mode = 0x00;
			byBitsPerPixel = 16;
			break;

		case 0x3: // 24Bpp - now supported
			bpp = 3;
			bpp4_mode = 0x00;
			byBitsPerPixel = 24;
			break;

		case 0x4: // 32Bpp
			bpp = 4;
			bpp4_mode = 0x00;
			byBitsPerPixel = 32;
			break;

		case 0x5:
		case 0x6: // these are reserved codes
			bpp = 0xFF;
			bpp4_mode = 0x00;
			break;
		case 0x7:
			bpp = 4;
			byBitsPerPixel = 32;
			bpp4_mode = 0x00;
			break;
	}
	return bpp;
}


static int compare_video_mode_params( VIDEO_MODE_PARAMS *vm1, VIDEO_MODE_PARAMS *vm2)
{
	if(vm1->video_mode_flags != vm2->video_mode_flags) {
		return 1 ;
	}
	if(vm1->width != vm2->width) {
		return 1;
	}
	if(vm1->height != vm2->height) {
		return 1;
	}
	if(vm1->depth != vm2->depth) {
		return 1;
	}
	if(vm1->char_height != vm2->char_height) {
		return 1;
	}
	if(vm1->video_buffer_offset != vm2->video_buffer_offset) {
		copy_video_mode_params(vm1, vm2) ; // make current			
	}
	return 0 ;
}

static void copy_video_mode_params( VIDEO_MODE_PARAMS *src,  VIDEO_MODE_PARAMS *dest)
{
	dest->width = src->width;
	dest->height = src->height;
	dest->depth = src->depth;
	dest->video_mode_flags = src->video_mode_flags;
	dest->columns_per_tile = src->columns_per_tile;
	dest->rows_per_tile = src->rows_per_tile;
	dest->char_height = src->char_height;
	dest->video_buffer_offset = src->video_buffer_offset;
	dest->stride = src->stride;
}


/**
 * This fn will re-initialize the driver capture thread
 * The capture thread will be waiting for an interrupt
 * Whenever we receive a resume call from JViewer
 * This will wake up the interruptible_wait call
 **/
void resume_video(void)
{
	// Resume the video thread
	if (m_pal_ready != IS_IT_PAL)
	{
		m_pal_ready = IS_IT_PAL;
		wake_up_interruptible(&pal_intr_wq);
	}

	// Resume the palette thread
	if (m_cap_ready != IS_IT_TSE)
	{
		m_cap_ready = IS_IT_TSE;
		wake_up_interruptible(&intr_wq);
	}
}
#define SCU_MISC_CTRL			0x0C0
#define SCU_MULTIFUN_CTRL4      0x8C
#define SCU_MULTIFUN_CTRL4_VPODE_ENABLE 0x00000001 /* bit 0 */
#define SCU_MULTIFUN_CTRL6      0x94
#define SCU_MULTIFUN_CTRL6_DVO_MASK     0x00000003 /* bit 1:0 */
#define SCU_MISC_CTRL_DAC_DISABLE		0x00000008 /* bit 3 */
/**
 * Checks for a flag
 * If Host is ON, then turns it OFF, else vice versa
**/
void control_display(unsigned char *lockstatus)
{
	u32 scu418,scu0C0,scu0D0,dp100,dp104;	
	unsigned int reg_val = 0;

	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x00); /* read SCU */

	if( reg_val == 0 ) {
		iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	}

	//CODE FROM SDK
	scu418 = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x418 );
	scu0C0 = ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL );
	scu0D0 =ioread32((void *__iomem)ast_scu_reg_virt_base + 0x0D0 );
	dp100 = ioread32( (void *__iomem)dp_descriptor_base+0x100 );
	dp104 = ioread32( (void *__iomem)dp_descriptor_base+0x104 );

	//lockstatus - 1(lock) , lockstatus - 0(unlock)
	// If lockstatus is 1, then lock the monitor else unlock the monitor

	if (*lockstatus == 0 ) {
		if (!(scu418 & (VGAVS_ENBL|VGAHS_ENBL))) {
			scu418 |= (VGAVS_ENBL|VGAHS_ENBL);
			iowrite32(scu418, (void *__iomem)ast_scu_reg_virt_base + 0x418);
		}
		if (scu0C0 & VGA_CRT_DISBL) {
			scu0C0 &= ~VGA_CRT_DISBL;
			iowrite32(scu0C0, (void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL );
		}
		if (scu0D0 & PWR_OFF_VDAC) {
			scu0D0 &= ~PWR_OFF_VDAC;
			iowrite32(scu0D0, (void *__iomem)ast_scu_reg_virt_base + 0x0D0);
		}
		if( !( dp100 & (1 << 24 ) ) ) {
			dp100 = dp100 | (1 << 24);
			iowrite32(dp100, (void *__iomem)dp_descriptor_base + 0x100);
		}
	} else if( *lockstatus == 1 ) { //turn off
		if (scu418 & (VGAVS_ENBL|VGAHS_ENBL)) {
			scu418 &= ~(VGAVS_ENBL|VGAHS_ENBL);
			iowrite32(scu418, (void *__iomem)ast_scu_reg_virt_base + 0x418);
		}
		if (!(scu0C0 & VGA_CRT_DISBL)) {
			scu0C0 |= VGA_CRT_DISBL;
			iowrite32(scu0C0, (void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL );
		}
		if (!(scu0D0 & PWR_OFF_VDAC)) {
			scu0D0 |= PWR_OFF_VDAC;
			iowrite32(scu0D0, (void *__iomem)ast_scu_reg_virt_base + 0x0D0);
		}
		if(dp100 & (1 << 24 ) ) {
			dp100 = dp100 & ~(1 << 24);
			iowrite32(dp100, (void *__iomem)dp_descriptor_base + 0x100);
			dp104 = dp104 & ~(1 << 8);
			iowrite32(dp104, (void *__iomem)dp_descriptor_base + 0x104 );
		}
	}

	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL );
	if (reg_val & (VGA_CRT_DISBL))
	{
		*lockstatus = 0x05;
	}
	else
	{
		*lockstatus = 0x04;
	}

}

int get_bandwidth_mode(void)
{
	return g_video_bandwidth_mode;
}

void set_bandwidth_mode(unsigned char Bandwidth_Mode)
{
	// Set the global flag to indicate the bandwidth mode
	// 0 = Normal mode
	// 1 = Low Bandwidth 8bpp
	// 2 = Low Bandwidth 16bpp
	// 3 = Low Bandwidth 8bpp (B&W)
	g_video_bandwidth_mode = g_low_bandwidth_mode = Bandwidth_Mode;

	// 3 is 8bpp B&W.  Hence, we set as 1 to indicate 8bpp capture
	if (g_video_bandwidth_mode == LOW_BANDWIDTH_MODE_8BPP_BW)
		g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_8BPP;

	// If the current screen BPP is same as our low bandwidth mode setting, then we reset it to normal mode
	if ((current_video_mode.depth == g_low_bandwidth_mode) || (g_low_bandwidth_mode == 0))
		g_video_bandwidth_mode = g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;

	current_video_mode.bandwidth_mode = g_video_bandwidth_mode;
	
	return;
}

/* 
 * This call will return whether the Pilot-III PCI Card will be visible to host or not
 * The PCI Vendor ID and PCI Device ID will be masked to hide the Pilot-III PCI from host
 * These IDs will be checked to identify and return if the card is available or not
*/
int get_card_presence(void)
{
	if (*card_presence_cntrl == P3_PCI_HIDE_MASK)
	{
		return 0;
	}
	else
	{
		return 1;
	}		
}

void control_card_presence(unsigned char CardPresence)
{
	// Check for CardPresence flag and act accordingly
	// If CardPresence is 1, then enable the Card else Hide the card's PCI IDs
	if (CardPresence)
	{
		// Sets the actual Device ID & Vendor ID for P3 PCI
		*card_presence_cntrl = ((P3_PCI_DEVICE_ID << 16) | (P3_PCI_VENDOR_ID));
	}
	else
	{
		// Hides the P3 PCI by resetting the PCI Device ID & Vendor ID with 0xFFFFFFFF
		*card_presence_cntrl = (P3_PCI_HIDE_MASK);
	}

	return;
} 

void set_pci_subsystem_Id(PCI_DEVID pcisubsysID)
{

      *PCISID_regs = ( (pcisubsysID.devID <<16) | (pcisubsysID.venID));

	return;
} 

unsigned long get_pci_subsystem_Id(void)
{
    return(*PCISID_regs);
}

void WaitForBSE()
{
	// Waits for BSE DMA Event to complete
	wait_event_interruptible_timeout(bse_intr_wq, (g_bse_ready == IS_IT_BSE), (100*HZ));
	g_bse_ready = 0;

	return;
}

void WaitForTFE()
{
	//unsigned int status = 0;

	// Waits for TFE DMA Event to complete : Waiting time is: 250 ms
	wait_event_interruptible_timeout(tfe_intr_wq, (g_tfe_ready == IS_IT_TFE), (100*HZ));
        g_tfe_ready = 0;

	return;
}

/**
 * Convert2Bppto3Bpp
 * for 24bpp capture, we capture it as a 2Bpp screen using TFE
 * Convert that 2Bpp data into 3Bpp data using this call.
 **/
void Convert2Bppto3Bpp(unsigned long *BaseSnoopMap, unsigned long Stride, unsigned long Height)
{
	unsigned int iLAllCols = 0;
	unsigned int iLAllRows = 0;
	unsigned int Row, Col;
	unsigned char SnoopMapBkup[512];
	unsigned char *CopyofSnoopMap;
	unsigned long *SnoopMapRow = NULL;
	unsigned char bySnoopBitMap = 0x00;
	unsigned char byIter = 0;
	unsigned long Bits = 63;
	unsigned char bySnoopBit;
	unsigned char ToggleSnoopBits = 0;

	/**
	iLAllCols = Stride >> 5;
	iLAllRows = Height >> 5;

	if (Height == 600)
		iLAllRows += 1;**/
	
	/** fixes **/
	/*Fix done by Emulex for RHEL black box Issue*/
	
	iLAllCols = (Stride+31) >> 5; // dwStride/32
	iLAllRows = (Height+31) >>5;
	iLAllCols = 3*((iLAllCols+2)/3);
	
	memset(SnoopMapBkup, 0x00, 512);

	for (Row = 0; Row < iLAllRows ; Row++)
	{
		CopyofSnoopMap = SnoopMapBkup;
		CopyofSnoopMap += (Row * 8);
		bySnoopBitMap = 0x00;
		byIter = 0;

		for (Col = 0; Col < iLAllCols ; Col++)
		{
			switch (Col)
			{
				case 0:
					SnoopMapRow = (BaseSnoopMap + (Row * 2));
					Bits = 31;
					break;
				case 32:
					SnoopMapRow = (BaseSnoopMap + ((Row * 2) + 1));
					Bits = 31;
					break;
			}
			bySnoopBit = (unsigned char)((*SnoopMapRow) << Bits >> 31);

			switch (ToggleSnoopBits)
			{
				case 0:
					ToggleSnoopBits = 1;
					if (bySnoopBit)
						bySnoopBitMap = 0x01;
					break;
				case 1:
					ToggleSnoopBits = 2;
					if (bySnoopBit)
						bySnoopBitMap |= 0x03;
					break;
				case 2:
					ToggleSnoopBits = 0;
					if (bySnoopBit)
						bySnoopBitMap |= 0x02;
					*CopyofSnoopMap |= (bySnoopBitMap << byIter);
					byIter += 2;
					if (byIter == 8)
					{
						CopyofSnoopMap++;
						byIter = 0x00;
					}
					bySnoopBitMap = 0x00;
					break;
			}
			Bits--;
		}
	}
	memcpy(BaseSnoopMap, SnoopMapBkup, 512);

	return;
}

/**
 * This fn will check for blank screen condition
 * The Combo of SEQ1 & CRTCEXT1 register is for BlankScreen and PowerSave conditions
 * The Combo of TSSTS and XMULCTRL is for PowerOFF and PowerON status conditions
 * The Combo of SEQ4, CRTC24, CRTC26 and TSSTS is for Power control conditions
 * This is based on the register updations done by the mga driver in Linux OS.
 * Also, the register conditions is different under windows when normally powered off and on and when powered off and on with standby state enabled.
 **/
static int check_blank_screen(void)
{
	u32 retval = 0;
	retval = ioread32((void *__iomem)grc_base + 0x9C);
	if ( (((crtc_seq->SEQ1) & SEQ1_POWOFF) || ((crtcext->CRTCEXT1) & SEQ1_BLNKSCRN))
			|| (((!(fgb_tse_regs->TSSTS)) && ((*XMULCTRL & 0x7) == 0x7)))
			//|| ((!((crtc->CRTC24) & 0x80)) && (!((crtc->CRTC26) & 0x20)) && ((crtcext->SEQ4) & 0x04))
			//CRTC24 == 0x80 --> the attribute controller is ready to accept a data value( according p4 doc)
			//CRTC26 == 0x20 ---> VGA Palette is enabled ( according p4 doc)
			//AST2600 -> VR35C/VGACR9C 24== VGA ENABLE , 26=== SCREEN OFF 30,31 === VGA POWER STATE(PCIS44(0,1))
			|| ( !( retval & ( 1 << 24) ) && ( retval & ( 1 << 26) ) && ((fgb_tse_regs->TSSTS) <= 0x00A) ) )   
	{
		g_IsModeChange = TRUE;
		current_video_mode.capture_status = MODE_CHANGE;
		g_ScreenBlank = 0;
		return 1;
	}
	return 0;
}	

/**
 * capture_xcur - wait for xcursor interrupt
 */

int capture_xcursor(xcursor_param_t *xcursor)
{
	int status;
	unsigned int lcl_status = 0;

	/* Enable XCursor Interrupt and then wait indefinitely until a XCursor interrupt arrives */
	/* First time alone, XCursor will be obtained unconditionally. Then on, only interrupt based */
	enable_xcursor_intr();

	if (xcursor->xcursor_mode == XCURSOR_ALL)
	{
		m_xcur_ready = IS_IT_XCURPOS;
		g_xcur_all = 1;
	}

	/**
	 * Going to wait for an interrupt here
	 * Setting appropriate flags
	**/
	status = wait_event_interruptible_timeout(xcur_intr_wq, (m_xcur_ready != 0), (1 * HZ));
	if (status == 0)
	{
		xcursor->xcursor_mode = XCURSOR_NONE;
		return status;
	}
	else
	{
		lcl_status = m_xcur_ready;
		m_xcur_ready = 0;
	}

	/* get hardware cursor info */
	status = get_xcursor_info(xcursor, lcl_status);
	
	return status;
}

static void get_video_mode_params_using_index ( VIDEO_MODE_PARAMS *vm )
{
	u8 modeIndex = ( crtcext->CRTCEXTE & 0xF0 );
	u8 index = 0;

	if ( (crtcext->CRTCEXTE == 0x12) || (modeIndex == 0x20) ) {
		vm->width = 640;
		vm->height = 480;
	} else if (modeIndex == 0x30) {
		index = crtcext->CRTCEXTE & 0x0f;
		vm->width = supportedResoultion[index].wWidth;
		vm->height = supportedResoultion[index].wHeight;
	} else if (modeIndex == 0x50) {
		index = crtcext->CRTCEXTE & 0x03;
		vm->width = lowResolutionList[index].wWidth;
		vm->height = lowResolutionList[index].wHeight;
	} else if (modeIndex == 0x60) {
		vm->width = 800;
		vm->height = 600;
	} else {
		vm->width = 0;
		vm->height = 0;
	}
	current_video_mode.width = current_video_mode.hdispend  = vm->width;
	current_video_mode.height = current_video_mode.vdispend = vm->height;
	if(crtc->CRTC9 & REG_BIT7)
	{
		current_video_mode.vdispend = vm->height/2;
	}
}

/**
 * get_hwcursor_info - get hardware cursor infomation
 * Read cursor mode, positions, maps data and colors
 */

int get_xcursor_info(xcursor_param_t *xcursor, unsigned int lcl_status)
{
	unsigned char colIndex = 0;
	volatile unsigned long *xcur_maps_base;
	xcursor_data_t *xcur_data = &(xcursor->xcursor_data);

	memset(xcursor, 0, sizeof(xcursor_param_t));

	switch (lcl_status)
	{
		case IS_IT_XCURPOS:
			xcursor->xcursor_mode = XCURSOR_POSCTL;
			break;
		case IS_IT_XCURCOL:
			xcursor->xcursor_mode = XCURSOR_ALL;
			break;
		case IS_IT_XCURCTL:
			xcursor->xcursor_mode = XCURSOR_CTL;
			break;
	}

	if (g_xcur_all)
	{
		xcursor->xcursor_mode = XCURSOR_ALL;
		g_xcur_all = 0;
	}
	
	/* get cursor mode */
	xcur_data->xcurpos.mode = grc_regs->XCURCTL;

	/* if the cursor is diabled, no need to send cursor info */
	if (xcur_data->xcurpos.mode == XCURSOR_MODE_DISABLED) {
		return 0;
	}

	/* get current cursor position */
	xcur_data->xcurpos.pos_x = grc_regs->CURPOSXL | (grc_regs->CURPOSXH << 8);
	xcur_data->xcurpos.pos_y = grc_regs->CURPOSYL | (grc_regs->CURPOSYH << 8);

	/* copy hw cursor buffer data from frame buffer */
	xcur_maps_base = (unsigned long *) (((grc_regs->XCURADDL << 10) | 
				((grc_regs->XCURADDH & 0x3F) << 18)) + framebuffer_base);
	/* Sometimes XCURADDH and XCURADDL crosses the 8mb buffer limit, 
	   to avoid kernel crash added the below condition. 
	   ToDo : Need to find and fix why the xcur_maps_base crosses the buffer limit. */ 
	if ( (xcur_data->xcurpos.mode == XCURSOR_MODE_16COLOR) && 
		 (((unsigned long int)(xcur_maps_base + (sizeof(unsigned long long) * XCURSOR_MAP_SIZE))) < 
			((unsigned long int)(framebuffer_base + FRAMEBUFFER_SIZE))) ) {
		/* total slice is 6 for 16 color mode */
		if (copy_to_user((void *) &xcur_data->xcurcol.maps, (void *) xcur_maps_base, 
			sizeof(unsigned long long) * XCURSOR_MAP_SIZE)) {
			printk("videocap: cannot copy xcursor map data to user end\n");
		}
	}
	else if ( ((unsigned long int)(xcur_maps_base + (sizeof(unsigned long long) * 128))) < 
			  ((unsigned long int)(framebuffer_base + FRAMEBUFFER_SIZE)) ) {
		/* total slice is 2 for other mode; each slice 64bits */
		if (copy_to_user((void *) &xcur_data->xcurcol.maps, (void *) xcur_maps_base, 
			sizeof(unsigned long long) * 128)) {
			printk("videocap: cannot copy xcursor map data to user end\n");
		}
	}

	/* read hw cursor color register */
	xcur_data->xcurcol.color[colIndex].red = grc_regs->XCURCOL0RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL0GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL0BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL1RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL1GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL1BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL2RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL2GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL2BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL3RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL3GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL3BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL4RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL4GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL4BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL5RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL5GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL5BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL6RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL6GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL6BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL7RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL7GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL7BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL8RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL8GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL8BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL9RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL9GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL9BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL10RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL10GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL10BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL11RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL11GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL11BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL12RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL12GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL12BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL13RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL13GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL13BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL14RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL14GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL14BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL15RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL15GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL15BLUE;

	return 0;
}



