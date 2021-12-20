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
****************************************************************
 ****************************************************************/

#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
#include <linux/dma-direction.h>
#endif
#include <linux/uaccess.h>

#ifndef SOC_AST2600
#include <mach/platform.h>
#else
// #include <linux/reset.h>
#endif

#include "ast_videocap_ioctl.h"
#include "ast_videocap_data.h"
#include "ast_videocap_functions.h"

static unsigned char EncodeKeys[256];
unsigned long JPEGHeaderTable[12288];

static void *prev_cursor_pattern_addr;
static uint32_t prev_cursor_status;
static uint32_t prev_cursor_position;
static uint16_t prev_cursor_pos_x;
static uint16_t prev_cursor_pos_y;
static uint32_t prev_cursor_checksum;
static uint32_t full_screen_capture;

static uint16_t cursor_pos_x;
static uint16_t cursor_pos_y;

static int ReInitVideoEngine = 0;
static int ISRDetectedModeOutOfLock = 1;

unsigned int TimeoutCount = 0;
unsigned int ModeDetTimeouts = 0;
unsigned int CaptureTimeouts = 0;
unsigned int CompressionTimeouts = 0;
unsigned int SpuriousTimeouts = 0;
unsigned int AutoModeTimeouts = 0;

struct ast_videocap_engine_info_t ast_videocap_engine_info;

static int ast_videocap_data_in_old_video_buf;

static int ModeDetectionIntrRecd;
static int CaptureIntrRecd;
static int CompressionIntrRecd;
static int AutoModeIntrRecd;

/* Status variables for help in debugging  */
int WaitingForModeDetection;
int WaitingForCapture;
int WaitingForCompression;
int WaitingForAutoMode = 0;
int CaptureMode = VIDEOCAP_YUV_SUPPORT;

static void *vga_mem_virt_addr;

/*  Timer related variables for Video Engine */
#define VE_TIMEOUT          1   /* in Seconds   */
unsigned long TimeToWait;
wait_queue_head_t mdwq;
wait_queue_head_t capwq;
wait_queue_head_t compwq;
wait_queue_head_t autowq;

/* Defines for Video Interrupt Control Register */
#define IRQ_WATCHDOG_OUT_OF_LOCK     (1<<0)
#define IRQ_CAPTURE_COMPLETE         (1<<1)
#define IRQ_COMPRESSION_PACKET_READY (1<<2)
#define IRQ_COMPRESSION_COMPLETE     (1<<3)
#define IRQ_MODE_DETECTION_READY     (1<<4)
#define IRQ_FRAME_COMPLETE           (1<<5)
#define ALL_IRQ_ENABLE_BITS (IRQ_WATCHDOG_OUT_OF_LOCK |  \
		IRQ_CAPTURE_COMPLETE | \
		IRQ_COMPRESSION_PACKET_READY | \
		IRQ_COMPRESSION_COMPLETE | \
		IRQ_MODE_DETECTION_READY | \
		IRQ_FRAME_COMPLETE)

/* Defines for Video Interrupt Status Register */
#define STATUS_WATCHDOG_OUT_OF_LOCK     (1<<0)
#define STATUS_CAPTURE_COMPLETE         (1<<1)
#define STATUS_COMPRESSION_PACKET_READY (1<<2)
#define STATUS_COMPRESSION_COMPLETE     (1<<3)
#define STATUS_MODE_DETECTION_READY     (1<<4)
#define STATUS_FRAME_COMPLETE           (1<<5)
#define ALL_IRQ_STATUS_BITS (STATUS_WATCHDOG_OUT_OF_LOCK |  \
		STATUS_CAPTURE_COMPLETE | \
		STATUS_COMPRESSION_PACKET_READY | \
		STATUS_COMPRESSION_COMPLETE | \
		STATUS_MODE_DETECTION_READY | \
		STATUS_FRAME_COMPLETE)

#define MAX_SYNC_CHECK_COUNT        (20)

#define BLOCK_SIZE_YUV444 (0x08)
#define BLOCK_SIZE_YUV420 (0x10)
#define VR060_VIDEO_COMPRESSION_SETTING 0x00080400
char *gtileinfo=NULL;

INTERNAL_MODE Internal_Mode[] = {
	// 1024x768
	{1024, 768, 0, 65},
	{1024, 768, 1, 65},
	{1024, 768, 2, 75},
	{1024, 768, 3, 79},
	{1024, 768, 4, 95},

	// 1280x1024
	{1280, 1024, 0, 108},
	{1280, 1024, 1, 108},
	{1280, 1024, 2, 135},
	{1280, 1024, 3, 158},

	// 1600x1200
	{1600, 1200, 0, 162},
	{1600, 1200, 1, 162},
	{1600, 1200, 2, 176},
	{1600, 1200, 3, 189},
	{1600, 1200, 4, 203},
	{1600, 1200, 5, 230},

	// 1920x1200 reduce blank
	{1920, 1200, 0, 157},
	{1920, 1200, 1, 157},
};

static const unsigned int InternalModeTableSize = (sizeof(Internal_Mode) / sizeof(INTERNAL_MODE));

inline uint32_t ast_videocap_read_reg(uint32_t reg)
{
	return ioread32((void __iomem*)ast_videocap_reg_virt_base + reg);
}

inline void ast_videocap_write_reg(uint32_t data, uint32_t reg)
{
	iowrite32(data, (void __iomem*)ast_videocap_reg_virt_base + reg);
}

inline void ast_videocap_set_reg_bits(uint32_t reg, uint32_t bits)
{
	uint32_t tmp;

	tmp = ast_videocap_read_reg(reg);
	tmp |= bits;
	ast_videocap_write_reg(tmp, reg);
}

inline void ast_videocap_clear_reg_bits(uint32_t reg, uint32_t bits)
{
	uint32_t tmp;

	tmp = ast_videocap_read_reg(reg);
	tmp &= ~bits;
	ast_videocap_write_reg(tmp, reg);
}

int ast_videocap_remap_vga_mem(void)
{
	uint32_t reg;
	unsigned int total_index, vga_index;
	unsigned int vga_mem_size;
	/*reference AST 2300 / 2400 / 2500 / 2600 data sheet
	    VGA Memory Space map to ARM Memory Space
	*/
#if defined(SOC_AST2300) || defined(SOC_AST2400)
	unsigned int vga_mem_address[4][4]={{0x43800000, 0x47800000, 0x4F800000, 0x5F800000},
											{0x43000000, 0x47000000, 0x4F000000, 0x5F000000},
											{0x43000000, 0x46000000, 0x4E000000, 0x5E000000},
											{0x0, 	   0x44000000, 0x4C000000, 0x5C000000}};
#elif defined(SOC_AST2500) || defined(SOC_AST2530) || defined(SOC_AST2600)
	unsigned int vga_mem_address[4][4]={{0x87800000, 0x8F800000, 0x9F800000, 0xBF800000},
											{0x87000000, 0x8F000000, 0x9F000000, 0xBF000000},
											{0x86000000, 0x8E000000, 0x9E000000, 0xBE000000},
											{0x84000000, 0x8C000000, 0x9C000000, 0xBC000000}};
#endif

	/* Total memory size
		AST2050 / AST2150: MMC Configuration Register[3:2]
			00: 32 MB
			01: 64 MB
			10: 128 MB
			11: 256 MB

		AST2300 / AST2400: MMC Configuration Register[1:0]
			00: 64 MB
			01: 128 MB
			10: 256 MB
			11: 512 MB

		AST2500: MMC Configuration Register[1:0]
			00: 128 MB
			01: 256 MB
			10: 512 MB
			11: 1024 MB

		AST2600: MMC Configuration Register[1:0]
			00: 256 MB
			01: 512 MB
			10: 1024 MB
			11: 2048 MB
	*/
#ifdef SOC_AST2600
	reg = ioread32((void * __iomem)(ast_sdram_reg_virt_base + 0x04));
#else
	reg = ioread32((void * __iomem)SDRAM_CONFIG_REG);
#endif
	total_index = (reg & 0x03);

	/* VGA memory size
		Hardware Strapping Register[3:2] definition:
		VGA Aperture Memory Size Setting:
		    00: 8 MB
		    01: 16 MB
		    10: 32 MB
		    11: 64 MB
	*/
#ifdef SOC_AST2600
	/* AST2600 - MR004[1:0] => 00 - 256M
	** AST2500 - MR004[1:0] => 00 - 128M
	** So increase 1 offset */
	if (total_index < 3)
		total_index++;
#else
	reg = ioread32((void * __iomem)SCU_HW_STRAPPING_REG);
#endif

	vga_index = ((reg >> 2) & 0x03);
	vga_mem_size = 0x800000 << ((reg >> 2) & 0x03);

	vga_mem_virt_addr = (void * __iomem)ioremap(vga_mem_address[vga_index][total_index], vga_mem_size);
	if (vga_mem_virt_addr == NULL) {
		printk(KERN_WARNING "ast_videocap: vga_mem_virt_addr ioremap failed\n");
		return -1;
	}

	return 0;
}

void ast_videocap_unmap_vga_mem(void)
{
#ifdef SOC_AST2600
	if (ast_sdram_reg_virt_base != NULL)
		iounmap(ast_sdram_reg_virt_base);

	if (ast_scu_reg_virt_base != NULL)
		iounmap(ast_scu_reg_virt_base);
#endif

	if(vga_mem_virt_addr != NULL)
		iounmap(vga_mem_virt_addr);
}

#if defined(SOC_AST2600)
#define SCU_MISC_CTRL			0xC0
#else
#define SCU_MISC_CTRL			0x2C
#endif
#define SCU_MISC_CTRL_DAC_DISABLE		0x00000008 /* bit 3 */
#if defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2530) || defined(SOC_AST2600)
#define SCU_MULTIFUN_CTRL4      0x8C
#define SCU_MULTIFUN_CTRL4_VPODE_ENABLE 0x00000001 /* bit 0 */
#define SCU_MULTIFUN_CTRL6      0x94
#define SCU_MULTIFUN_CTRL6_DVO_MASK     0x00000003 /* bit 1:0 */
#endif
static int ast_Videocap_vga_dac_status(void)
{
#ifdef SOC_AST2600
	if ((ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MULTIFUN_CTRL6) & SCU_MULTIFUN_CTRL6_DVO_MASK) == 0)
	{
		return (ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL) & SCU_MISC_CTRL_DAC_DISABLE) ? 0 : 1;
	}
	else
	{
		return (ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MULTIFUN_CTRL4) & SCU_MULTIFUN_CTRL4_VPODE_ENABLE) ? 1 : 0;
	}
#else
#if defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2530)
    if ((ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MULTIFUN_CTRL6) & SCU_MULTIFUN_CTRL6_DVO_MASK) == 0) {
        return (ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL) & SCU_MISC_CTRL_DAC_DISABLE) ? 0 : 1;
    } else {
        return (ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MULTIFUN_CTRL4) & SCU_MULTIFUN_CTRL4_VPODE_ENABLE) ? 1 : 0;
    }
#else
	return (ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL) & SCU_MISC_CTRL_DAC_DISABLE) ? 0 : 1;
#endif
#endif
}

static void ast_videocap_vga_dac_ctrl(int enable)
{
	uint32_t reg;

#ifdef SOC_AST2600
	reg = ioread32( (void *__iomem)dp_descriptor_base + 0x100 );
	if (enable){
		reg |= DPTX100;	
	}
	else
	{
		reg &= ~DPTX100;
	}
	iowrite32(reg, (void * __iomem)dp_descriptor_base + 0x100);

	iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	if ((ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MULTIFUN_CTRL6) & SCU_MULTIFUN_CTRL6_DVO_MASK) == 0)
	{
		reg = ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL);
		if (enable)
		{
			reg &= ~SCU_MISC_CTRL_DAC_DISABLE;
		}
		else
		{
			reg |= SCU_MISC_CTRL_DAC_DISABLE;
		}
		iowrite32(reg, (void *__iomem)ast_scu_reg_virt_base + SCU_MISC_CTRL);
	}
	else
	{
		reg = ioread32((void *__iomem)ast_scu_reg_virt_base + SCU_MULTIFUN_CTRL4);
		if (enable)
		{
			reg |= SCU_MULTIFUN_CTRL4_VPODE_ENABLE;
		}
		else
		{
			reg &= ~SCU_MULTIFUN_CTRL4_VPODE_ENABLE;
		}
		iowrite32(reg, (void *__iomem)ast_scu_reg_virt_base + SCU_MULTIFUN_CTRL4);
	}
	iowrite32(0, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* lock SCU */
#else
	iowrite32(0x1688A8A8, (void * __iomem)SCU_KEY_CONTROL_REG); /* unlock SCU */
#if defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2530)
    if ((ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MULTIFUN_CTRL6) & SCU_MULTIFUN_CTRL6_DVO_MASK) == 0) {
        reg = ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL);
        if (enable) {
            reg &= ~SCU_MISC_CTRL_DAC_DISABLE;
        } else {
            reg |= SCU_MISC_CTRL_DAC_DISABLE;
        }
        iowrite32(reg, (void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL);
    } else {
        reg = ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MULTIFUN_CTRL4);
        if (enable) {
            reg |= SCU_MULTIFUN_CTRL4_VPODE_ENABLE;
        } else {
            reg &= ~SCU_MULTIFUN_CTRL4_VPODE_ENABLE;
        }
        iowrite32(reg, (void * __iomem)AST_SCU_VA_BASE + SCU_MULTIFUN_CTRL4);
    }
#else
	reg = ioread32((void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL);
	if (enable) {
		reg &= ~SCU_MISC_CTRL_DAC_DISABLE;
	} else {
		reg |= SCU_MISC_CTRL_DAC_DISABLE;
	}
	iowrite32(reg, (void * __iomem)AST_SCU_VA_BASE + SCU_MISC_CTRL);
#endif
	iowrite32(0, (void * __iomem)SCU_KEY_CONTROL_REG); /* lock SCU */
#endif
}

void ast_videocap_engine_data_init(void)
{
	ModeDetectionIntrRecd = 0;
	CaptureIntrRecd = 0;
	CompressionIntrRecd = 0;
	AutoModeIntrRecd = 0;

	WaitingForModeDetection = 0;
	WaitingForCapture = 0;
	WaitingForCompression = 0;

	ast_videocap_data_in_old_video_buf = 0;

	prev_cursor_pattern_addr = NULL;
	prev_cursor_status = 0;
	prev_cursor_position = 0;
	prev_cursor_pos_x = 0;
	prev_cursor_pos_y = 0;
	prev_cursor_checksum = 0;

	memset((void *) &ast_videocap_engine_info, 0, sizeof(struct ast_videocap_engine_info_t));
	ast_videocap_engine_info.FrameHeader.CompressionMode = COMPRESSION_MODE;
	ast_videocap_engine_info.FrameHeader.JPEGTableSelector = JPEG_TABLE_SELECTOR;
	ast_videocap_engine_info.FrameHeader.AdvanceTableSelector = ADVANCE_TABLE_SELECTOR;
	
	init_waitqueue_head(&autowq);
}

irqreturn_t ast_videocap_irq_handler(int irq, void *dev_id)
{
	uint32_t status;

	status = ast_videocap_read_reg(AST_VIDEOCAP_ISR);
	ast_videocap_write_reg(status, AST_VIDEOCAP_ISR); // clear interrupts flags

	/* Mode Detection Watchdog Interrupt */
	if (status & STATUS_WATCHDOG_OUT_OF_LOCK) {
		ISRDetectedModeOutOfLock = 1;
	}

	/* Mode Detection Ready */
	if (status & STATUS_MODE_DETECTION_READY) {
		ModeDetectionIntrRecd = 1;
		wake_up_interruptible(&mdwq);
	}

	/* Capture Engine Idle */
	if (status & STATUS_CAPTURE_COMPLETE) {
		CaptureIntrRecd = 1;
		if (!AUTO_MODE) {
			wake_up_interruptible(&capwq);
		}
	}

	/* Compression Engine Idle */
	if (status & STATUS_COMPRESSION_COMPLETE) {
		CompressionIntrRecd = 1;
		if (!AUTO_MODE) {
			wake_up_interruptible(&compwq);
		}
	}

	if ((ISRDetectedModeOutOfLock == 1) || ((CaptureIntrRecd == 1) && (CompressionIntrRecd == 1))) {
		CaptureIntrRecd = 0;
		CompressionIntrRecd = 0;
        AutoModeIntrRecd = 1;
        if (AUTO_MODE) {
        	wake_up_interruptible (&autowq);
        }
    }
	
	return IRQ_HANDLED;
}

void ast_videocap_reset_hw(void)
{
	uint32_t reg;
#ifdef SOC_AST2600
	iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	/* enable reset video engine - SCU40[6] = 1 */
	reg = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x40);
	reg |= 0x00000040;
	iowrite32(reg, (void *__iomem)ast_scu_reg_virt_base + 0x40);
	udelay(100);
	/* enable video engine clock - SCU080, use SCU084 to set */
	//reg = ioread32((void * __iomem)ast_scu_reg_virt_base + 0x80);
	//reg &= ~(0x0000002E);
	//Reset ECLK(Video Engine),GCLK(2D Engine),VCLK(Video Capture Engine), DCLK(DAC)
	iowrite32(0x0000002E, (void * __iomem)ast_scu_reg_virt_base + 0x84);

	/* Clear SCU300 30:28 to fix kvm noise issue */
	reg = ioread32((void * __iomem)ast_scu_reg_virt_base + 0x300);
	reg &= ~(0x70000000);
	iowrite32(reg, (void * __iomem)ast_scu_reg_virt_base + 0x300);

	wmb();
	mdelay(10);
	/* disable reset video engine - SCU44[6] = 1 (A1 datasheet v0.6) */
	iowrite32(0x00000040, (void *__iomem)ast_scu_reg_virt_base + 0x44);
	/* support wide screen resolution */
	// SCU40 -> SCU100
	reg = ioread32((void * __iomem)ast_scu_reg_virt_base + 0x100);
	reg |= (0x00000001);
	iowrite32(reg, (void * __iomem)ast_scu_reg_virt_base + 0x100);
	iowrite32(0, (void * __iomem)ast_scu_reg_virt_base); /* lock SCU */
	// reset_control_assert(reset);
	// udelay(100);
	// reset_control_deassert(reset);
#else
	iowrite32(0x1688A8A8, (void * __iomem)SCU_KEY_CONTROL_REG); /* unlock SCU */

	/* enable reset video engine */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg |= 0x00000040;
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);

	udelay(100);

	/* enable video engine clock */
	reg = ioread32((void * __iomem)SCU_CLK_STOP_REG);
	reg &= ~(0x0000002B);
	iowrite32(reg, (void * __iomem)SCU_CLK_STOP_REG);

	wmb();

	mdelay(10);

	/* disable reset video engine */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg &= ~(0x00000040);
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);

	#if defined(SOC_AST2300) || defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2530)
	/* support wide screen resolution */
	reg = ioread32((void * __iomem)SCU_SOC_SCRATCH1_REG);
	reg |= (0x00000001);
	iowrite32(reg, (void * __iomem)SCU_SOC_SCRATCH1_REG);
	#endif

	iowrite32(0, (void * __iomem)SCU_KEY_CONTROL_REG); /* lock SCU */
#endif
}


void ast_videocap_hw_init(void)
{
	uint32_t reg;
#ifndef SOC_AST2600
	iowrite32(0x1688A8A8, (void * __iomem)SCU_KEY_CONTROL_REG); /* unlock SCU */

#if defined(SOC_AST2500) || defined(SOC_AST2530)
#define SCU_CLK_VIDEO_SLOW_MASK     (0x7 << 28)
#define SCU_ECLK_SOURCE_MASK        (0x3 << 2)
	reg = ioread32((void * __iomem)SCU_CLK_SELECT_REG);
	// Enable Clock & ECLK = inverse of (M-PLL / 2)
	reg &= ~(SCU_ECLK_SOURCE_MASK | SCU_CLK_VIDEO_SLOW_MASK);
	iowrite32(reg, (void * __iomem)SCU_CLK_SELECT_REG);
#endif

	/* enable reset video engine */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg |= 0x00000040;
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);

	udelay(100);

	/* enable video engine clock */
	reg = ioread32((void * __iomem)SCU_CLK_STOP_REG);
	reg &= ~(0x0000002B);
	iowrite32(reg, (void * __iomem)SCU_CLK_STOP_REG);

	wmb();

	mdelay(10);

	/* disable reset video engine */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg &= ~(0x00000040);
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);

	iowrite32(0, (void * __iomem)SCU_KEY_CONTROL_REG); /* lock SCU */
#else
	// reset_control_assert(reset);
	// udelay(100);
	// reset_control_deassert(reset);
	iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	reg = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x40);
	reg |= 0x00000040; /* enable reset video engine - SCU40[6] = 1 */
	iowrite32(reg, (void *__iomem)ast_scu_reg_virt_base + 0x40);
	udelay(100);

	/* enable video engine clock - SCU080, use SCU084 to set */
	//reg = ioread32((void * __iomem)ast_scu_reg_virt_base + 0x80);
	//reg &= ~(0x0000002E);
	//Reset ECLK(Video Engine),GCLK(2D Engine),VCLK(Video Capture Engine), DCLK(DAC)
	iowrite32(0x0000002E, (void * __iomem)ast_scu_reg_virt_base + 0x84);

	/* Clear SCU300 30:28 to fix kvm noise issue */
	reg = ioread32((void * __iomem)ast_scu_reg_virt_base + 0x300);
	reg &= ~(0x70000000);
	iowrite32(reg, (void * __iomem)ast_scu_reg_virt_base + 0x300);

	wmb();
	mdelay(10);
	/* disable reset video engine - SCU44[6] = 1 (A1 datasheet v0.6) */
	iowrite32(0x00000040, (void *__iomem)ast_scu_reg_virt_base + 0x44);
	iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* lock SCU */
#endif
	/* unlock video engine */
	udelay(100);
	ast_videocap_write_reg(AST_VIDEOCAP_KEY_MAGIC, AST_VIDEOCAP_KEY);

	/* clear interrupt status */
	ast_videocap_write_reg(0xFFFFFFFF, AST_VIDEOCAP_ISR);
	ast_videocap_write_reg(ALL_IRQ_STATUS_BITS, AST_VIDEOCAP_ISR);

	/* enable interrupt */
	reg = IRQ_WATCHDOG_OUT_OF_LOCK | IRQ_CAPTURE_COMPLETE | IRQ_COMPRESSION_COMPLETE | IRQ_MODE_DETECTION_READY;
	ast_videocap_write_reg(reg, AST_VIDEOCAP_ICR);

	ast_videocap_write_reg(0, AST_VIDEOCAP_COMPRESS_STREAM_BUF_READ_OFFSET);

	/* reset RC4 */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL2, 0x00000100);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL2, 0x00000100);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL2, 0x00000100);

	//ast_videocap_vga_dac_ctrl(1);

	/* clear flag buffer */
	memset(AST_VIDEOCAP_FLAG_BUF_ADDR, 0x1, AST_VIDEOCAP_FLAG_BUF_SZ);
	ast_videocap_data_in_old_video_buf = 0;
}

void StopVideoCapture(void)
{
}

void ProcessTimeOuts(unsigned char Type)
{

	if (Type == NO_TIMEOUT) {
		return;
	}

	if (Type == SPURIOUS_TIMEOUT) {
		SpuriousTimeouts ++;
		return;
	}

	TimeoutCount ++;
	ast_videocap_data_in_old_video_buf = 0;

	if (Type == MODE_DET_TIMEOUT) {
		ModeDetTimeouts ++;
		return;
	}

	if (Type == CAPTURE_TIMEOUT) {
		CaptureTimeouts ++;
		ReInitVideoEngine = 1;
		return;
	}

	if (Type == COMPRESSION_TIMEOUT) {
		CompressionTimeouts ++;
		ReInitVideoEngine = 1;
		return;
	}
	
	if (Type == AUTOMODE_TIMEOUT) {
		AutoModeTimeouts ++;
		ReInitVideoEngine = 1;
		return;
	}
}

/* return 0 on succeeds, -1 on times out */
int ast_videocap_trigger_mode_detection(void)
{
	init_waitqueue_head(&mdwq);
	ModeDetectionIntrRecd = 0;
	TimeToWait = VE_TIMEOUT * HZ;

	/* trigger mode detection by rising edge(0 -> 1) */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_MODE_DETECT);
	wmb();
	udelay(100);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_MODE_DETECT);

	if (wait_event_interruptible_timeout(mdwq, ModeDetectionIntrRecd, TimeToWait) != 0) {
		WaitingForModeDetection = 0;
	} else { /* timeout */
		if (ModeDetectionIntrRecd == 0) {
		    ProcessTimeOuts(MODE_DET_TIMEOUT);
		    return -1; /* Possibly out of lock or invalid input, we should send a blank screen */
		} else {
		    /* ModeDetectionIntrRecd is set, but timeout triggered - should never happen!! */
		    ProcessTimeOuts(SPURIOUS_TIMEOUT);
		    /* Not much to do here, just continue on */
		}
	}

	return 0;
}

/* Return 0 if Capture Engine becomes idle, -1 if engine remains busy */
int ast_videocap_trigger_capture(void)
{
	init_waitqueue_head(&capwq);
	TimeToWait = VE_TIMEOUT*HZ;
	CaptureIntrRecd = 0;

	if (!(ast_videocap_read_reg(AST_VIDEOCAP_SEQ_CTRL) & AST_VIDEOCAP_SEQ_CTRL_CAP_STATUS)) {
		return -1; /* capture engine remains busy */
	}

	/* trigger capture */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_IDLE);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_CAP);
	udelay(100);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_CAP);

	WaitingForCapture = 1;
	if (wait_event_interruptible_timeout(capwq, CaptureIntrRecd, TimeToWait) != 0) {
		WaitingForCapture = 0;
	} else {
		/* Did Video Engine timeout? */
		if (CaptureIntrRecd == 0) {
			ProcessTimeOuts(CAPTURE_TIMEOUT);
			return -1; /* Capture Engine remains busy */
		} else {
			/* CaptureIntrRecd is set, but timeout triggered - should never happen!! */
			ProcessTimeOuts(SPURIOUS_TIMEOUT);
			/* Not much to do here, just continue on */
		}
	}

	return 0;
}

/* Return 0 if Compression becomes idle, -1 if engine remains busy */
int ast_videocap_trigger_compression(void)
{
	init_waitqueue_head(&compwq);
	TimeToWait = VE_TIMEOUT*HZ;
	CompressionIntrRecd = 0;

	if (!(ast_videocap_read_reg(AST_VIDEOCAP_SEQ_CTRL) & AST_VIDEOCAP_SEQ_CTRL_COMPRESS_STATUS)) {
		return -1; /* compression engine remains busy */
	}

	/* trigger compression */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_IDLE);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_CAP);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS);
	udelay(100);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS);

	WaitingForCompression = 1;
	if (wait_event_interruptible_timeout(compwq, CompressionIntrRecd, TimeToWait) != 0) {
		WaitingForCompression = 0;
	} else {
		/* Did Video Engine timeout? */
		if (CompressionIntrRecd == 0) {
			ProcessTimeOuts(COMPRESSION_TIMEOUT);
			return -1; /* Compression remains busy */
		} else {
			/* CompressionIntrRecd is set, but timeout triggered - should never happen!! */
			ProcessTimeOuts(SPURIOUS_TIMEOUT);
			/* Not much to do here, just continue on */
		}
	}

	return 0;
}

int ast_videocap_automode_trigger(void)
{
	unsigned int status;

	TimeToWait = VE_TIMEOUT*HZ;
	AutoModeIntrRecd = 0;

	if (!(ast_videocap_read_reg(AST_VIDEOCAP_SEQ_CTRL) & AST_VIDEOCAP_SEQ_CTRL_CAP_STATUS)) {
		return -1; /* capture engine remains busy */
	}
	if (!(ast_videocap_read_reg(AST_VIDEOCAP_SEQ_CTRL) & AST_VIDEOCAP_SEQ_CTRL_COMPRESS_STATUS)) {
		return -1; /* compression engine remains busy */
	}

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_IDLE);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_CAP);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS); 
	barrier();

    status = ast_videocap_read_reg(AST_VIDEOCAP_SEQ_CTRL);
    ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, ((status & 0xFFFFFFED) | 0x12));

    WaitingForAutoMode = 1;
    if (wait_event_interruptible_timeout(autowq, AutoModeIntrRecd, TimeToWait) != 0) {
    	WaitingForAutoMode = 0;
	 } else {
	 	if (AutoModeIntrRecd == 0) {
	 		ProcessTimeOuts(AUTOMODE_TIMEOUT);
			return -1;
		} else {			
			ProcessTimeOuts(SPURIOUS_TIMEOUT);
			/* Not much to do here, just continue on */
		}
	 }
	 

	 return 0;
}

void ast_videocap_determine_syncs(unsigned long *hp, unsigned long *vp)
{
	int i = 0;
	int hPcount = 0;
	int vPcount = 0;

	*vp = 0;
	*hp = 0;

	for (i = 0; i < MAX_SYNC_CHECK_COUNT; i ++) {
		if (ast_videocap_read_reg(AST_VIDEOCAP_MODE_DETECT_STATUS) & 0x20000000) // Hsync polarity is positive
		    hPcount ++;
		if (ast_videocap_read_reg(AST_VIDEOCAP_MODE_DETECT_STATUS) & 0x10000000) // Vsync polarity is positive
		    vPcount ++;
	}

	/* Majority readings indicate positive? Then it must be positive.*/
	if (hPcount > (MAX_SYNC_CHECK_COUNT / 2)) {
		*hp = 1;
	}

	if (vPcount > (MAX_SYNC_CHECK_COUNT / 2)) {
		*vp = 1;
	}
}

int ast_videocap_auto_position_adjust(struct ast_videocap_engine_info_t *info)
{
	uint32_t reg;
	unsigned long __maybe_unused H_Start, H_End, V_Start, V_End, i, H_Temp = 0;
//	unsigned long  V_Temp = 0;
	unsigned long __maybe_unused Mode_PixelClock = 0, mode_bandwidth, refresh_rate_index, color_depth_index;
	unsigned long __maybe_unused color_depth, mode_clock;

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0x0000FF00);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0x65 << 8);

Redo:
	ast_videocap_set_reg_bits(AST_VIDEOCAP_ISR, 0x00000010);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_MODE_DETECT);

	if (ast_videocap_trigger_mode_detection() < 0) {
		/*  Possibly invalid input, we should send a blank screen   */
		ISRDetectedModeOutOfLock = 1;
		return -1;
	}

	H_Start = ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_H) & AST_VIDEOCAP_EDGE_DETECT_START_MASK;
	H_End = (ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_H) & AST_VIDEOCAP_EDGE_DETECT_END_MASK) >> AST_VIDEOCAP_EDGE_DETECT_END_SHIFT;
	V_Start = ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_V) & AST_VIDEOCAP_EDGE_DETECT_START_MASK;
	V_End = (ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_V) & AST_VIDEOCAP_EDGE_DETECT_END_MASK) >> AST_VIDEOCAP_EDGE_DETECT_END_SHIFT;

	//If any unsupported frame size comes, we should skip processing it.
	if ((((H_End - H_Start) + 1) > AST_VIDEOCAP_MAX_FRAME_WIDTH) ||
		( ((V_End - V_Start) + 1) > AST_VIDEOCAP_MAX_FRAME_HEIGHT) ) {
			/*  Possibly invalid input, we should send a blank screen   */
			ISRDetectedModeOutOfLock = 1;
			return -1;				
	}

	//Check if cable quality is too bad. If it is bad then we use 0x65 as threshold
	//Because RGB data is arrived slower than H-sync, V-sync. We have to read more times to confirm RGB data is arrived
	if ((abs(H_Temp - H_Start) > 1) || ((H_Start <= 1) || (V_Start <= 1) || (H_Start == 0x3FF) || (V_Start == 0x3FF))) {
		H_Temp = ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_H) & AST_VIDEOCAP_EDGE_DETECT_START_MASK;
		//V_Temp = ast_videocap_read_reg(AST_VIDEOCAP_EDGE_DETECT_V) & AST_VIDEOCAP_EDGE_DETECT_START_MASK;
		//printk("REDO\n");
		//printk("Data = %x\n", ReadMMIOLong (MODE_DETECTION_REGISTER));
		//printk("H_Start = %x, H_End = %x\n", H_Start, H_End);
		//printk("V_Start = %x, V_End = %x\n", V_Start, V_End);
		goto Redo;
	}

	//printk("H_Start = %lx, H_End = %lx\n", H_Start, H_End);
	//printk("V_Start = %lx, V_End = %lx\n", V_Start, V_End);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_TIMING_GEN_H, AST_VIDEOCAP_TIMING_GEN_LAST_PIXEL_MASK);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_TIMING_GEN_H, H_End);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_TIMING_GEN_H, AST_VIDEOCAP_TIMING_GEN_FIRST_PIXEL_MASK);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_TIMING_GEN_H, (H_Start - 1) << AST_VIDEOCAP_TIMING_GEN_FIRST_PIXEL_SHIFT);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_TIMING_GEN_V, AST_VIDEOCAP_TIMING_GEN_LAST_PIXEL_MASK);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_TIMING_GEN_V, V_End + 1);

	ast_videocap_clear_reg_bits(AST_VIDEOCAP_TIMING_GEN_V, AST_VIDEOCAP_TIMING_GEN_FIRST_PIXEL_MASK);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_TIMING_GEN_V, (V_Start) << AST_VIDEOCAP_TIMING_GEN_FIRST_PIXEL_SHIFT);

	info->src_mode.x = (H_End - H_Start) + 1;
	info->src_mode.y = (V_End - V_Start) + 1;

	info->dest_mode.x = info->src_mode.x;
	info->dest_mode.y = info->src_mode.y;

	printk("Mode Detected: %d x %d\n", info->src_mode.x, info->src_mode.y);

	//Set full screen flag to avoid bcd calculation and re-capture
	if(CaptureMode == VIDEOCAP_JPEG_SUPPORT) 
		full_screen_capture =1;

	//  Judge if bandwidth is not enough then enable direct mode in internal VGA

	reg = ast_videocap_read_reg(AST_VIDEOCAP_VGA_SCRATCH_9093);
	if (((reg & 0xff00) >> 8) == 0xA8) { // Driver supports to get display information in scratch register
		color_depth = (reg & 0xff0000) >> 16; //VGA's Color Depth is 0 when real color depth is less than 8
		mode_clock = (reg & 0xff000000) >> 24;
#ifdef SOC_AST2600 //Ref: (sdk7.1) drivers/soc/aspeed/ast_video.c > ast_video_vga_mode_detect()
		if (color_depth < 15) { /* Disable direct mode for less than 15 bpp */
			printk("Color Depth is less than 15 bpp\n");
			info->INFData.DirectMode = 0;
		}
		else /* Enable direct mode for 15 bpp and higher */
		{
			printk("Color Depth is 15 bpp or higher\n");
			info->INFData.DirectMode = 1;
		}
#else
		if (color_depth == 0) {
			printk("Color Depth is not 15 bpp or higher\n");
			info->INFData.DirectMode = 0;
	    } else {
			mode_bandwidth = (mode_clock * (color_depth + 32)) / 8; //Video uses 32bits
			if (info->MemoryBandwidth < mode_bandwidth) {
				info->INFData.DirectMode = 1;
			} else {
				info->INFData.DirectMode = 0;
			}
		}
#endif
	} else { /* old driver, get display information from table */
		refresh_rate_index = (ast_videocap_read_reg(AST_VIDEOCAP_VGA_SCRATCH_8C8F) >> 8) & 0x0F;
		color_depth_index = (ast_videocap_read_reg(AST_VIDEOCAP_VGA_SCRATCH_8C8F) >> 4) & 0x0F;
#ifdef SOC_AST2600 //Ref: (sdk7.1) drivers/soc/aspeed/ast_video.c > ast_video_vga_mode_detect()
		/* To fix AST2600 A0 Host VGA output tearing / noisy issue, ASPEED modified MCR74[31], MCR74[0] as 1.
		** This change caused DRAM shortage side effect in USB controller (A1 and higher).
		**
		** USB bandwidth is increased (MCR44 Maximum Grant Length) to fix DRAM shortage issue.
		** but it caused abnormal video screen with AST2600 A1/A2/A3 as side effect.
		**
		** To mitigate this abnormal video issue aspeed suggested to use direct fetch mode
		** for host resolution >= 1024x768. */
		if ((color_depth_index == 0xe) || (color_depth_index == 0xf)) { /* Disable direct mode for 14, 15 bpp */
			info->INFData.DirectMode = 0;
		} else { /* enable direct mode for host resolution >= 1024x768 */
			if (color_depth_index > 2) {
				if ((info->src_mode.x * info->src_mode.y) < (1024 * 768))
					info->INFData.DirectMode = 0;
				else
					info->INFData.DirectMode = 1;
			} else {
				printk("Color Depth is less than 15bpp\n");
				info->INFData.DirectMode = 0;
			}
		}
#else
		if ((color_depth_index == 2) || (color_depth_index == 3) || (color_depth_index == 4)) { // 15 bpp /16 bpp / 32 bpp
			for (i = 0; i < InternalModeTableSize; i ++) { /* traverse table to find mode */
				if ((info->src_mode.x == Internal_Mode[i].HorizontalActive) &&
					(info->src_mode.y == Internal_Mode[i].VerticalActive) &&
					(refresh_rate_index == Internal_Mode[i].RefreshRateIndex)) {
					Mode_PixelClock = Internal_Mode[i].PixelClock;
					//printk("Mode_PixelClock = %ld\n", Mode_PixelClock);
					break;
				}
			}

			if (i == InternalModeTableSize) {
				printk("videocap: No match mode\n");
			}

			//  Calculate bandwidth required for this mode
			//  Video requires pixelclock * 4, VGA requires pixelclock * bpp / 8
			mode_bandwidth = Mode_PixelClock * (4 + 2 * (color_depth_index - 2));
			//printk("mode_bandwidth = %ld\n", mode_bandwidth);
			if (info->MemoryBandwidth < mode_bandwidth) {
				info->INFData.DirectMode = 1;
			} else {
				info->INFData.DirectMode = 0;
			}
		} else {
			printk("Color Depth is not 15bpp or higher\n");
			info->INFData.DirectMode = 0;
		}
#endif
	}

	return 0;
}

int ast_videocap_mode_detection(struct ast_videocap_engine_info_t *info, bool in_video_capture)
{
	unsigned long HPolarity, VPolarity;
	uint32_t vga_status;

	ast_videocap_hw_init();

	/* Check if internal VGA screen is off */
	vga_status = ast_videocap_read_reg(AST_VIDEOCAP_VGA_STATUS) >> 24;
	if ((vga_status == 0x10) || (vga_status == 0x11) || (vga_status & 0xC0) || (vga_status & 0x04) || (!(vga_status & 0x3B))) {
		info->NoSignal = 1;
	} else {
		info->NoSignal = 0;
	}

	/* video source from internal VGA */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_SRC);

	/* Set VideoClockPhase Selection for Internal Video */
	//ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, 0x00000A00);

	/* polarity is same as source */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_HSYNC_POLARITY | AST_VIDEOCAP_CTRL_VSYNC_POLARITY);

	/* Set Stable Parameters */
	/*  VSync Stable Max*/
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0x000F0000);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, MODEDETECTION_VERTICAL_STABLE_MININUM << 16);

	/*  HSync Stable Max*/
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0x00F00000);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, MODEDETECTION_HORIZONTAL_STABLE_MININUM << 20);

	/*  VSync counter threshold */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0x0F000000);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, MODEDETECTION_VERTICAL_STABLE_THRESHOLD << 24);

	/*  HSync counter threshold */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, 0xF0000000);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_MODE_DETECT_PARAM, MODEDETECTION_HORIZONTAL_STABLE_THRESHOLD << 28);

	/* Disable Watchdog */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_WDT | AST_VIDEOCAP_SEQ_CTRL_MODE_DETECT);

	/* New Mode Detection procedure prescribed by ASPEED:
	 *
	 * Trigger mode detection once
	 * After that succeeds, figure out the polarities of HSync and VSync
	 * Program the right polarities...
	 * Then Trigger mode detection again
	 * Now with the new values, use JudgeMode to figure out the correct mode
	 */

	/* Trigger first detection, and check for timeout? */
	if (ast_videocap_trigger_mode_detection() < 0) {
		/*  Possibly invalid input, we should send a blank screen   */
		ISRDetectedModeOutOfLock = 1;
		return -1;
	}

	/* Now, determine sync polarities and program appropriately */
	ast_videocap_determine_syncs(&HPolarity, &VPolarity);

	if (info->INFData.DirectMode == 1) {
		/* Direct mode always uses positive/positive so we inverse them */
		ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_HSYNC_POLARITY | AST_VIDEOCAP_CTRL_VSYNC_POLARITY);
	} else {
		ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, (VPolarity << 1) | HPolarity);
	}

	/* Disable Watchdog */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_WDT | AST_VIDEOCAP_SEQ_CTRL_MODE_DETECT);

	/* Trigger second detection, and check for timeout? */
	if (ast_videocap_trigger_mode_detection() < 0) {
		/*  Possibly invalid input, we should send a blank screen */
		ISRDetectedModeOutOfLock = 1;
		return -1;
	}

	vga_status = ast_videocap_read_reg(AST_VIDEOCAP_VGA_STATUS) >> 24;
	if ((vga_status == 0x10) || (vga_status == 0x11) || (vga_status & 0xC0) || (vga_status & 0x04) || (!(vga_status & 0x3B))) {
		info->NoSignal = 1;
		udelay(100);
	} else {
		info->NoSignal = 0;
	}

	/* Enable Watchdog detection for next time */
	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_WDT);

	ISRDetectedModeOutOfLock = 0;

    if (in_video_capture == 1)
    {
		ast_videocap_auto_position_adjust(info);
    }
    else if (info->NoSignal == 0) {
		ast_videocap_auto_position_adjust(info);
	}


	return 0;
}

// Set defaults for user settings
void InitDefaultSettings(struct ast_videocap_engine_info_t *info)
{
	info->FrameHeader.JPEGYUVTableMapping = JPEG_YUVTABLE_MAPPING;
	info->FrameHeader.JPEGScaleFactor = JPEG_SCALE_FACTOR;
	info->FrameHeader.AdvanceScaleFactor = ADVANCE_SCALE_FACTOR;
	info->FrameHeader.SharpModeSelection = SHARP_MODE_SELECTION;
	info->INFData.DownScalingMethod = DOWN_SCALING_METHOD;
	info->INFData.DifferentialSetting = DIFF_SETTING;
	info->FrameHeader.RC4Enable = RC4_ENABLE;
	info->FrameHeader.RC4Reset = RC4_RESET;
	info->INFData.AnalogDifferentialThreshold = ANALOG_DIFF_THRESHOLD;
	info->INFData.DigitalDifferentialThreshold = DIGITAL_DIFF_THRESHOLD;
	info->INFData.ExternalSignalEnable = EXTERNAL_SIGNAL_ENABLE;
	info->INFData.AutoMode = AUTO_MODE;

	memcpy(EncodeKeys, ENCODE_KEYS, ENCODE_KEYS_LEN);
}

int ast_videocap_get_engine_config(struct ast_videocap_engine_info_t *info, struct ast_videocap_engine_config_t *config, unsigned long *size)
{
	config->differential_setting = info->INFData.DifferentialSetting;
	config->dct_quant_quality = info->FrameHeader.JPEGScaleFactor;
	config->dct_quant_tbl_select = info->FrameHeader.JPEGTableSelector;
	config->sharp_mode_selection = info->FrameHeader.SharpModeSelection;
	config->sharp_quant_quality = info->FrameHeader.AdvanceScaleFactor;
	config->sharp_quant_tbl_select = info->FrameHeader.AdvanceTableSelector;
	config->compression_mode = info->FrameHeader.CompressionMode;
	config->vga_dac = ast_Videocap_vga_dac_status();
	*size = sizeof(struct ast_videocap_engine_config_t);
	return 0;
}

int ast_videocap_set_engine_config(struct ast_videocap_engine_info_t *info, struct ast_videocap_engine_config_t *config)
{
	ast_videocap_data_in_old_video_buf = 0; /* configurations are changed, send a full screen next time */

	info->INFData.DifferentialSetting = config->differential_setting;
	info->FrameHeader.JPEGScaleFactor = config->dct_quant_quality;
	info->FrameHeader.JPEGTableSelector = config->dct_quant_tbl_select;
	info->FrameHeader.SharpModeSelection = config->sharp_mode_selection;
	info->FrameHeader.AdvanceScaleFactor = config->sharp_quant_quality;
	info->FrameHeader.AdvanceTableSelector = config->sharp_quant_tbl_select;
	info->FrameHeader.CompressionMode = config->compression_mode;

	if (config->vga_dac == 0) { /* turn off VGA output */
		ast_videocap_vga_dac_ctrl(0);
	} else { /* turn on VGA output */
		ast_videocap_vga_dac_ctrl(1);
	}

	return 0;
}

int ast_videocap_enable_video_dac(struct ast_videocap_engine_info_t *info, struct ast_videocap_engine_config_t *config)
{
	ast_videocap_data_in_old_video_buf = 0; /* configurations are changed, send a full screen next time */
//	info->INFData.DifferentialSetting = config->differential_setting;
//	info->FrameHeader.JPEGScaleFactor = config->dct_quant_quality;
//	info->FrameHeader.JPEGTableSelector = config->dct_quant_tbl_select;
//	info->FrameHeader.SharpModeSelection = config->sharp_mode_selection;
//	info->FrameHeader.AdvanceScaleFactor = config->sharp_quant_quality;
//	info->FrameHeader.AdvanceTableSelector = config->sharp_quant_tbl_select;
//	info->FrameHeader.CompressionMode = config->compression_mode;
    config->vga_dac=1;
	ast_videocap_vga_dac_ctrl(1);
	return 0;
}

#ifdef SOC_AST2600
#ifdef SCU_M_PLL_PARAM_REG
#undef SCU_M_PLL_PARAM_REG
#endif
#define SCU_M_PLL_PARAM_REG 0x220
#endif

void ast_videocap_get_mem_bandwidth(struct ast_videocap_engine_info_t *info)
{
	uint32_t reg;
	unsigned long Numerator, Denumerator, PostDivider, OutputDivider, Buswidth = 16, MemoryClock;
#ifdef SOC_AST2600
	reg = ioread32((void * __iomem)ast_scu_reg_virt_base + SCU_M_PLL_PARAM_REG);
	Numerator = (reg & 0x1FFF); // [12:0]
	Denumerator = ((reg >> 13) & 0x3F); // [18:13]
	OutputDivider = 0; // unused
	PostDivider = ((reg >> 19) & 0xF); // [22:19]
	MemoryClock = 25 * (( Numerator + 1 ) / ( Denumerator + 1 )) / ( PostDivider + 1 );
	info->MemoryBandwidth = ((MemoryClock * 2 * Buswidth * 6) / 10) / 8;
#else
	reg = ioread32((void * __iomem)SCU_M_PLL_PARAM_REG);

	/* We discard M-PLL bypass and Turn-Off M-PLL mode */
	Numerator = ((reg >> 5) & 0x3F);
	Denumerator = (reg & 0xF);
	OutputDivider = ((reg >> 4) & 0x1); /* Fortify issue :: False Positive */
	switch ((reg >> 12) & 0x7) {
	case 0x04: /* 100 */
		PostDivider = 2;
		break;
	case 0x05: /* 101 */
		PostDivider = 4;
		break;
	case 0x06: /* 110 */
		PostDivider = 8;
		break;
	case 0x07: /* 111 */
		PostDivider = 16;
		break;
	default:
		PostDivider = 1;
		break;
	}


	#if defined(SOC_AST2300) || defined(SOC_AST2400)
	MemoryClock = (24 * (2 - OutputDivider) * (Numerator + 2) / (Denumerator + 1)) / PostDivider;
	info->MemoryBandwidth = ((MemoryClock * 2 * Buswidth * 4) / 10) / 8;
	#else
	MemoryClock = 24 * (( Numerator + 1 ) / ( Denumerator + 1 )) / ( PostDivider + 1 );
	info->MemoryBandwidth = ((MemoryClock * 2 * Buswidth * 6) / 10) / 8;
	#endif
	//printk("Memory Clock = %ld\n", MemoryClock);
	//printk("Memory Bandwidth = %d\n", info->MemoryBandwidth);
#endif
}

void ast_init_jpeg_table()
{
	int i=0;
	int base=0;

	//JPEG header default value:
	for(i = 0; i<12; i++) {
		base = (1024*i);
		JPEGHeaderTable[base + 0] = 0xE0FFD8FF;
		JPEGHeaderTable[base + 1] = 0x464A1000;
		JPEGHeaderTable[base + 2] = 0x01004649;
		JPEGHeaderTable[base + 3] = 0x60000101;
		JPEGHeaderTable[base + 4] = 0x00006000;
		JPEGHeaderTable[base + 5] = 0x0F00FEFF;
		JPEGHeaderTable[base + 6] = 0x00002D05;
		JPEGHeaderTable[base + 7] = 0x00000000;
		JPEGHeaderTable[base + 8] = 0x00000000;
		JPEGHeaderTable[base + 9] = 0x00DBFF00;
		JPEGHeaderTable[base + 44] = 0x081100C0;
		JPEGHeaderTable[base + 45] = 0x00000000;
		JPEGHeaderTable[base + 46] = 0x00110103;
		JPEGHeaderTable[base + 47] = 0x03011102;
		JPEGHeaderTable[base + 48] = 0xC4FF0111;
		JPEGHeaderTable[base + 49] = 0x00001F00;
		JPEGHeaderTable[base + 50] = 0x01010501;
		JPEGHeaderTable[base + 51] = 0x01010101;
		JPEGHeaderTable[base + 52] = 0x00000000;
		JPEGHeaderTable[base + 53] = 0x00000000;
		JPEGHeaderTable[base + 54] = 0x04030201;
		JPEGHeaderTable[base + 55] = 0x08070605;
		JPEGHeaderTable[base + 56] = 0xFF0B0A09;
		JPEGHeaderTable[base + 57] = 0x10B500C4;
		JPEGHeaderTable[base + 58] = 0x03010200;
		JPEGHeaderTable[base + 59] = 0x03040203;
		JPEGHeaderTable[base + 60] = 0x04040505;
		JPEGHeaderTable[base + 61] = 0x7D010000;
		JPEGHeaderTable[base + 62] = 0x00030201;
		JPEGHeaderTable[base + 63] = 0x12051104;
		JPEGHeaderTable[base + 64] = 0x06413121;
		JPEGHeaderTable[base + 65] = 0x07615113;
		JPEGHeaderTable[base + 66] = 0x32147122;
		JPEGHeaderTable[base + 67] = 0x08A19181;
		JPEGHeaderTable[base + 68] = 0xC1B14223;
		JPEGHeaderTable[base + 69] = 0xF0D15215;
		JPEGHeaderTable[base + 70] = 0x72623324;
		JPEGHeaderTable[base + 71] = 0x160A0982;
		JPEGHeaderTable[base + 72] = 0x1A191817;
		JPEGHeaderTable[base + 73] = 0x28272625;
		JPEGHeaderTable[base + 74] = 0x35342A29;
		JPEGHeaderTable[base + 75] = 0x39383736;
		JPEGHeaderTable[base + 76] = 0x4544433A;
		JPEGHeaderTable[base + 77] = 0x49484746;
		JPEGHeaderTable[base + 78] = 0x5554534A;
		JPEGHeaderTable[base + 79] = 0x59585756;
		JPEGHeaderTable[base + 80] = 0x6564635A;
		JPEGHeaderTable[base + 81] = 0x69686766;
		JPEGHeaderTable[base + 82] = 0x7574736A;
		JPEGHeaderTable[base + 83] = 0x79787776;
		JPEGHeaderTable[base + 84] = 0x8584837A;
		JPEGHeaderTable[base + 85] = 0x89888786;
		JPEGHeaderTable[base + 86] = 0x9493928A;
		JPEGHeaderTable[base + 87] = 0x98979695;
		JPEGHeaderTable[base + 88] = 0xA3A29A99;
		JPEGHeaderTable[base + 89] = 0xA7A6A5A4;
		JPEGHeaderTable[base + 90] = 0xB2AAA9A8;
		JPEGHeaderTable[base + 91] = 0xB6B5B4B3;
		JPEGHeaderTable[base + 92] = 0xBAB9B8B7;
		JPEGHeaderTable[base + 93] = 0xC5C4C3C2;
		JPEGHeaderTable[base + 94] = 0xC9C8C7C6;
		JPEGHeaderTable[base + 95] = 0xD4D3D2CA;
		JPEGHeaderTable[base + 96] = 0xD8D7D6D5;
		JPEGHeaderTable[base + 97] = 0xE2E1DAD9;
		JPEGHeaderTable[base + 98] = 0xE6E5E4E3;
		JPEGHeaderTable[base + 99] = 0xEAE9E8E7;
		JPEGHeaderTable[base + 100] = 0xF4F3F2F1;
		JPEGHeaderTable[base + 101] = 0xF8F7F6F5;
		JPEGHeaderTable[base + 102] = 0xC4FFFAF9;
		JPEGHeaderTable[base + 103] = 0x00011F00;
		JPEGHeaderTable[base + 104] = 0x01010103;
		JPEGHeaderTable[base + 105] = 0x01010101;
		JPEGHeaderTable[base + 106] = 0x00000101;
		JPEGHeaderTable[base + 107] = 0x00000000;
		JPEGHeaderTable[base + 108] = 0x04030201;
		JPEGHeaderTable[base + 109] = 0x08070605;
		JPEGHeaderTable[base + 110] = 0xFF0B0A09;
		JPEGHeaderTable[base + 111] = 0x11B500C4;
		JPEGHeaderTable[base + 112] = 0x02010200;
		JPEGHeaderTable[base + 113] = 0x04030404;
		JPEGHeaderTable[base + 114] = 0x04040507;
		JPEGHeaderTable[base + 115] = 0x77020100;
		JPEGHeaderTable[base + 116] = 0x03020100;
		JPEGHeaderTable[base + 117] = 0x21050411;
		JPEGHeaderTable[base + 118] = 0x41120631;
		JPEGHeaderTable[base + 119] = 0x71610751;
		JPEGHeaderTable[base + 120] = 0x81322213;
		JPEGHeaderTable[base + 121] = 0x91421408;
		JPEGHeaderTable[base + 122] = 0x09C1B1A1;
		JPEGHeaderTable[base + 123] = 0xF0523323;
		JPEGHeaderTable[base + 124] = 0xD1726215;
		JPEGHeaderTable[base + 125] = 0x3424160A;
		JPEGHeaderTable[base + 126] = 0x17F125E1;
		JPEGHeaderTable[base + 127] = 0x261A1918;
		JPEGHeaderTable[base + 128] = 0x2A292827;
		JPEGHeaderTable[base + 129] = 0x38373635;
		JPEGHeaderTable[base + 130] = 0x44433A39;
		JPEGHeaderTable[base + 131] = 0x48474645;
		JPEGHeaderTable[base + 132] = 0x54534A49;
		JPEGHeaderTable[base + 133] = 0x58575655;
		JPEGHeaderTable[base + 134] = 0x64635A59;
		JPEGHeaderTable[base + 135] = 0x68676665;
		JPEGHeaderTable[base + 136] = 0x74736A69;
		JPEGHeaderTable[base + 137] = 0x78777675;
		JPEGHeaderTable[base + 138] = 0x83827A79;
		JPEGHeaderTable[base + 139] = 0x87868584;
		JPEGHeaderTable[base + 140] = 0x928A8988;
		JPEGHeaderTable[base + 141] = 0x96959493;
		JPEGHeaderTable[base + 142] = 0x9A999897;
		JPEGHeaderTable[base + 143] = 0xA5A4A3A2;
		JPEGHeaderTable[base + 144] = 0xA9A8A7A6;
		JPEGHeaderTable[base + 145] = 0xB4B3B2AA;
		JPEGHeaderTable[base + 146] = 0xB8B7B6B5;
		JPEGHeaderTable[base + 147] = 0xC3C2BAB9;
		JPEGHeaderTable[base + 148] = 0xC7C6C5C4;
		JPEGHeaderTable[base + 149] = 0xD2CAC9C8;
		JPEGHeaderTable[base + 150] = 0xD6D5D4D3;
		JPEGHeaderTable[base + 151] = 0xDAD9D8D7;
		JPEGHeaderTable[base + 152] = 0xE5E4E3E2;
		JPEGHeaderTable[base + 153] = 0xE9E8E7E6;
		JPEGHeaderTable[base + 154] = 0xF4F3F2EA;
		JPEGHeaderTable[base + 155] = 0xF8F7F6F5;
		JPEGHeaderTable[base + 156] = 0xDAFFFAF9;
		JPEGHeaderTable[base + 157] = 0x01030C00;
		JPEGHeaderTable[base + 158] = 0x03110200;
		JPEGHeaderTable[base + 159] = 0x003F0011;

		//Table 0
		if (i==0) {
			JPEGHeaderTable[base + 10] = 0x0D140043;
			JPEGHeaderTable[base + 11] = 0x0C0F110F;
			JPEGHeaderTable[base + 12] = 0x11101114;
			JPEGHeaderTable[base + 13] = 0x17141516;
			JPEGHeaderTable[base + 14] = 0x1E20321E;
			JPEGHeaderTable[base + 15] = 0x3D1E1B1B;
			JPEGHeaderTable[base + 16] = 0x32242E2B;
			JPEGHeaderTable[base + 17] = 0x4B4C3F48;
			JPEGHeaderTable[base + 18] = 0x44463F47;
			JPEGHeaderTable[base + 19] = 0x61735A50;
			JPEGHeaderTable[base + 20] = 0x566C5550;
			JPEGHeaderTable[base + 21] = 0x88644644;
			JPEGHeaderTable[base + 22] = 0x7A766C65;
			JPEGHeaderTable[base + 23] = 0x4D808280;
			JPEGHeaderTable[base + 24] = 0x8C978D60;
			JPEGHeaderTable[base + 25] = 0x7E73967D;
			JPEGHeaderTable[base + 26] = 0xDBFF7B80;
			JPEGHeaderTable[base + 27] = 0x1F014300;
			JPEGHeaderTable[base + 28] = 0x272D2121;
			JPEGHeaderTable[base + 29] = 0x3030582D;
			JPEGHeaderTable[base + 30] = 0x697BB958;
			JPEGHeaderTable[base + 31] = 0xB8B9B97B;
			JPEGHeaderTable[base + 32] = 0xB9B8A6A6;
			JPEGHeaderTable[base + 33] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 34] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 35] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 36] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 37] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 38] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 39] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 40] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 41] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 42] = 0xB9B9B9B9;
			JPEGHeaderTable[base + 43] = 0xFFB9B9B9;
		}
		//Table 1
		if (i==1) {
			JPEGHeaderTable[base + 10] = 0x0C110043;
			JPEGHeaderTable[base + 11] = 0x0A0D0F0D;
			JPEGHeaderTable[base + 12] = 0x0F0E0F11;
			JPEGHeaderTable[base + 13] = 0x14111213;
			JPEGHeaderTable[base + 14] = 0x1A1C2B1A;
			JPEGHeaderTable[base + 15] = 0x351A1818;
			JPEGHeaderTable[base + 16] = 0x2B1F2826;
			JPEGHeaderTable[base + 17] = 0x4142373F;
			JPEGHeaderTable[base + 18] = 0x3C3D373E;
			JPEGHeaderTable[base + 19] = 0x55644E46;
			JPEGHeaderTable[base + 20] = 0x4B5F4A46;
			JPEGHeaderTable[base + 21] = 0x77573D3C;
			JPEGHeaderTable[base + 22] = 0x6B675F58;
			JPEGHeaderTable[base + 23] = 0x43707170;
			JPEGHeaderTable[base + 24] = 0x7A847B54;
			JPEGHeaderTable[base + 25] = 0x6E64836D;
			JPEGHeaderTable[base + 26] = 0xDBFF6C70;
			JPEGHeaderTable[base + 27] = 0x1B014300;
			JPEGHeaderTable[base + 28] = 0x22271D1D;
			JPEGHeaderTable[base + 29] = 0x2A2A4C27;
			JPEGHeaderTable[base + 30] = 0x5B6BA04C;
			JPEGHeaderTable[base + 31] = 0xA0A0A06B;
			JPEGHeaderTable[base + 32] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 33] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 34] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 35] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 36] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 37] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 38] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 39] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 40] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 41] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 42] = 0xA0A0A0A0;
			JPEGHeaderTable[base + 43] = 0xFFA0A0A0;
		}
		//Table 2
		if (i==2) {
			JPEGHeaderTable[base + 10] = 0x090E0043;
			JPEGHeaderTable[base + 11] = 0x090A0C0A;
			JPEGHeaderTable[base + 12] = 0x0C0B0C0E;
			JPEGHeaderTable[base + 13] = 0x110E0F10;
			JPEGHeaderTable[base + 14] = 0x15172415;
			JPEGHeaderTable[base + 15] = 0x2C151313;
			JPEGHeaderTable[base + 16] = 0x241A211F;
			JPEGHeaderTable[base + 17] = 0x36372E34;
			JPEGHeaderTable[base + 18] = 0x31322E33;
			JPEGHeaderTable[base + 19] = 0x4653413A;
			JPEGHeaderTable[base + 20] = 0x3E4E3D3A;
			JPEGHeaderTable[base + 21] = 0x62483231;
			JPEGHeaderTable[base + 22] = 0x58564E49;
			JPEGHeaderTable[base + 23] = 0x385D5E5D;
			JPEGHeaderTable[base + 24] = 0x656D6645;
			JPEGHeaderTable[base + 25] = 0x5B536C5A;
			JPEGHeaderTable[base + 26] = 0xDBFF595D;
			JPEGHeaderTable[base + 27] = 0x16014300;
			JPEGHeaderTable[base + 28] = 0x1C201818;
			JPEGHeaderTable[base + 29] = 0x22223F20;
			JPEGHeaderTable[base + 30] = 0x4B58853F;
			JPEGHeaderTable[base + 31] = 0x85858558;
			JPEGHeaderTable[base + 32] = 0x85858585;
			JPEGHeaderTable[base + 33] = 0x85858585;
			JPEGHeaderTable[base + 34] = 0x85858585;
			JPEGHeaderTable[base + 35] = 0x85858585;
			JPEGHeaderTable[base + 36] = 0x85858585;
			JPEGHeaderTable[base + 37] = 0x85858585;
			JPEGHeaderTable[base + 38] = 0x85858585;
			JPEGHeaderTable[base + 39] = 0x85858585;
			JPEGHeaderTable[base + 40] = 0x85858585;
			JPEGHeaderTable[base + 41] = 0x85858585;
			JPEGHeaderTable[base + 42] = 0x85858585;
			JPEGHeaderTable[base + 43] = 0xFF858585;
		}
		//Table 3
		if (i==3) {
			JPEGHeaderTable[base + 10] = 0x070B0043;
			JPEGHeaderTable[base + 11] = 0x07080A08;
			JPEGHeaderTable[base + 12] = 0x0A090A0B;
			JPEGHeaderTable[base + 13] = 0x0D0B0C0C;
			JPEGHeaderTable[base + 14] = 0x11121C11;
			JPEGHeaderTable[base + 15] = 0x23110F0F;
			JPEGHeaderTable[base + 16] = 0x1C141A19;
			JPEGHeaderTable[base + 17] = 0x2B2B2429;
			JPEGHeaderTable[base + 18] = 0x27282428;
			JPEGHeaderTable[base + 19] = 0x3842332E;
			JPEGHeaderTable[base + 20] = 0x313E302E;
			JPEGHeaderTable[base + 21] = 0x4E392827;
			JPEGHeaderTable[base + 22] = 0x46443E3A;
			JPEGHeaderTable[base + 23] = 0x2C4A4A4A;
			JPEGHeaderTable[base + 24] = 0x50565137;
			JPEGHeaderTable[base + 25] = 0x48425647;
			JPEGHeaderTable[base + 26] = 0xDBFF474A;
			JPEGHeaderTable[base + 27] = 0x12014300;
			JPEGHeaderTable[base + 28] = 0x161A1313;
			JPEGHeaderTable[base + 29] = 0x1C1C331A;
			JPEGHeaderTable[base + 30] = 0x3D486C33;
			JPEGHeaderTable[base + 31] = 0x6C6C6C48;
			JPEGHeaderTable[base + 32] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 33] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 34] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 35] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 36] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 37] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 38] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 39] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 40] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 41] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 42] = 0x6C6C6C6C;
			JPEGHeaderTable[base + 43] = 0xFF6C6C6C;
		}
		//Table 4
		if (i==4) {
			JPEGHeaderTable[base + 10] = 0x06090043;
			JPEGHeaderTable[base + 11] = 0x05060706;
			JPEGHeaderTable[base + 12] = 0x07070709;
			JPEGHeaderTable[base + 13] = 0x0A09090A;
			JPEGHeaderTable[base + 14] = 0x0D0E160D;
			JPEGHeaderTable[base + 15] = 0x1B0D0C0C;
			JPEGHeaderTable[base + 16] = 0x16101413;
			JPEGHeaderTable[base + 17] = 0x21221C20;
			JPEGHeaderTable[base + 18] = 0x1E1F1C20;
			JPEGHeaderTable[base + 19] = 0x2B332824;
			JPEGHeaderTable[base + 20] = 0x26302624;
			JPEGHeaderTable[base + 21] = 0x3D2D1F1E;
			JPEGHeaderTable[base + 22] = 0x3735302D;
			JPEGHeaderTable[base + 23] = 0x22393A39;
			JPEGHeaderTable[base + 24] = 0x3F443F2B;
			JPEGHeaderTable[base + 25] = 0x38334338;
			JPEGHeaderTable[base + 26] = 0xDBFF3739;
			JPEGHeaderTable[base + 27] = 0x0D014300;
			JPEGHeaderTable[base + 28] = 0x11130E0E;
			JPEGHeaderTable[base + 29] = 0x15152613;
			JPEGHeaderTable[base + 30] = 0x2D355026;
			JPEGHeaderTable[base + 31] = 0x50505035;
			JPEGHeaderTable[base + 32] = 0x50505050;
			JPEGHeaderTable[base + 33] = 0x50505050;
			JPEGHeaderTable[base + 34] = 0x50505050;
			JPEGHeaderTable[base + 35] = 0x50505050;
			JPEGHeaderTable[base + 36] = 0x50505050;
			JPEGHeaderTable[base + 37] = 0x50505050;
			JPEGHeaderTable[base + 38] = 0x50505050;
			JPEGHeaderTable[base + 39] = 0x50505050;
			JPEGHeaderTable[base + 40] = 0x50505050;
			JPEGHeaderTable[base + 41] = 0x50505050;
			JPEGHeaderTable[base + 42] = 0x50505050;
			JPEGHeaderTable[base + 43] = 0xFF505050;
		}
		//Table 5
		if (i==5) {
			JPEGHeaderTable[base + 10] = 0x04060043;
			JPEGHeaderTable[base + 11] = 0x03040504;
			JPEGHeaderTable[base + 12] = 0x05040506;
			JPEGHeaderTable[base + 13] = 0x07060606;
			JPEGHeaderTable[base + 14] = 0x09090F09;
			JPEGHeaderTable[base + 15] = 0x12090808;
			JPEGHeaderTable[base + 16] = 0x0F0A0D0D;
			JPEGHeaderTable[base + 17] = 0x16161315;
			JPEGHeaderTable[base + 18] = 0x14151315;
			JPEGHeaderTable[base + 19] = 0x1D221B18;
			JPEGHeaderTable[base + 20] = 0x19201918;
			JPEGHeaderTable[base + 21] = 0x281E1514;
			JPEGHeaderTable[base + 22] = 0x2423201E;
			JPEGHeaderTable[base + 23] = 0x17262726;
			JPEGHeaderTable[base + 24] = 0x2A2D2A1C;
			JPEGHeaderTable[base + 25] = 0x25222D25;
			JPEGHeaderTable[base + 26] = 0xDBFF2526;
			JPEGHeaderTable[base + 27] = 0x09014300;
			JPEGHeaderTable[base + 28] = 0x0B0D0A0A;
			JPEGHeaderTable[base + 29] = 0x0E0E1A0D;
			JPEGHeaderTable[base + 30] = 0x1F25371A;
			JPEGHeaderTable[base + 31] = 0x37373725;
			JPEGHeaderTable[base + 32] = 0x37373737;
			JPEGHeaderTable[base + 33] = 0x37373737;
			JPEGHeaderTable[base + 34] = 0x37373737;
			JPEGHeaderTable[base + 35] = 0x37373737;
			JPEGHeaderTable[base + 36] = 0x37373737;
			JPEGHeaderTable[base + 37] = 0x37373737;
			JPEGHeaderTable[base + 38] = 0x37373737;
			JPEGHeaderTable[base + 39] = 0x37373737;
			JPEGHeaderTable[base + 40] = 0x37373737;
			JPEGHeaderTable[base + 41] = 0x37373737;
			JPEGHeaderTable[base + 42] = 0x37373737;
			JPEGHeaderTable[base + 43] = 0xFF373737;
		}
		//Table 6
		if (i==6) {
			JPEGHeaderTable[base + 10] = 0x02030043;
			JPEGHeaderTable[base + 11] = 0x01020202;
			JPEGHeaderTable[base + 12] = 0x02020203;
			JPEGHeaderTable[base + 13] = 0x03030303;
			JPEGHeaderTable[base + 14] = 0x04040704;
			JPEGHeaderTable[base + 15] = 0x09040404;
			JPEGHeaderTable[base + 16] = 0x07050606;
			JPEGHeaderTable[base + 17] = 0x0B0B090A;
			JPEGHeaderTable[base + 18] = 0x0A0A090A;
			JPEGHeaderTable[base + 19] = 0x0E110D0C;
			JPEGHeaderTable[base + 20] = 0x0C100C0C;
			JPEGHeaderTable[base + 21] = 0x140F0A0A;
			JPEGHeaderTable[base + 22] = 0x1211100F;
			JPEGHeaderTable[base + 23] = 0x0B131313;
			JPEGHeaderTable[base + 24] = 0x1516150E;
			JPEGHeaderTable[base + 25] = 0x12111612;
			JPEGHeaderTable[base + 26] = 0xDBFF1213;
			JPEGHeaderTable[base + 27] = 0x04014300;
			JPEGHeaderTable[base + 28] = 0x05060505;
			JPEGHeaderTable[base + 29] = 0x07070D06;
			JPEGHeaderTable[base + 30] = 0x0F121B0D;
			JPEGHeaderTable[base + 31] = 0x1B1B1B12;
			JPEGHeaderTable[base + 32] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 33] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 34] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 35] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 36] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 37] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 38] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 39] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 40] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 41] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 42] = 0x1B1B1B1B;
			JPEGHeaderTable[base + 43] = 0xFF1B1B1B;
		}
		//Table 7
		if (i==7) {
			JPEGHeaderTable[base + 10] = 0x01020043;
			JPEGHeaderTable[base + 11] = 0x01010101;
			JPEGHeaderTable[base + 12] = 0x01010102;
			JPEGHeaderTable[base + 13] = 0x02020202;
			JPEGHeaderTable[base + 14] = 0x03030503;
			JPEGHeaderTable[base + 15] = 0x06030202;
			JPEGHeaderTable[base + 16] = 0x05030404;
			JPEGHeaderTable[base + 17] = 0x07070607;
			JPEGHeaderTable[base + 18] = 0x06070607;
			JPEGHeaderTable[base + 19] = 0x090B0908;
			JPEGHeaderTable[base + 20] = 0x080A0808;
			JPEGHeaderTable[base + 21] = 0x0D0A0706;
			JPEGHeaderTable[base + 22] = 0x0C0B0A0A;
			JPEGHeaderTable[base + 23] = 0x070C0D0C;
			JPEGHeaderTable[base + 24] = 0x0E0F0E09;
			JPEGHeaderTable[base + 25] = 0x0C0B0F0C;
			JPEGHeaderTable[base + 26] = 0xDBFF0C0C;
			JPEGHeaderTable[base + 27] = 0x03014300;
			JPEGHeaderTable[base + 28] = 0x03040303;
			JPEGHeaderTable[base + 29] = 0x04040804;
			JPEGHeaderTable[base + 30] = 0x0A0C1208;
			JPEGHeaderTable[base + 31] = 0x1212120C;
			JPEGHeaderTable[base + 32] = 0x12121212;
			JPEGHeaderTable[base + 33] = 0x12121212;
			JPEGHeaderTable[base + 34] = 0x12121212;
			JPEGHeaderTable[base + 35] = 0x12121212;
			JPEGHeaderTable[base + 36] = 0x12121212;
			JPEGHeaderTable[base + 37] = 0x12121212;
			JPEGHeaderTable[base + 38] = 0x12121212;
			JPEGHeaderTable[base + 39] = 0x12121212;
			JPEGHeaderTable[base + 40] = 0x12121212;
			JPEGHeaderTable[base + 41] = 0x12121212;
			JPEGHeaderTable[base + 42] = 0x12121212;
			JPEGHeaderTable[base + 43] = 0xFF121212;
		}
		//Table 8
		if (i==8) {
			JPEGHeaderTable[base + 10] = 0x01020043;
			JPEGHeaderTable[base + 11] = 0x01010101;
			JPEGHeaderTable[base + 12] = 0x01010102;
			JPEGHeaderTable[base + 13] = 0x02020202;
			JPEGHeaderTable[base + 14] = 0x03030503;
			JPEGHeaderTable[base + 15] = 0x06030202;
			JPEGHeaderTable[base + 16] = 0x05030404;
			JPEGHeaderTable[base + 17] = 0x07070607;
			JPEGHeaderTable[base + 18] = 0x06070607;
			JPEGHeaderTable[base + 19] = 0x090B0908;
			JPEGHeaderTable[base + 20] = 0x080A0808;
			JPEGHeaderTable[base + 21] = 0x0D0A0706;
			JPEGHeaderTable[base + 22] = 0x0C0B0A0A;
			JPEGHeaderTable[base + 23] = 0x070C0D0C;
			JPEGHeaderTable[base + 24] = 0x0E0F0E09;
			JPEGHeaderTable[base + 25] = 0x0C0B0F0C;
			JPEGHeaderTable[base + 26] = 0xDBFF0C0C;
			JPEGHeaderTable[base + 27] = 0x02014300;
			JPEGHeaderTable[base + 28] = 0x03030202;
			JPEGHeaderTable[base + 29] = 0x04040703;
			JPEGHeaderTable[base + 30] = 0x080A0F07;
			JPEGHeaderTable[base + 31] = 0x0F0F0F0A;
			JPEGHeaderTable[base + 32] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 33] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 34] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 35] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 36] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 37] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 38] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 39] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 40] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 41] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 42] = 0x0F0F0F0F;
			JPEGHeaderTable[base + 43] = 0xFF0F0F0F;
		}
		//Table 9
		if (i==9) {
			JPEGHeaderTable[base + 10] = 0x01010043;
			JPEGHeaderTable[base + 11] = 0x01010101;
			JPEGHeaderTable[base + 12] = 0x01010101;
			JPEGHeaderTable[base + 13] = 0x01010101;
			JPEGHeaderTable[base + 14] = 0x02020302;
			JPEGHeaderTable[base + 15] = 0x04020202;
			JPEGHeaderTable[base + 16] = 0x03020303;
			JPEGHeaderTable[base + 17] = 0x05050405;
			JPEGHeaderTable[base + 18] = 0x05050405;
			JPEGHeaderTable[base + 19] = 0x07080606;
			JPEGHeaderTable[base + 20] = 0x06080606;
			JPEGHeaderTable[base + 21] = 0x0A070505;
			JPEGHeaderTable[base + 22] = 0x09080807;
			JPEGHeaderTable[base + 23] = 0x05090909;
			JPEGHeaderTable[base + 24] = 0x0A0B0A07;
			JPEGHeaderTable[base + 25] = 0x09080B09;
			JPEGHeaderTable[base + 26] = 0xDBFF0909;
			JPEGHeaderTable[base + 27] = 0x02014300;
			JPEGHeaderTable[base + 28] = 0x02030202;
			JPEGHeaderTable[base + 29] = 0x03030503;
			JPEGHeaderTable[base + 30] = 0x07080C05;
			JPEGHeaderTable[base + 31] = 0x0C0C0C08;
			JPEGHeaderTable[base + 32] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 33] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 34] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 35] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 36] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 37] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 38] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 39] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 40] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 41] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 42] = 0x0C0C0C0C;
			JPEGHeaderTable[base + 43] = 0xFF0C0C0C;
		}
		//Table 10
		if (i==10) {
			JPEGHeaderTable[base + 10] = 0x01010043;
			JPEGHeaderTable[base + 11] = 0x01010101;
			JPEGHeaderTable[base + 12] = 0x01010101;
			JPEGHeaderTable[base + 13] = 0x01010101;
			JPEGHeaderTable[base + 14] = 0x01010201;
			JPEGHeaderTable[base + 15] = 0x03010101;
			JPEGHeaderTable[base + 16] = 0x02010202;
			JPEGHeaderTable[base + 17] = 0x03030303;
			JPEGHeaderTable[base + 18] = 0x03030303;
			JPEGHeaderTable[base + 19] = 0x04050404;
			JPEGHeaderTable[base + 20] = 0x04050404;
			JPEGHeaderTable[base + 21] = 0x06050303;
			JPEGHeaderTable[base + 22] = 0x06050505;
			JPEGHeaderTable[base + 23] = 0x03060606;
			JPEGHeaderTable[base + 24] = 0x07070704;
			JPEGHeaderTable[base + 25] = 0x06050706;
			JPEGHeaderTable[base + 26] = 0xDBFF0606;
			JPEGHeaderTable[base + 27] = 0x01014300;
			JPEGHeaderTable[base + 28] = 0x01020101;
			JPEGHeaderTable[base + 29] = 0x02020402;
			JPEGHeaderTable[base + 30] = 0x05060904;
			JPEGHeaderTable[base + 31] = 0x09090906;
			JPEGHeaderTable[base + 32] = 0x09090909;
			JPEGHeaderTable[base + 33] = 0x09090909;
			JPEGHeaderTable[base + 34] = 0x09090909;
			JPEGHeaderTable[base + 35] = 0x09090909;
			JPEGHeaderTable[base + 36] = 0x09090909;
			JPEGHeaderTable[base + 37] = 0x09090909;
			JPEGHeaderTable[base + 38] = 0x09090909;
			JPEGHeaderTable[base + 39] = 0x09090909;
			JPEGHeaderTable[base + 40] = 0x09090909;
			JPEGHeaderTable[base + 41] = 0x09090909;
			JPEGHeaderTable[base + 42] = 0x09090909;
			JPEGHeaderTable[base + 43] = 0xFF090909;
		}
		//Table 11
	if (i==11) {
			JPEGHeaderTable[base + 10] = 0x01010043;
			JPEGHeaderTable[base + 11] = 0x01010101;
			JPEGHeaderTable[base + 12] = 0x01010101;
			JPEGHeaderTable[base + 13] = 0x01010101;
			JPEGHeaderTable[base + 14] = 0x01010101;
			JPEGHeaderTable[base + 15] = 0x01010101;
			JPEGHeaderTable[base + 16] = 0x01010101;
			JPEGHeaderTable[base + 17] = 0x01010101;
			JPEGHeaderTable[base + 18] = 0x01010101;
			JPEGHeaderTable[base + 19] = 0x02020202;
			JPEGHeaderTable[base + 20] = 0x02020202;
			JPEGHeaderTable[base + 21] = 0x03020101;
			JPEGHeaderTable[base + 22] = 0x03020202;
			JPEGHeaderTable[base + 23] = 0x01030303;
			JPEGHeaderTable[base + 24] = 0x03030302;
			JPEGHeaderTable[base + 25] = 0x03020303;
			JPEGHeaderTable[base + 26] = 0xDBFF0403;
			JPEGHeaderTable[base + 27] = 0x01014300;
			JPEGHeaderTable[base + 28] = 0x01010101;
			JPEGHeaderTable[base + 29] = 0x01010201;
			JPEGHeaderTable[base + 30] = 0x03040602;
			JPEGHeaderTable[base + 31] = 0x06060604;
			JPEGHeaderTable[base + 32] = 0x06060606;
			JPEGHeaderTable[base + 33] = 0x06060606;
			JPEGHeaderTable[base + 34] = 0x06060606;
			JPEGHeaderTable[base + 35] = 0x06060606;
			JPEGHeaderTable[base + 36] = 0x06060606;
			JPEGHeaderTable[base + 37] = 0x06060606;
			JPEGHeaderTable[base + 38] = 0x06060606;
			JPEGHeaderTable[base + 39] = 0x06060606;
			JPEGHeaderTable[base + 40] = 0x06060606;
			JPEGHeaderTable[base + 41] = 0x06060606;
			JPEGHeaderTable[base + 42] = 0x06060606;
			JPEGHeaderTable[base + 43] = 0xFF060606;
		}
	}
	memcpy(AST_VIDEOCAP_HDR_BUF_ADDR, JPEGHeaderTable , sizeof(JPEGHeaderTable));
	//printk("sizeof JPEGHeaderTable = %d\n",sizeof(JPEGHeaderTable));
	ast_videocap_write_reg((uint32_t)AST_VIDEOCAP_HDR_BUF_ADDR, AST_VIDEOCAP_CRC_BUF_ADDR);
}


int StartVideoCapture(struct ast_videocap_engine_info_t *info)
{
	unsigned long DRAM_Data;

	InitDefaultSettings(info);
	prev_cursor_status = 0;
	prev_cursor_pattern_addr = NULL;
	
	// Set full screen flag for jpeg mode alone
	if(CaptureMode == VIDEOCAP_JPEG_SUPPORT) 
		full_screen_capture = 1;

	// For first screen, whole frame should be reported as changed
	ast_videocap_write_reg(virt_to_phys(AST_VIDEOCAP_REF_BUF_ADDR), AST_VIDEOCAP_SRC_BUF2_ADDR);
	ast_videocap_write_reg(virt_to_phys(AST_VIDEOCAP_IN_BUF_ADDR), AST_VIDEOCAP_SRC_BUF1_ADDR);
	ast_videocap_write_reg(virt_to_phys(AST_VIDEOCAP_FLAG_BUF_ADDR), AST_VIDEOCAP_BCD_FLAG_BUF_ADDR);
	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
		ast_videocap_write_reg(virt_to_phys(AST_VIDEOCAP_HDR_BUF_ADDR), AST_VIDEOCAP_CRC_BUF_ADDR);
	ast_videocap_write_reg(virt_to_phys(AST_VIDEOCAP_COMPRESS_BUF_ADDR), AST_VIDEOCAP_COMPRESSED_BUF_ADDR);

	/* reset compression control & disable RC4 encryption */
	ast_videocap_write_reg(0x00080000, AST_VIDEOCAP_COMPRESS_CTRL);

#ifdef SOC_AST2600
	// DRAM_Data = *(volatile unsigned long *) ast_sdram_reg_virt_base;
	DRAM_Data = ioread32((void * __iomem)ast_sdram_reg_virt_base + 0x04);
#else
	DRAM_Data = *(volatile unsigned long *) SDRAM_CONFIG_REG;
#endif
	
#if defined(SOC_AST2300) || defined(SOC_AST2400)
	switch (DRAM_Data & 0x03) {
	case 0:
		info->TotalMemory = 64;
		break;
	case 1:
		info->TotalMemory = 128;
		break;
	case 2:
		info->TotalMemory = 256;
		break;
	case 3:
		info->TotalMemory = 512;
		break;
	}
	switch ((DRAM_Data >> 2) & 0x03) {
	case 0:
		info->VGAMemory = 8;
		break;
	case 1:
		info->VGAMemory = 16;
		break;
	case 2:
		info->VGAMemory = 32;
		break;
	case 3:
		info->VGAMemory = 64;
		break;
	}
#elif defined(SOC_AST2500) || defined(SOC_AST2530)
	switch (DRAM_Data & 0x03) {
	case 0:
		info->TotalMemory = 128;
		break;
	case 1:
		info->TotalMemory = 256;
		break;
	case 2:
		info->TotalMemory = 512;
		break;
	case 3:
		info->TotalMemory = 1024;
		break;
	}

	switch ((DRAM_Data >> 2) & 0x03) {
	case 0:
		info->VGAMemory = 8;
		break;
	case 1:
		info->VGAMemory = 16;
		break;
	case 2:
		info->VGAMemory = 32;
		break;
	case 3:
		info->VGAMemory = 64;
		break;
	}
#elif defined(SOC_AST2600)
	switch (DRAM_Data & 0x03) {
	case 0:
		info->TotalMemory = 256;
		break;
	case 1:
		info->TotalMemory = 512;
		break;
	case 2:
		info->TotalMemory = 1024;
		break;
	case 3:
		info->TotalMemory = 2048;
		break;
	}

	switch ((DRAM_Data >> 2) & 0x03) {
	case 0:
		info->VGAMemory = 8;
		break;
	case 1:
		info->VGAMemory = 16;
		break;
	case 2:
		info->VGAMemory = 32;
		break;
	case 3:
		info->VGAMemory = 64;
		break;
	}
#else
	switch ((DRAM_Data >> 2) & 0x03) {
	case 0:
		info->TotalMemory = 32;
		break;
	case 1:
		info->TotalMemory = 64;
		break;
	case 2:
		info->TotalMemory = 128;
		break;
	case 3:
		info->TotalMemory = 256;
		break;
	}
	switch ((DRAM_Data >> 4) & 0x03) {
	case 0:
		info->VGAMemory = 8;
		break;
	case 1:
		info->VGAMemory = 16;
		break;
	case 2:
		info->VGAMemory = 32;
		break;
	case 3:
		info->VGAMemory = 64;
		break;
	}
#endif

	//printk("Total Memory = %ld MB\n", info->TotalMemory);
	//printk("VGA Memory = %ld MB\n", info->VGAMemory);
	ast_videocap_get_mem_bandwidth(info);

	ast_videocap_mode_detection(info,0);
	ast_videocap_data_in_old_video_buf = 0;

	return 0;
}

static void ast_videocap_horizontal_down_scaling(struct ast_videocap_engine_info_t *info)
{
	ast_videocap_write_reg(0, AST_VIDEOCAP_SCALING_FACTOR);
	ast_videocap_write_reg(0x10001000, AST_VIDEOCAP_SCALING_FACTOR);
}

static void ast_videocap_vertical_down_scaling(struct ast_videocap_engine_info_t *info)
{
	ast_videocap_write_reg(0, AST_VIDEOCAP_SCALING_FACTOR);
	ast_videocap_write_reg(0x10001000, AST_VIDEOCAP_SCALING_FACTOR);
}

int ast_videocap_engine_init(struct ast_videocap_engine_info_t *info)
{
	uint32_t diff_setting;
	uint32_t compress_setting;
	unsigned long OldBufferAddress, NewBufferAddress;
	unsigned long buf_offset;
	unsigned long remainder;

	#if defined(SOC_AST2300)
	ast_videocap_write_reg(0, AST_VIDEOCAP_PRIMARY_CRC_PARAME);
	ast_videocap_write_reg(0, AST_VIDEOCAP_SECONDARY_CRC_PARAM);
	#endif
	ast_videocap_write_reg(0, AST_VIDEOCAP_DATA_TRUNCATION); /* no reduction on RGB */

	/* Set to JPEG compress/decompress mode */
	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		 
		ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_JPEG);
		ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_JPEG_FRAME);

	}
	else
	{
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_JPEG);
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_JPEG_FRAME);
	}

	/* Set to YUV444 mode */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_FORMAT_MASK);
	info->FrameHeader.Mode420 = 0;

	if (info->INFData.AutoMode) {
		ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_AUTO_COMPRESS);
	}
	else {
		/* disable automatic compression */
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_AUTO_COMPRESS);
	}
	/* Internal always uses internal timing generation */
	ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_DE_SIGNAL);

	/* Internal use inverse clock and software cursor */
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_CLK_DELAY_MASK);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_INV_CLK_NO_DELAY);

  /* clear bit 8 of VR008 when not in DirectMode. */
  if (info->INFData.DirectMode) {
		ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_DIRECT_FETCH_FRAME_BUF);

		ast_videocap_write_reg((info->TotalMemory - info->VGAMemory) << 20, AST_VIDEOCAP_DIRECT_FRAME_BUF_ADDR);
		ast_videocap_write_reg(info->src_mode.x * 4, AST_VIDEOCAP_DIRECT_FRAME_BUF_CTRL);

		/* always use auto mode since we can not get the correct setting when the VGA driver is not installed */
		ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_AUTO_MODE);
	} else {
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_DIRECT_FETCH_FRAME_BUF);

		/* without VGA hardware cursor overlay image */
		ast_videocap_set_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_HW_CSR_OVERLAY); 
	}

	/* set scaling filter parameters */
	if ((info->src_mode.x == info->dest_mode.x) && (info->src_mode.y == info->dest_mode.y)) {
		/* scaling factor == 1.0, that is, non-scaling */
		ast_videocap_write_reg(AST_VIDEOCAP_SCALING_FILTER_PARAM_ONE, AST_VIDEOCAP_SCALING_FILTER_PARAM0);
		ast_videocap_write_reg(AST_VIDEOCAP_SCALING_FILTER_PARAM_ONE, AST_VIDEOCAP_SCALING_FILTER_PARAM1);
		ast_videocap_write_reg(AST_VIDEOCAP_SCALING_FILTER_PARAM_ONE, AST_VIDEOCAP_SCALING_FILTER_PARAM2);
		ast_videocap_write_reg(AST_VIDEOCAP_SCALING_FILTER_PARAM_ONE, AST_VIDEOCAP_SCALING_FILTER_PARAM3);
	}

  /* Fix ast2500 resolution 1680x1050 issue, follow SDK ast-video.c line:507 */
#if defined(SOC_AST2500) || defined(SOC_AST2600)
  if(info->src_mode.x==1680)
  {
   ast_videocap_write_reg((1728 << AST_VIDEOCAP_WINDOWS_H_SHIFT) + info->src_mode.y, AST_VIDEOCAP_CAPTURE_WINDOW);
   ast_videocap_write_reg((info->src_mode.x << AST_VIDEOCAP_WINDOWS_H_SHIFT) | info->src_mode.y, AST_VIDEOCAP_COMPRESS_WINDOW);
  }
  else
  {
   ast_videocap_write_reg((info->src_mode.x << AST_VIDEOCAP_WINDOWS_H_SHIFT) + info->src_mode.y, AST_VIDEOCAP_CAPTURE_WINDOW);
   ast_videocap_write_reg((info->src_mode.x << AST_VIDEOCAP_WINDOWS_H_SHIFT) + info->src_mode.y, AST_VIDEOCAP_COMPRESS_WINDOW);
  }
#else
   ast_videocap_write_reg((info->src_mode.x << AST_VIDEOCAP_WINDOWS_H_SHIFT) | info->src_mode.y, AST_VIDEOCAP_CAPTURE_WINDOW);
   ast_videocap_write_reg((info->src_mode.x << AST_VIDEOCAP_WINDOWS_H_SHIFT) | info->src_mode.y, AST_VIDEOCAP_COMPRESS_WINDOW);
#endif

	/* set buffer offset based on detected mode */
	buf_offset = info->src_mode.x;
	remainder = buf_offset % 8;
	if (remainder != 0) {
		buf_offset += (8 - remainder);
	}
	ast_videocap_write_reg(buf_offset * 4, AST_VIDEOCAP_SCAN_LINE_OFFSET);

	/* set buffers addresses */
	if(((CaptureMode == VIDEOCAP_JPEG_SUPPORT) && (full_screen_capture == 1))|| (ast_videocap_data_in_old_video_buf ==0))
	{
		ast_videocap_write_reg((unsigned long) AST_VIDEOCAP_IN_BUF_ADDR, AST_VIDEOCAP_SRC_BUF1_ADDR);
		ast_videocap_write_reg((unsigned long) AST_VIDEOCAP_REF_BUF_ADDR, AST_VIDEOCAP_SRC_BUF2_ADDR);
		diff_setting = AST_VIDEOCAP_BCD_CTRL_DISABLE;
	}
	else
	{
		OldBufferAddress = ast_videocap_read_reg(AST_VIDEOCAP_SRC_BUF1_ADDR);
		NewBufferAddress = ast_videocap_read_reg(AST_VIDEOCAP_SRC_BUF2_ADDR);
		ast_videocap_write_reg(NewBufferAddress, AST_VIDEOCAP_SRC_BUF1_ADDR);
		ast_videocap_write_reg(OldBufferAddress, AST_VIDEOCAP_SRC_BUF2_ADDR);
		diff_setting = AST_VIDEOCAP_BCD_CTRL_ENABLE;
	}

	diff_setting |= (info->INFData.DigitalDifferentialThreshold << AST_VIDEOCAP_BCD_CTRL_TOLERANCE_SHIFT);
	ast_videocap_write_reg(diff_setting, AST_VIDEOCAP_BCD_CTRL);

	// Set this stream register even in frame mode
	ast_videocap_write_reg(0x0000001F, AST_VIDEOCAP_STREAM_BUF_SIZE);

	//  Configure JPEG parameters
	switch (info->FrameHeader.CompressionMode) {
	case COMP_MODE_YUV420:
		info->FrameHeader.Mode420 = 1;
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_FORMAT_MASK);
		ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_FORMAT_YUV420 << AST_VIDEOCAP_SEQ_CTRL_FORMAT_SHIFT);
		compress_setting = 1;
		break;
	case COMP_MODE_YUV444_JPG_ONLY:
		compress_setting = 1;
		break;
	case COMP_MODE_YUV444_2_CLR_VQ:
		info->INFData.VQMode = 2;
		compress_setting = 0;
		break;
	case COMP_MODE_YUV444_4_CLR_VQ:
		info->INFData.VQMode = 4;
		compress_setting = 2;
		break;
	default:
		printk("videocap: Default CompressionMode triggered!! %ld\n",
		info->FrameHeader.CompressionMode);
		info->FrameHeader.Mode420 = 1;
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_FORMAT_MASK);
		ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_FORMAT_YUV420 << AST_VIDEOCAP_SEQ_CTRL_FORMAT_SHIFT);
		compress_setting = 1;
		info->FrameHeader.CompressionMode = COMP_MODE_YUV420;
		break;
	}

	/* always use the same selector */
	compress_setting |= (info->FrameHeader.JPEGTableSelector) << 11;
	compress_setting |= (info->FrameHeader.JPEGTableSelector) << 6;
	/* if mapping is set, luminance and chrominance use the same table, else use respective table with same slector */
	if (!info->FrameHeader.JPEGYUVTableMapping)
		compress_setting |= 1 << 10;
	#if defined(SOC_AST2300) || defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2530) || defined(SOC_AST2600)
	if (!info->FrameHeader.Mode420) {
		compress_setting |= (info->FrameHeader.AdvanceTableSelector) << 27;
		compress_setting |= (info->FrameHeader.AdvanceTableSelector) << 22;
		if (!info->FrameHeader.JPEGYUVTableMapping)
			compress_setting |= 1 << 26;
		compress_setting |= 0x00010000;
	}
	#endif
	compress_setting |= 0x00080000;
	
	if(CaptureMode == VIDEOCAP_JPEG_SUPPORT) 
	{
		compress_setting |= VR060_VIDEO_COMPRESSION_SETTING;
	}
	ast_videocap_write_reg(compress_setting, AST_VIDEOCAP_COMPRESS_CTRL);

	return 0;
}

void update_cursor_position(void)
{
	uint32_t cursor_position;

	cursor_position = ast_videocap_read_reg(AST_VIDEOCAP_HW_CURSOR_POSITION);
	if (cursor_position != prev_cursor_position) { /* position is changed */
		cursor_pos_x = (uint16_t) (cursor_position & 0x00000FFF);
		cursor_pos_y = (uint16_t) ((cursor_position >> 16) & 0x7FF);
		prev_cursor_position = cursor_position;
	}
}

int ast_videocap_create_cursor_packet(struct ast_videocap_engine_info_t *info, void * ioc)
{
	struct ast_videocap_cursor_info_t *cursor_info;
	ASTCap_Ioctl *ioctl_ptr = (ASTCap_Ioctl *)ioc;
	void *cursor_pattern_addr;
	uint32_t cursor_status;
	uint32_t cursor_checksum;

	cursor_status = ast_videocap_read_reg(AST_VIDEOCAP_HW_CURSOR_STATUS);

	if ((cursor_status & AST_VIDEOCAP_HW_CURSOR_STATUS_DRIVER_MASK) != AST_VIDEOCAP_HW_CURSOR_STATUS_DRIVER_MGAIC) { /* VGA driver is not installed */
		//printk("VGA driver is not installed\n");
		return -1;
	}

	if (!(cursor_status & AST_VIDEOCAP_HW_CURSOR_STATUS_ENABLE)) { /* HW cursor is disabled */
		//printk("HW cursor is disabled\n");
		return -1;
	}

	update_cursor_position();
	cursor_pattern_addr = vga_mem_virt_addr + ast_videocap_read_reg(AST_VIDEOCAP_HW_CURSOR_PATTERN_ADDR);
	cursor_checksum = ioread32((void * __iomem)cursor_pattern_addr + AST_VIDEOCAP_CURSOR_BITMAP_DATA); /* cursor checksum is behind the cursor pattern data */

	if ((cursor_status != prev_cursor_status) || /* type or offset changed */
		(cursor_pattern_addr != prev_cursor_pattern_addr) || /* pattern address changed */
		(cursor_checksum != prev_cursor_checksum) || /* pattern changed */
		(cursor_pos_x != prev_cursor_pos_x) || (cursor_pos_y != prev_cursor_pos_y)) { /* position changed */
		cursor_info = (struct ast_videocap_cursor_info_t *) (AST_VIDEOCAP_HDR_BUF_ADDR + AST_VIDEOCAP_HDR_BUF_VIDEO_SZ);
		cursor_info->type = (cursor_status & AST_VIDEOCAP_HW_CURSOR_STATUS_TYPE) ? AST_VIDEOCAP_HW_CURSOR_TYPE_COLOR : AST_VIDEOCAP_HW_CURSOR_TYPE_MONOCHROME;
		cursor_info->pos_x = cursor_pos_x;
		cursor_info->pos_y = cursor_pos_y;
		cursor_info->offset_x = (uint16_t) ((cursor_status >> 24) & 0x3F); /* bits 29:24 */
		cursor_info->offset_y = (uint16_t) ((cursor_status >> 16) & 0x3F); /* bits 21:16 */
		cursor_info->checksum = cursor_checksum;

		/* only send the cursor pattern to client when checksum is changed */
		if ((cursor_checksum != prev_cursor_checksum) || (cursor_pattern_addr != prev_cursor_pattern_addr)) { /* pattern is changed */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
			dmac_inv_range(cursor_pattern_addr, cursor_pattern_addr + AST_VIDEOCAP_CURSOR_BITMAP_DATA);
#else
#if !defined(SOC_AST2500) && !defined(SOC_AST2530) && !defined(SOC_AST2600)
			dmac_map_area(cursor_pattern_addr,AST_VIDEOCAP_CURSOR_BITMAP_DATA,DMA_BIDIRECTIONAL);
			outer_inv_range((unsigned long)cursor_pattern_addr,(unsigned long)cursor_pattern_addr+AST_VIDEOCAP_CURSOR_BITMAP_DATA);
#endif
#endif
			memcpy((void *) cursor_info->pattern, cursor_pattern_addr, AST_VIDEOCAP_CURSOR_BITMAP_DATA);
			prev_cursor_checksum = cursor_checksum;
			ioctl_ptr->Size = sizeof(struct ast_videocap_cursor_info_t);
		} else {
			ioctl_ptr->Size = sizeof(struct ast_videocap_cursor_info_t) - AST_VIDEOCAP_CURSOR_BITMAP_DATA;
		}

		if(0 == access_ok(ioctl_ptr->vPtr, sizeof(struct ast_videocap_cursor_info_t)))
		{
			printk("access ok failed in ast_videocap_cursor_info_packet\n");
		    return -1;

		}
		if (copy_to_user(ioctl_ptr->vPtr, cursor_info, sizeof(struct ast_videocap_cursor_info_t)))
		{
		    printk("copy_to_user failed in ast_videocap_cursor_info_packet\n");
		    return -1;
		}

		/* save current status */
		prev_cursor_status = cursor_status;
		prev_cursor_pos_x = cursor_pos_x;
		prev_cursor_pos_y = cursor_pos_y;
		prev_cursor_pattern_addr = cursor_pattern_addr;
		return 0;
	} else { /* no change */
		return -1;
	}
}

int partialjpeg(struct ast_videocap_engine_info_t *info, int X0, int Y0, int X1, int Y1)
{

	int bpp = (info->FrameHeader.Mode420 != 0) ? BLOCK_SIZE_YUV420 : BLOCK_SIZE_YUV444;
	int width = (X1 - X0 + 1) * bpp;
	int height = (Y1 - Y0 + 1) * bpp;
	uint32_t VR044_buff_addr = ast_videocap_read_reg(AST_VIDEOCAP_SRC_BUF1_ADDR);
	unsigned long VR044_move_pos = 0;
	unsigned long VR0314_buff_end_addr = 0;
	unsigned long VR0318_buff_start_addr = 0;

	//calculate the address of first changed tile to move into the video source buffer
	VR044_move_pos = (unsigned long)((ast_videocap_read_reg(AST_VIDEOCAP_SCAN_LINE_OFFSET)) * bpp * Y0 + 256 * X0);

	//Step 5.	Clear CBD flag buffer with 0x1 and disable BCD in VR02C.
	//Clear BCD buffer by settin 0x1
	memset(AST_VIDEOCAP_FLAG_BUF_ADDR, 0x1, AST_VIDEOCAP_FLAG_BUF_SZ);

	//Disable BCD
	ast_videocap_write_reg(AST_VIDEOCAP_BCD_CTRL_DISABLE, AST_VIDEOCAP_BCD_CTRL);

	//Step 6.	Trigger JPEG compression base on the minimum bonding box.
	// VR044 needs to be programmed to the byte address of the first changed block in YUV data buffer which is source buffer #1 (base address+offset).
	// In YUV444 mode:
	// Address = Address_in_VR044+VR048*8*Y0+256*X0
	if (VR044_move_pos > 0)
	{
		ast_videocap_write_reg((unsigned long)(VR044_buff_addr + VR044_move_pos), AST_VIDEOCAP_SRC_BUF1_ADDR);
	}

	//BACKUP the register address
	VR0314_buff_end_addr = (unsigned long)ast_videocap_read_reg(AST_VIDEOCAP_MEM_RESTRICT_END);
	VR0318_buff_start_addr = (unsigned long)ast_videocap_read_reg(AST_VIDEOCAP_MEM_RESTRICT_START);

	//Update the register
	ast_videocap_write_reg(ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_BUF_ADDR), AST_VIDEOCAP_MEM_RESTRICT_START);
	ast_videocap_write_reg(ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_BUF_ADDR) + AST_VIDEOCAP_COMPRESS_BUF_SZ, AST_VIDEOCAP_MEM_RESTRICT_END);

	// VR034: window width and height needs to be reprogrammed.
	ast_videocap_write_reg((width << AST_VIDEOCAP_WINDOWS_H_SHIFT) + height, AST_VIDEOCAP_CAPTURE_WINDOW);
	ast_videocap_write_reg((width << AST_VIDEOCAP_WINDOWS_H_SHIFT) + height, AST_VIDEOCAP_COMPRESS_WINDOW);

	// clear and set the jpeg compression mode.
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_JPEG);
	ast_videocap_clear_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_JPEG_FRAME);

	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_JPEG);
	ast_videocap_set_reg_bits(AST_VIDEOCAP_SEQ_CTRL, AST_VIDEOCAP_SEQ_CTRL_COMPRESS_JPEG_FRAME);

#ifdef SOC_AST2600
	/* Fixes capture engine busy issue when direct mode is enabled for host resolution >= 1024x768 */
	if (info->INFData.DirectMode == 0)
#endif
	{
		ast_videocap_clear_reg_bits(AST_VIDEOCAP_CTRL, AST_VIDEOCAP_CTRL_DIRECT_FETCH_FRAME_BUF);
	}

	//Delay may required, But to avoid the engine busy it will be useful.
	udelay(100);

	if (ast_videocap_automode_trigger() < 0)
	{
		printk("[%s:%d] ast_videocap_automode_trigger failed!\n", __FUNCTION__, __LINE__);
		return ASTCAP_IOCTL_BLANK_SCREEN;
	}

	//RESTORE

	//7.Restore VR044 with original source buffer address.
	if (VR044_move_pos > 0)
		ast_videocap_write_reg((unsigned long)VR044_buff_addr, AST_VIDEOCAP_SRC_BUF1_ADDR);

	//restore original
	ast_videocap_write_reg(VR0318_buff_start_addr, AST_VIDEOCAP_MEM_RESTRICT_START);
	ast_videocap_write_reg(VR0314_buff_end_addr, AST_VIDEOCAP_MEM_RESTRICT_END);

	return 0;
}

int create_bonding_box(unsigned char *addr, struct ast_videocap_engine_info_t *info)
{
	int i = 0, j = 0;
	int width = 0, height = 0;
	int tile_count = 0;
	int temp_width = 0, temp_height = 0;
	int start_x = -1, start_y = -1, end_x = 0, end_y = 0;
	int block_size = BLOCK_SIZE_YUV444;
#if defined(SOC_AST2600)
	struct ast_videocap_jpeg_tile_info_t tile_info;
#else
	struct ast_videocap_jpeg_tile_info_t *tile_info = NULL;
#endif

	if (gtileinfo == NULL)
	{
		printk("[%s:%d] Block change info is null!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if ((info->src_mode.x <= 0) || (info->src_mode.y <= 0))
	{
		printk("[%s:%d] Invalid %s (%d) for creating bonding box!\n", __FUNCTION__, __LINE__, (info->src_mode.x <= 0) ? "width" : "height", (info->src_mode.x <= 0) ? info->src_mode.x : info->src_mode.y);
		return 0;
	}
#if !defined(SOC_AST2600)
	tile_info = (struct ast_videocap_jpeg_tile_info_t *)gtileinfo;
#endif

	if (info->FrameHeader.Mode420 != 0)
	{
		block_size = BLOCK_SIZE_YUV420;
	}

	width = (info->src_mode.x / block_size);
	height = (info->src_mode.y / block_size);

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
#if defined(SOC_AST2600)
			if (0xf != (0xf & addr[i + (j * width)]))
#else
			if (0xf == addr[i + j * width])
#endif
			{
				if (start_x == -1)
					start_x = end_x = i;

				if (start_y == -1)
					start_y = end_y = j;

				if (i < start_x)
					start_x = i;

				if (j < start_y)
					start_y = j;

				if (i > end_x)
					end_x = i;

				if (j > end_y)
					end_y = j;

				tile_count++;
			}
		}
	}

	if ((end_x + block_size) >= (info->src_mode.x / block_size))
		end_x = (info->src_mode.x / block_size) - 1;
	else
		end_x = (end_x + block_size);

	if ((end_y + block_size) >= (info->src_mode.y / block_size))
		end_y = (info->src_mode.y / block_size) - 1;
	else
		end_y = (end_y + block_size);

	/* width, height of bonding box */
	width = (end_x - (start_x) + 1) * block_size;
	height = (end_y - (start_y) + 1) * block_size;

	/* To check overflow */
	temp_width = (start_x * block_size) + width;
	temp_height = (start_y * block_size) + height;

	// In case of overflow, adjust the width / height values
	// accroding to current host video resolution
	if (temp_width > info->src_mode.x)
	{
		width -= (temp_width - info->src_mode.x);
	}

	if (temp_height > info->src_mode.y)
	{
		height -= (temp_height - info->src_mode.y);
	}

	if (tile_count > 0)
	{
		// trigger partial JPEG capture & compression
		if (partialjpeg(info, start_x, start_y, end_x, end_y) == 0)
#if defined(SOC_AST2600)
		{
			memset(&tile_info,0,sizeof(struct ast_videocap_jpeg_tile_info_t));
			tile_info.pos_x = start_x;
			tile_info.pos_y = start_y;
			tile_info.width = width;
			tile_info.height = height;
			tile_info.compressed_size = ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_DATA_COUNT) * AST_VIDEOCAP_COMPRESSED_DATA_COUNT_UNIT;
		}
		else
		{
			tile_info.pos_x = tile_info.pos_y = tile_info.width = tile_info.height = tile_info.compressed_size = 0;
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
		tile_info.resolution_x = info->src_mode.x;
		tile_info.resolution_y = info->src_mode.y;
		tile_info.bpp = block_size;

		if(access_ok(gtileinfo, sizeof(struct ast_videocap_jpeg_tile_info_t)) == 0)
		{
			printk("access ok failed for capture video ioctl \n");
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}

		if (copy_to_user(gtileinfo, &tile_info, sizeof(struct ast_videocap_jpeg_tile_info_t)))
		{
			printk("copy_to_user failed for capture video ioctl \n");
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
#else
		{
			tile_info[0].pos_x = start_x;
			tile_info[0].pos_y = start_y;
			tile_info[0].width = width;
			tile_info[0].height = height;
			tile_info[0].compressed_size = ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_DATA_COUNT) * AST_VIDEOCAP_COMPRESSED_DATA_COUNT_UNIT;
		}
		else
		{
			tile_info[0].pos_x = tile_info[0].pos_y = tile_info[0].width = tile_info[0].height = tile_info[0].compressed_size = 0;
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
#endif
	}

	return tile_count;
}


int VideoCapture(struct ast_videocap_engine_info_t *info)
{
	uint32_t vga_status;
#if !defined(SOC_AST2600)
	struct ast_videocap_jpeg_tile_info_t *tile_info;
	//workaround added
	tile_info = (struct ast_videocap_jpeg_tile_info_t *)gtileinfo;
#else
	int ret = 0;
	struct ast_videocap_jpeg_tile_info_t tile_info;
	
#endif

	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		if (gtileinfo == NULL)
		{
			printk("[%s:%d] Block change info is null!\n", __FUNCTION__, __LINE__);
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
#if !defined(SOC_AST2600)
		tile_info = (struct ast_videocap_jpeg_tile_info_t *)gtileinfo;
#endif
	}

	/* Check if internal VGA screen is off */
	vga_status = ast_videocap_read_reg(AST_VIDEOCAP_VGA_STATUS) >> 24;
	if ((vga_status == 0x10) || (vga_status == 0x11) || (vga_status & 0xC0) || (vga_status & 0x04) || (!(vga_status & 0x3B))) {
		info->NoSignal = 1;
		udelay(100);
		return ASTCAP_IOCTL_BLANK_SCREEN;
	} else {
		info->NoSignal = 0;
	}

	if (ISRDetectedModeOutOfLock) {
		ast_videocap_data_in_old_video_buf = 0;
		if (ast_videocap_mode_detection(info,1) < 0) {
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
	}

	ast_videocap_engine_init(info);
	ast_videocap_horizontal_down_scaling(info);
	ast_videocap_vertical_down_scaling(info);

	if (info->INFData.AutoMode) {
		if (ast_videocap_automode_trigger() < 0) {
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
	} else {
	
		if (ast_videocap_trigger_capture() < 0) {
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}

		// If RC4 is required, here is where we enable it

		if (ast_videocap_trigger_compression() < 0) {
			return ASTCAP_IOCTL_BLANK_SCREEN;
		}
	}

	ast_videocap_data_in_old_video_buf = 1;
	info->FrameHeader.NumberOfMB = ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_BLOCK_COUNT) >> AST_VIDEOCAP_COMPRESSED_BLOCK_COUNT_SHIFT;

	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT) //jpeg
	{
		if (full_screen_capture == 1)
		{
			//Clear BCD buffer by settin 0x1
			memset(AST_VIDEOCAP_FLAG_BUF_ADDR, 0x1, AST_VIDEOCAP_FLAG_BUF_SZ);

			memset(&tile_info,0,sizeof(struct ast_videocap_jpeg_tile_info_t));

			tile_info.pos_x = 0;

			tile_info.pos_y = 0;
			tile_info.width = info->src_mode.x;
			tile_info.height = info->src_mode.y;

			tile_info.compressed_size = ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_DATA_COUNT) * AST_VIDEOCAP_COMPRESSED_DATA_COUNT_UNIT;
#if defined(SOC_AST2600)
			tile_info.resolution_x = info->src_mode.x;
			tile_info.resolution_y = info->src_mode.y;
			tile_info.bpp = (info->FrameHeader.Mode420 != 0) ? BLOCK_SIZE_YUV420 : BLOCK_SIZE_YUV444;
#endif
			ret = access_ok(gtileinfo, sizeof(struct ast_videocap_jpeg_tile_info_t));
			if (ret == 0)
			{
			    printk("access ok failed in get video engine config ioctl \n");
			    return ASTCAP_IOCTL_BLANK_SCREEN;
			}
			
			if (copy_to_user(gtileinfo, &tile_info, sizeof(struct ast_videocap_jpeg_tile_info_t)))
			{
			    printk("copy_to_user failed in get video engine config ioctl\n");
			    return ASTCAP_IOCTL_BLANK_SCREEN;
			}
		}
		else if ((info->FrameHeader.NumberOfMB > 0) && (full_screen_capture == 0))
		{
			//calculate the bonding box ,recapture,get partial jpeg and update the MB
			info->FrameHeader.NumberOfMB = create_bonding_box(AST_VIDEOCAP_FLAG_BUF_ADDR, info);
		}
		else
		{
			// will not execute (just for reference)
		}
	}

	info->CompressData.CompressSize = ast_videocap_read_reg(AST_VIDEOCAP_COMPRESSED_DATA_COUNT) * AST_VIDEOCAP_COMPRESSED_DATA_COUNT_UNIT;

	update_cursor_position();

	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		/* [AST2500] Capture engine busy happens in case of host resolution 1600x900 and higher.
		** so force full screen capture for host video 1600x900 and higher.
		**
		** Issue fixed on AST2600 A1/A2/A3 */
#ifdef SOC_AST2500
		if(info->src_mode.x < 1600)
#endif
		{
			full_screen_capture = 0; /* video driver will capture partial frame next time */
		}

		return (info->FrameHeader.NumberOfMB == 0) ? ASTCAP_IOCTL_NO_VIDEO_CHANGE : ASTCAP_IOCTL_SUCCESS;
	}
	return (info->CompressData.CompressSize == 12) ? ASTCAP_IOCTL_NO_VIDEO_CHANGE : ASTCAP_IOCTL_SUCCESS;
}

int ast_videocap_create_video_packet(struct ast_videocap_engine_info_t *info, void *ioc)
{
	struct ast_videocap_video_hdr_t *video_hdr = NULL;
	ASTCap_Ioctl *ioctl_ptr = (ASTCap_Ioctl *)ioc;
	unsigned long compressed_buf_size;
	unsigned long VideoCapStatus;
	int ret=0;

	if(CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		gtileinfo = ioctl_ptr->vPtr;
	}
	/* Capture Video */
	VideoCapStatus = VideoCapture(info);
	if (VideoCapStatus != ASTCAP_IOCTL_SUCCESS) {
		ioctl_ptr->Size = 0;
		return VideoCapStatus;
	}

	/* Check if Mode changed while capturing */
	if (ISRDetectedModeOutOfLock) {
		/* Send a blank screen this time */
		ioctl_ptr->Size = 0;
		return ASTCAP_IOCTL_BLANK_SCREEN;
	}

	compressed_buf_size = info->CompressData.CompressSize;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
  dmac_inv_range((void *) AST_VIDEOCAP_COMPRESS_BUF_ADDR, (void *) AST_VIDEOCAP_COMPRESS_BUF_ADDR + AST_VIDEOCAP_COMPRESS_BUF_SZ);
#else
#if !defined(SOC_AST2500) && !defined(SOC_AST2530) && !defined(SOC_AST2600)
	dmac_map_area((void *) AST_VIDEOCAP_COMPRESS_BUF_ADDR,AST_VIDEOCAP_COMPRESS_BUF_SZ,DMA_FROM_DEVICE);
    outer_inv_range((unsigned long)AST_VIDEOCAP_COMPRESS_BUF_ADDR,(unsigned long)AST_VIDEOCAP_COMPRESS_BUF_ADDR+AST_VIDEOCAP_COMPRESS_BUF_SZ);
#endif
#endif

	/* fill AST video data header */
	video_hdr = (struct ast_videocap_video_hdr_t *) (AST_VIDEOCAP_HDR_BUF_ADDR);
	video_hdr->iEngVersion = 1;
	video_hdr->wHeaderLen = AST_VIDEOCAP_VIDEO_HEADER_SIZE;
	video_hdr->CompressData_CompressSize = compressed_buf_size;

	video_hdr->SourceMode_X = info->src_mode.x;
	video_hdr->SourceMode_Y = info->src_mode.y;
	video_hdr->DestinationMode_X = info->src_mode.x;
	video_hdr->DestinationMode_Y = info->src_mode.y;

	video_hdr->FrameHdr_RC4Enable = info->FrameHeader.RC4Enable;
	video_hdr->FrameHdr_JPEGScaleFactor = info->FrameHeader.JPEGScaleFactor;
	video_hdr->FrameHdr_Mode420 = info->FrameHeader.Mode420;
	video_hdr->FrameHdr_NumberOfMB = info->FrameHeader.NumberOfMB;
	video_hdr->FrameHdr_AdvanceScaleFactor = info->FrameHeader.AdvanceScaleFactor;
	video_hdr->FrameHdr_JPEGTableSelector = info->FrameHeader.JPEGTableSelector;
	video_hdr->FrameHdr_AdvanceTableSelector = info->FrameHeader.AdvanceTableSelector;
	video_hdr->FrameHdr_JPEGYUVTableMapping = info->FrameHeader.JPEGYUVTableMapping;
	video_hdr->FrameHdr_SharpModeSelection = info->FrameHeader.SharpModeSelection;
	video_hdr->FrameHdr_CompressionMode = info->FrameHeader.CompressionMode;
	video_hdr->FrameHdr_RC4Reset = info->FrameHeader.RC4Reset;

	video_hdr->InfData_DownScalingMethod = info->INFData.DownScalingMethod;
	video_hdr->InfData_DifferentialSetting = info->INFData.DifferentialSetting;
	video_hdr->InfData_AnalogDifferentialThreshold = info->INFData.AnalogDifferentialThreshold;
	video_hdr->InfData_DigitalDifferentialThreshold = info->INFData.DigitalDifferentialThreshold;
	video_hdr->InfData_AutoMode = info->INFData.AutoMode;
	video_hdr->InfData_VQMode = info->INFData.VQMode;

	video_hdr->Cursor_XPos = cursor_pos_x;
	video_hdr->Cursor_YPos = cursor_pos_y;

	ioctl_ptr->Size = compressed_buf_size;

	if(CaptureMode != VIDEOCAP_JPEG_SUPPORT)
	{
		ret = access_ok(ioctl_ptr->vPtr, sizeof(struct ast_videocap_video_hdr_t));
		if (ret == 0)
		{
		    printk("access ok failed in ast_videocap_create_video_packet\n");
		    return ASTCAP_IOCTL_BLANK_SCREEN;
		}
		
		if (copy_to_user(ioctl_ptr->vPtr, video_hdr, sizeof(struct ast_videocap_video_hdr_t)))
		{
		    printk("copy_to_user failed in ast_videocap_create_video_packet\n");
		    return ASTCAP_IOCTL_BLANK_SCREEN;
		}
	}
	return ASTCAP_IOCTL_SUCCESS;
}
