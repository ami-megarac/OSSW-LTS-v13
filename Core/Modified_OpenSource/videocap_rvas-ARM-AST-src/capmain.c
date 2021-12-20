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
* @file   	capmain.c
* @author 	Varadachari Sudan Ayanam <varadacharia@ami.com>
* @brief  	capture driver main file
****************************************************************/

/* Video Capture Module Version */
#define PILOT_ENGINE_MAJOR	15
#define PILOT_ENGINE_MINOR	0	/* 0 for 2.4 and 1 for 2.6 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/uaccess.h>	/* copyin and copyout   */
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/bigphysarea.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <linux/device.h>
//#include <mach/platform.h>

#include <linux/interrupt.h>

#include "helper.h" /* symbols from helper module */
#include "dbgout.h"

#include "videocap.h"
#include "iohndlr.h"
#include "ioctl.h"
#include "proc.h"
#include "mmap.h"
#include "cap90xx.h"
#include "profile.h"

#define DEFINE_VIDEODATA	1

/* Function Prototypes */
static int videocap_open (struct inode *inode, struct file *file);
static int videocap_release (struct inode *inode, struct file *file);

/* Data Structures */
#include "videodata.h"

extern irqreturn_t pilot_fgb_intr(int irq, void *dev_id);
int enable_rvas_engines(void);
void disable_rvas_engines(void);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
struct proc_info *statisticsproc;
struct proc_info *statusproc;
struct proc_info *infoproc;
struct proc_info *regproc;
struct proc_info *paletteproc;
struct proc_info *videoproc;
struct proc_info *tfebuffproc;
struct proc_info *deccolproc;
struct proc_info *decrowproc;
struct proc_info *decmaxproc;
#endif

static dev_t pilot_engine_devno = MKDEV(PILOT_ENGINE_MAJOR, PILOT_ENGINE_MINOR);
static struct cdev pilot_engine_cdev;
tm_entry_t *tile_map;
struct device_node *dt_node;
// struct reset_control *reset;

/* Sysctl Table 8 */
static struct ctl_table VideoTable [] =
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	{.procname="DebugLevel",	.data=&(TDBG_FORM_VAR_NAME(videocap)),	.maxlen=sizeof(int),			.mode=0644,.proc_handler=&proc_dointvec},
	{.procname="TileColumnSize",.data=&TileColumnSize,					.maxlen=sizeof(unsigned long),	.mode=0644,.proc_handler=&proc_doulongvec_minmax},
	{.procname="TileRowSize",	.data=&TileRowSize,						.maxlen=sizeof(unsigned long),	.mode=0644,.proc_handler=&proc_doulongvec_minmax},
	{.procname="MaxRectangles",	.data=&MaxRectangles,					.maxlen=sizeof(unsigned long),	.mode=0644,.proc_handler=&proc_doulongvec_minmax},
#else

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	{1,"DebugLevel",&(TDBG_FORM_VAR_NAME(videocap)),sizeof(int),0644,NULL,NULL,&proc_dointvec},
	{2,"TileColumnSize",&TileColumnSize,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
	{3,"TileRowSize",&TileRowSize,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
	{4,"MaxRectangles",&MaxRectangles,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
#else
	{"DebugLevel",&(TDBG_FORM_VAR_NAME(videocap)),sizeof(int),0644,NULL,&proc_dointvec},
	{"TileColumnSize",&TileColumnSize,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
	{"TileRowSize",&TileRowSize,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
	{"MaxRectangles",&MaxRectangles,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
#endif
	{0}
#endif
};

/* Proc and Sysctl entries */
static struct proc_dir_entry *moduledir = NULL;
static struct ctl_table_header *my_sys 	= NULL;

/* Driver Information Table */
static struct file_operations videocap_fops =
{
	owner:		THIS_MODULE,
#ifdef USE_UNLOCKED_IOCTL
	unlocked_ioctl:		videocap_ioctl,
#else
	ioctl:		videocap_ioctl,
#endif
	open:		videocap_open,
	release:	videocap_release,
	mmap:		videocap_mmap,
};


static int opencount = 0;


static
int
videocap_open (struct inode *inode, struct file *file)
{
	// MOD_INC_USE_COUNT;
	/* First Open, Initialize everything */
	if (opencount == 0) {
		// just clears some unused variables
		ReInitializeDriver(); 
	}

	opencount++;
	return 0;
}

static
int
videocap_release (struct inode *inode, struct file *file)
{
	// MOD_DEC_USE_COUNT;
	opencount --;

	/* Last Release. Clear EveryThing */
	if (opencount == 0) {
		ReInitializeDriver();
	}

	return 0;
}


static
int
AllocateResources (void)
{
	int ret = 0;
	
	pilot_fg_base = (unsigned long)ioremap(PILOT_FG_BASE, PILOT_FG_BASE_SIZE);
	if (!pilot_fg_base) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}

	grc_base = (unsigned long)ioremap(GRCE_BASE, GRCE_BASE_SIZE);
	if (!grc_base) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}

	tfe_descriptor_base = (unsigned long)ioremap(LMEMSTART, SZ_8K);
	if ( !tfe_descriptor_base ) {
		printk(KERN_WARNING "%s: ioremap failed for LMEM \n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}

	bse_descriptor_base = (unsigned long)ioremap(BSE_BASE, BSE_BASE_SIZE);
	if ( !bse_descriptor_base) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}

	dp_descriptor_base = (unsigned long)ioremap(DP_BASE, SZ_512);
	if ( !dp_descriptor_base) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}

	descaddr = (unsigned long)(tfe_descriptor_base);
	bsedescaddr = (unsigned long)(tfe_descriptor_base);

	tfedest_base = NULL; 
	tfedest_base = dma_alloc_coherent(NULL, (size_t)BIGPHYS_SIZE, &tfedest_bus, GFP_KERNEL);
	if (!tfedest_base) {
	    TCRIT("Could not allocate the Physical memory pool for Videocap\n");
	    ret = -ENOMEM;
		goto out_iomap;
	}

	tmpbuffer_base = NULL;
	tmpbuffer_base = dma_alloc_coherent(NULL, (size_t)BIGPHYS_SIZE, &tmpbuffer_bus, GFP_KERNEL);
	if (!tmpbuffer_base) {
		TCRIT("Could not allocate the Physical memory pool for Videocap\n");
		ret = -ENOMEM;
		goto out_iomap;
	}

	ast_scu_reg_virt_base = (unsigned long)ioremap(0x1E6E2000, SZ_128K);
	if ( !ast_scu_reg_virt_base )
	{
		printk(KERN_WARNING "vidoecap_pilot: ast_scu_reg_virt_base ioremap failed\n");
		ret = -ENOMEM;
		goto out_iomap;
	}
	if ( !( ast_sdram_reg_virt_base = (unsigned long)ioremap(0x1E6E0000, SZ_128K) ) )
	{
		printk(KERN_WARNING "ast_videocap: ast_sdram_reg_virt_base ioremap failed\n");
		ret = -ENOMEM;
		goto out_iomap;
	}

	/* Initialize the Register Locations */
	fgb_top_regs = (volatile FGB_TOP_REG_STRUCT *)(pilot_fg_base);
	fgb_tse_regs = (volatile TSE_REG_STRUCT *)(pilot_fg_base + 0x400);
	fgb_tfe_regs = (volatile TFE_REG_STRUCT *)(pilot_fg_base + 0x100);
	fgb_tse_rr_base = (volatile unsigned long *)(pilot_fg_base + 0x600);
	fgb_bse_regs = (volatile BSE_REG_STRUCT *)(pilot_fg_base + 0x200);
	grc_ctl_regs = (volatile GRC_STRUCT *)(grc_base + GRC_OFFSET);
	grc_regs = (volatile GRC_REGS_STRUCT *)(grc_base + GRC_REGS_STRUCT_OFFSET);
	crtc = (volatile CRTC0_REG_STRUCT * )(grc_base +  CRTC0_OFFSET);
	crtcext =  (volatile CRTCEXT_REG_STRCT *)(grc_base +  CRTCEXT_OFFSET);
	crtc_seq =  (volatile CRTC_SEQ_REG_STRUCT *)(grc_base +  CRTC_SEQ_OFFSET);
	crtc_gctl =  (volatile CRTC_GCTL_REG_STRUCT *)(grc_base +  CRTC_GCTL_OFFSET);
	attr =  (volatile ATTR_REG_STRUCT *)(grc_base +  ATTR0_OFFSET /* 0x0*/);
	XMULCTRL = (volatile unsigned char *)(grc_base +  0xC8 /* VGACRC8-CA 0xC8 - 0xCA */);
	tile_map = (tm_entry_t *)kmalloc(64 * 64 * sizeof(tm_entry_t), GFP_KERNEL);
	if (tile_map == NULL) {
		printk("\n KMalloc failed for VIDEO CAP \n");
		ret = -ENOMEM;
		goto out_iomap;
	}

	pilot_fgb_irq = 98;
	PollMode = 0;

	pilot_fgb_irq = GetIrqFromDT("ami_videocap", pilot_fgb_irq);

	/* Request IRQ */
	if (!PollMode)
	{
		if (request_irq(pilot_fgb_irq, pilot_fgb_intr, 0, "PILOTFGB", NULL))
		{
			TWARN("Request IRQ of PILOT FRAME GRABBER IRQ Failed. Running in Poll Mode\n");
			PollMode = 1;
		}
	}

	if ( enable_rvas_engines() != 0 ) {
		ret = -ENOMEM;
		goto out_iomap;
	}

	//set LMEM
	iowrite32(0x0,(void * __iomem)pilot_fg_base + LMEM_BASE_REG_3 );
	iowrite32(0x2000,(void * __iomem)pilot_fg_base + LMEM_LIMIT_REG_3 );
	iowrite32(0x9c89c8,(void * __iomem)pilot_fg_base + LMEM11_P0 );
	iowrite32(0x9c89c8,(void * __iomem)pilot_fg_base + LMEM12_P0 );
	iowrite32(0xf3cf3c,(void * __iomem)pilot_fg_base + LMEM11_P1 );
	iowrite32(0x067201,(void * __iomem)pilot_fg_base + LMEM11_P2 );
	iowrite32(0x00F3CF3C,(void * __iomem)pilot_fg_base + LMEM12_P1 );
	iowrite32(0x00067201,(void * __iomem)pilot_fg_base + LMEM12_P2 );
	// display_cntrl = (volatile unsigned long *)(ast_scu_reg_virt_base + DISP_CNTRL_OFFSET);
	printk("\n FRAMEBUFFER VALUE === %#x \n",GetPhyFBStartAddress() );
	framebuffer_base = (unsigned long)ioremap(GetPhyFBStartAddress(), FRAMEBUFFER_SIZE);
	if (!pilot_fg_base) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ENGINE_DRV_NAME);
		ret = -ENOMEM;
		goto out_iomap;
	}
out_iomap:
	if( ret != 0 )
	{
		if ( !pilot_fg_base )
			iounmap((void*)pilot_fg_base);
		if ( !grc_base )
			iounmap((void*)grc_base);
		if ( !framebuffer_base )
			iounmap((void*)framebuffer_base);
		if ( !tfe_descriptor_base )
			iounmap((void*)tfe_descriptor_base);
		if ( !bse_descriptor_base )
			iounmap((void*)bse_descriptor_base);
		if ( !ast_scu_reg_virt_base )
			iounmap((void*)ast_scu_reg_virt_base);
		if ( !ast_sdram_reg_virt_base )
			iounmap((void*)ast_sdram_reg_virt_base);
		if ( !dp_descriptor_base)
			iounmap((void*)dp_descriptor_base);
		if (tfedest_base) {
			dma_free_coherent(NULL, (size_t)BIGPHYS_SIZE, tfedest_base, tfedest_bus);
		}
		if (tmpbuffer_base) {
			dma_free_coherent(NULL, (size_t)BIGPHYS_SIZE, tmpbuffer_base, tmpbuffer_bus);
		}
		if (!tile_map) {
			kfree(tile_map);
		}
	}

	return ret ;
}

// Called when insmod/modprobe is invoked
int
init_videocap (void)
{
	int ret = 0;
	/* Initialize Data */
	pilot_fg_base = 0;
	pilot_fgb_irq  = 0;
	PollMode    = 0;

	/* Tuneable Parameters */
	TileColumnSize = 32 ; // 32 pixels
	TileRowSize = 32 ; // 32 lines
	MaxRectangles = 1 ;

	/* Register the driver */
	ret = register_chrdev_region(pilot_engine_devno, PILOT_ENGINE_DEV_NUM, PILOT_ENGINE_DEV_NAME);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to register char device\n", PILOT_ENGINE_DEV_NAME);
		goto RegisterFailed;
	}
	cdev_init(&pilot_engine_cdev, &videocap_fops);

	ret = cdev_add(&pilot_engine_cdev, pilot_engine_devno, PILOT_ENGINE_DEV_NUM);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to add char device\n", PILOT_ENGINE_DEV_NAME);
		goto out_cdev_register;
	}

	/* Create this module's directory entry in proc and add a file "DriverStatus" */
	moduledir = proc_mkdir("videocap",rootdir);


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
	statisticsproc = AddProcEntry(moduledir,"Statistics",ReadDriverStatistics,NULL,NULL);
	statusproc = AddProcEntry(moduledir,"Status",ReadDriverStatus,NULL,NULL);
	infoproc = AddProcEntry(moduledir,"Info",ReadDriverInfo,NULL,NULL);
	regproc = AddProcEntry(moduledir,"Registers",DumpRegisters,NULL,NULL);
	paletteproc = AddProcEntry(moduledir,"Palette",DumpPalette,NULL,NULL);
	videoproc = AddProcEntry(moduledir,"VideoBuff",DumpVideo,NULL,NULL);
	tfebuffproc = AddProcEntry(moduledir,"TfeBuff",DumpTfeBuff,NULL,NULL);
	deccolproc = AddProcEntry(moduledir,"DecColSize",DECTSEColumnSize,NULL,NULL);
	decrowproc = AddProcEntry(moduledir,"DecRowSize",DECTSERowSize,NULL,NULL);
	decmaxproc = AddProcEntry(moduledir,"DecMaxRect",DECTSEMaxRectangles,NULL,NULL);
#else
	AddProcEntry(moduledir,"Statistics",ReadDriverStatistics,NULL,NULL);
	AddProcEntry(moduledir,"Status",ReadDriverStatus,NULL,NULL);
	AddProcEntry(moduledir,"Info",ReadDriverInfo,NULL,NULL);
	AddProcEntry(moduledir,"Registers",DumpRegisters,NULL,NULL);
	AddProcEntry(moduledir,"Palette",DumpPalette,NULL,NULL);
	AddProcEntry(moduledir,"VideoBuff",DumpVideo,NULL,NULL);
	AddProcEntry(moduledir,"TfeBuff",DumpTfeBuff,NULL,NULL);
	AddProcEntry(moduledir,"DecColSize",DECTSEColumnSize,NULL,NULL);
	AddProcEntry(moduledir,"DecRowSize",DECTSERowSize,NULL,NULL);
	AddProcEntry(moduledir,"DecMaxRect",DECTSEMaxRectangles,NULL,NULL);
#endif

	/* Add sysctl to access the videocap */
	my_sys  = AddSysctlTable("videocap",&VideoTable[0]);

	/* Request Resources - IRQ */
	if (AllocateResources() != 0)
		goto AllocResFailed;

	return 0;

AllocResFailed:
	/* Remove driver related sysctl entries */
	RemoveSysctlTable(my_sys);

	/* Remove the driver's proc entries */
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
	RemoveProcEntry(statisticsproc);
	RemoveProcEntry(statusproc);
	RemoveProcEntry(infoproc);
	RemoveProcEntry(regproc);
	RemoveProcEntry(paletteproc);
	RemoveProcEntry(videoproc);
	RemoveProcEntry(tfebuffproc);
	RemoveProcEntry(deccolproc);
	RemoveProcEntry(decrowproc);
	RemoveProcEntry(decmaxproc);
#else
	RemoveProcEntry(moduledir,"Info");
	RemoveProcEntry(moduledir,"Status");
	RemoveProcEntry(moduledir,"Statistics");
	RemoveProcEntry(moduledir,"Registers");
	RemoveProcEntry(moduledir,"Palette");
	RemoveProcEntry(moduledir,"VideoBuff");
	RemoveProcEntry(moduledir,"TfeBuff");
	RemoveProcEntry(moduledir,"DecColSize");
	RemoveProcEntry(moduledir,"DecRowSize");
	RemoveProcEntry(moduledir,"DecMaxRect");
#endif

	remove_proc_entry("videocap",rootdir);

	/* Unregister the driver */
	unregister_chrdev(PILOT_ENGINE_MAJOR,"videocap");
// out_cdev_add:
	cdev_del(&pilot_engine_cdev);
out_cdev_register:
	unregister_chrdev_region(pilot_engine_devno, PILOT_ENGINE_DEV_NUM);
RegisterFailed:
	return 1;
}


void
exit_videocap (void)
{
	TDBG_FLAGGED(videocap,DBG_MAIN,"Unloading Video Capture Driver ..............\n");
	disable_rvas_engines();

	printk("videocap exiting\n");
	/* Release Resources */
	if (!PollMode){
		printk("\n Release IRQ \n");
		free_irq(pilot_fgb_irq, NULL);
	}
	if ( !pilot_fg_base )
		iounmap((void*)pilot_fg_base);
	if ( !grc_base )
		iounmap((void*)grc_base);
	if ( !framebuffer_base )
		iounmap((void*)framebuffer_base);
	if ( !tfe_descriptor_base )
		iounmap((void*)tfe_descriptor_base);
	if ( !bse_descriptor_base )
		iounmap((void*)bse_descriptor_base);
	if ( !dp_descriptor_base)
		iounmap((void*)dp_descriptor_base);
	if ( !ast_scu_reg_virt_base )
		iounmap((void*)ast_scu_reg_virt_base);
	if ( !ast_sdram_reg_virt_base )
		iounmap((void*)ast_sdram_reg_virt_base);
	if (tfedest_base) {
		dma_free_coherent(NULL, (size_t)BIGPHYS_SIZE, tfedest_base, tfedest_bus);
	}
	if (tmpbuffer_base) {
		dma_free_coherent(NULL, (size_t)BIGPHYS_SIZE, tmpbuffer_base, tmpbuffer_bus);
	}

	if (!tile_map) {
		kfree(tile_map);
	}

	/* Remove driver related sysctl entries */
	if( my_sys != NULL )
		RemoveSysctlTable(my_sys);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  RemoveProcEntry(statisticsproc);
  RemoveProcEntry(statusproc);
  RemoveProcEntry(infoproc);
  RemoveProcEntry(regproc);
  RemoveProcEntry(paletteproc);
  RemoveProcEntry(videoproc);
  RemoveProcEntry(tfebuffproc);
  RemoveProcEntry(deccolproc);
  RemoveProcEntry(decrowproc);
  RemoveProcEntry(decmaxproc);
#else
  RemoveProcEntry(moduledir,"Info");
  RemoveProcEntry(moduledir,"Status");
  RemoveProcEntry(moduledir,"Statistics");
  RemoveProcEntry(moduledir,"Registers");
  RemoveProcEntry(moduledir,"Palette");
  RemoveProcEntry(moduledir,"VideoBuff");
  RemoveProcEntry(moduledir,"TfeBuff");
  RemoveProcEntry(moduledir,"DecColSize");
  RemoveProcEntry(moduledir,"DecRowSize");
  RemoveProcEntry(moduledir,"DecMaxRect");
#endif
	
  remove_proc_entry("videocap",rootdir);

	/* Unregister the driver */
	// unregister_chrdev(PILOT_ENGINE_MAJOR,"videocap");
	cdev_del(&pilot_engine_cdev);
	unregister_chrdev_region(pilot_engine_devno, PILOT_ENGINE_DEV_NUM);

	TDBG_FLAGGED(videocap,DBG_MAIN,"Unloading Video Capture Driver Completed \n");
}

void disable_rvas_engines()
{
	u32 reg_val = 0;

	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x00); /* read SCU */

	if( reg_val == 0 ) {
		iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	}

	iowrite32(0x2000000, (void *__iomem)ast_scu_reg_virt_base + 0x84);
	iowrite32(0, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* lock SCU */
}


int enable_rvas_engines()
{
	u32 reg_val = 0;
	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x0); /* read SCU */

	if( reg_val == 0 ) {
    	iowrite32(0x1688A8A8, (void * __iomem)ast_scu_reg_virt_base + 0x00); /* unlock SCU */
	}
	
	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x80);

	if( reg_val & 0x2000000 ) { // 25th bit in SCU080  is zero
		iowrite32(0x200,(void * __iomem)ast_scu_reg_virt_base + 0x040 );
		udelay(100);
		iowrite32(0x2000000, (void *__iomem)ast_scu_reg_virt_base + 0x84);
		mdelay(10);
	}

	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x40 );
	if( reg_val & 0x200 ) {
		iowrite32(0x200,(void * __iomem)ast_scu_reg_virt_base + 0x044 );
	}

	reg_val = ioread32((void *__iomem)ast_scu_reg_virt_base + 0x40 );
	
	if ((reg_val & 0x200) != 0) {
		printk("RVAS Engine cannot be unlocked\n");
		return -1;
	}
	return 0;
}

module_init(init_videocap);
module_exit(exit_videocap);

/*--------------------- Module Information Follows ------------------------*/
MODULE_AUTHOR ("Varadachari Sudan - American Megatrends Inc");
MODULE_DESCRIPTION ("Video Capture Driver");
MODULE_LICENSE ("GPL");

