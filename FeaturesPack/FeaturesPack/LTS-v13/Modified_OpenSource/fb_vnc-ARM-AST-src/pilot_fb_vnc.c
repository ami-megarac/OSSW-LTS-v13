/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2018, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/

/****************************************************************
 *
 * pilot_fb.c
 * PILOT frame buffer driver
 *
 ****************************************************************/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/selection.h>
#include <linux/bigphysarea.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/vt_kern.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include <linux/platform_device.h>


#ifdef  DEFINE_VIDEODATA
#define EXTERN
#else
#define EXTERN extern
#endif
#include "pilot_fb_vnc.h"
#include "pilot_fb_vnc_mode.h"
#include "fb_vnc_ioctl.h"

#define PILOT_FB_DRV_NAME "pilotfb"
extern int pilot_fb_set_mode(struct fb_var_screeninfo *var, struct pilot_info *dinfo);
extern int pilot_fb_get_mode_info(struct fb_var_screeninfo *var, struct pilot_info *dinfo, struct pilot_fb_mode_info_t *mode_info);
uint32_t  GetPhyFBStartAddress( void );
struct pilot_info *dinfo;

static int ypan = 0;

struct pilotvga_par {
	struct platform_device	*pdev;
	struct fb_info			*info;
	void __iomem *video_base;
	u32 pseudo_palette[17];

};

/*
 * Here we define the default structs fb_fix_screeninfo and fb_var_screeninfo
 * if we don't use modedb. If we do use modedb see pilotvgafb_init how to use it
 * to get a fb_var_screeninfo. Otherwise define a default var as well.
 */
static struct fb_fix_screeninfo pilotvgafb_fix = {
		.id =		PILOT_FB_DRV_NAME,
		.type =		FB_TYPE_PACKED_PIXELS,
		.visual =	FB_VISUAL_PSEUDOCOLOR,
		.xpanstep =	1,
		.ypanstep =	1,
		.ywrapstep =	1,
		.accel =	FB_ACCEL_NONE,
};

static int pilotfb_set_fix(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix;
	struct pilot_info *dinfo = GET_DINFO(info);

	fix = &(info->fix);
	memset(fix, 0, sizeof(struct fb_fix_screeninfo));
	strcpy(fix->id, dinfo->name);
	fix->smem_start = dinfo->frame_buf_phys;
	fix->smem_len = dinfo->frame_buf_sz;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->type_aux = 0;
	fix->visual = (dinfo->bpp == 8) ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;
	fix->xpanstep = 0;
	fix->ypanstep = ypan ? 1 : 0;
	fix->ywrapstep = 0;
	fix->line_length = dinfo->pitch;
	fix->mmio_start = dinfo->ulMMIOPhys;
	fix->mmio_len = dinfo->ulMMIOSize;
	fix->accel = FB_ACCEL_NONE;

	return 0;
}

static int pilotfb_blank(int blank, struct fb_info *info)
{
	return 0;
}

static int pilotfb_pan_display(struct fb_var_screeninfo *var, struct fb_info* info)
{
	struct pilot_info *dinfo = GET_DINFO(info);
	uint32_t base;
	u32 xoffset, yoffset;

	xoffset = (var->xoffset + 3) & ~3; /* DW alignment */
	yoffset = var->yoffset;

	if ((xoffset + var->xres) > var->xres_virtual) {
		return -EINVAL;
	}

	if ((yoffset + var->yres) > var->yres_virtual) {
		return -EINVAL;
	}

	info->var.xoffset = xoffset;
	info->var.yoffset = yoffset;

	base = (var->yoffset * var->xres_virtual) + var->xoffset;

	/* calculate base bpp depth */
	switch(var->bits_per_pixel) {
	case 32:
		break;
	case 16:
		base >>= 1;
		break;
	case 8:
	default:
		base >>= 2;
		break;
	}

	//use pilot register to set HERE

	/* set starting address of frame buffer */
	iowrite32(dinfo->frame_buf_phys + base, dinfo->io );
	return 0;
}


static int pilotfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct pilot_info *dinfo = GET_DINFO(info);
	struct pilotfb_dfbinfo dfbinfo;
	struct pilotfb_dispsrc dispsrc;

	switch(cmd) {
	case AMIFB_GET_DFBINFO:
		dfbinfo.ulFBSize = dinfo->frame_buf_sz;
		dfbinfo.ulFBPhys = dinfo->frame_buf_phys;
		dfbinfo.ulCMDQSize = dinfo->cmd_q_sz;
		dfbinfo.ulCMDQOffset = dinfo->cmd_q_offset;
		//dfbinfo.ul2DMode = dinfo->use_2d_engine;
		if (copy_to_user((void __user *)arg, &dfbinfo, sizeof(struct pilotfb_dfbinfo)))
			return -EFAULT;
		return 0;

	case AMIFB_SET_DISPSRC:
		if (copy_from_user((void*)&dispsrc.val, (void*)arg, sizeof(u32)))
		{
			printk("PILOTFB_SET_DFB_DACSRC: Error copying data from user!\n");
			return -EFAULT;
		}
		break;

	case AMIFB_GET_DISPSRC:
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int pilotfb_get_cmap_len(struct fb_var_screeninfo *var)
{
	return (var->bits_per_pixel == 8) ? 256 : 16;
}

static int pilotfb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue, unsigned transp, struct fb_info *info)
{
	if (regno >= pilotfb_get_cmap_len(&info->var))
		return 1;

	switch(info->var.bits_per_pixel) {
	case 8:
		return 1;
		break;
	case 16:
		((u32 *) (info->pseudo_palette))[regno] = (red & 0xf800) | ((green & 0xfc00) >> 5) | ((blue & 0xf800) >> 11);
		break;
	case 32:
		red >>= 8;
		green >>= 8;
		blue >>= 8;
		((u32 *)(info->pseudo_palette))[regno] =
				(red << 16) | (green << 8) | (blue);
		break;
	}

	return 0;
}

static int pilotfb_set_par(struct fb_info *info)
{

	pilotfb_set_fix(info);

	return 0;
}

static int pilotfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
#if 0
struct pilot_info *dinfo = GET_DINFO(info);
struct ast_fb_mode_info_t mode_info;

if (pilot_fb_get_mode_info(var, dinfo, &mode_info)) {
	printk(KERN_ERR "astfb: check mode failed \n");
	return 1;
}

/* Sanity check for offsets */
if (var->xoffset < 0)
	var->xoffset = 0;
if (var->yoffset < 0)
	var->yoffset = 0;

if (var->xres > var->xres_virtual)
	var->xres_virtual = var->xres;

/* Truncate offsets to maximum if too high */
if (var->xoffset > var->xres_virtual - var->xres) {
	var->xoffset = var->xres_virtual - var->xres - 1;
}

if (var->yoffset > var->yres_virtual - var->yres) {
	var->yoffset = var->yres_virtual - var->yres - 1;
}

switch (var->bits_per_pixel) {
case 8:
	var->red.offset = var->green.offset = var->blue.offset = 0;
	var->red.length = var->green.length = var->blue.length = 6;
	break;
case 16:
	var->red.offset = 11;
	var->red.length = 5;
	var->green.offset = 5;
	var->green.length = 6;
	var->blue.offset = 0;
	var->blue.length = 5;
	var->transp.offset = 0;
	var->transp.length = 0;
	break;
case 32:
	var->red.offset = 16;
	var->red.length = 8;
	var->green.offset = 8;
	var->green.length = 8;
	var->blue.offset = 0;
	var->blue.length = 8;
	var->transp.offset = 24;
	var->transp.length = 8;
	break;
}

/* Set everything else to 0 */
var->red.msb_right = 0;
var->green.msb_right = 0;
var->blue.msb_right = 0;
var->transp.offset = 0;
var->transp.length = 0;
var->transp.msb_right = 0;
#endif
return 0;
}

static struct fb_ops pilotvgafb_ops = {
		.owner          = THIS_MODULE,
		.fb_check_var   = pilotfb_check_var,
		.fb_set_par     = pilotfb_set_par,
		.fb_setcolreg   = pilotfb_setcolreg,
		.fb_blank       = pilotfb_blank,
		.fb_pan_display = pilotfb_pan_display,
		.fb_fillrect    = cfb_fillrect,
		.fb_copyarea    = cfb_copyarea,
		.fb_imageblit   = cfb_imageblit,
		.fb_ioctl       = pilotfb_ioctl
};

static void cleanup(struct pilot_info *dinfo)
{
	if (!dinfo)
		return;

	if (dinfo->frame_buf != NULL)
		iounmap(dinfo->frame_buf);

	if (dinfo->registered) {
		unregister_framebuffer(dinfo->info);
		framebuffer_release(dinfo->info);
	}

	dev_set_drvdata(dinfo->dev, NULL);
}

static void pilot_release(struct device *dev)
{
}

static int pilotvgafb_probe(struct platform_device *pdev)
{
	struct fb_info *info;
	struct pilotvga_par *par;
	struct device *device = &pdev->dev; /* or &pdev->dev */
	/*
	 * * Dynamically allocate info and par
	 */

	info = framebuffer_alloc(sizeof(struct pilotvga_par), device);

	if (!info) {
		/* goto error path */
		printk(KERN_ERR "Could not allocate memory for pilot_info.\n");
		return -ENODEV;
	}

	// init device info.
	dinfo = (struct pilot_info *) info->par;
	if (!dinfo) {
		/* goto error path */
		printk(KERN_ERR "Could not allocate memory for dinfo.\n");
		return -ENODEV;
	}
	memset(dinfo, 0, sizeof(struct pilot_info));
	dinfo->info = info;
	dinfo->dev = device;
	strcpy(dinfo->name, PILOT_FB_DRV_NAME);
	dev_set_drvdata(device, (void *) dinfo);
	par = info->par;
	//par->video_base = ioremap(0x80000000, 0x400);

	/*
	 * Here we set the screen_base to the virtual memory address
	 * for the framebuffer. Usually we obtain the resource address
	 * from the bus layer and then translate it to virtual memory
	 * space via ioremap. Consult ioport.h.
	 */
	printk("map to vga memory \n");
	dinfo->frame_buf = ioremap(GetPhyFBStartAddress(), dinfo->frame_buf_sz);//framebuffer_virtual_memory; 16MB VGA
	dinfo->frame_buf_phys = dinfo->frame_buf_sz;
	info->screen_base = dinfo->frame_buf;
	info->fbops = &pilotvgafb_ops;
	info->fix = pilotvgafb_fix;

	info->fix.smem_start = GetPhyFBStartAddress();
	info->fix.smem_len = dinfo->frame_buf_phys;

	info->pseudo_palette = par->pseudo_palette;
	info->flags = FBINFO_DEFAULT;


	if (register_framebuffer(info) < 0) {
		fb_dealloc_cmap(&info->cmap);
		cleanup(dinfo);
		return -EINVAL;
	}
	dinfo->registered = 1;
	device->release = pilot_release;

	fb_info(info, "%s frame buffer device\n", info->fix.id);
	printk(KERN_INFO "FB: got physical memory pool for size (%d on %08lx bus)\n", info->fix.smem_len, (unsigned long)info->fix.smem_start);
	platform_set_drvdata(pdev, info); /* or platform_set_drvdata(pdev, info) */
	return 0;
}


static int pilotvgafb_remove(struct platform_device *pdev)
{
	struct fb_info *info = platform_get_drvdata(pdev);
	/* or platform_get_drvdata(pdev); */
	if (info) {
		unregister_framebuffer(info);
		//fb_dealloc_cmap(&info->cmap);
		/* ... */
		framebuffer_release(info);
	}
	return 0;
}

// #ifdef CONFIG_PM
// /**
//  *	pilotvgafb_resume - Optional but recommended function. Resume the device.
//  *	@dev: platform device
//  *
//  *      See Documentation/power/devices.txt for more information
//  */
// static int pilotvgafb_resume(struct platform_device *dev)
// {
// 	struct fb_info *info = platform_get_drvdata(dev);
// 	// struct pilotvgafb_par *par = info->par;

// 	/* resume here */
// 	return 0;
// }
// static int pilotvgafb_suspend(struct platform_device *dev)
// {
// 	return 0;
// }
// #else
// #define pilotvgafb_suspend NULL
// #define pilotvgafb_resume NULL
// #endif /* CONFIG_PM */

static struct platform_driver pilotvgafb_driver = {
		.probe = pilotvgafb_probe,
		.remove = pilotvgafb_remove,
		// .suspend = pilotvgafb_suspend, /* optional but recommended */
		// .resume = pilotvgafb_resume,   /* optional but recommended */
		.driver = {
				.name = PILOT_FB_DRV_NAME,
		},
};


static struct platform_device pilotfb_device = {
		.name = PILOT_FB_DRV_NAME,
};

int __init pilotfb_init(void)
{
	int ret;
	ret = platform_driver_register(&pilotvgafb_driver);

	if (!ret) {
		ret = platform_device_register(&pilotfb_device);
		if (ret)
			platform_driver_unregister(&pilotvgafb_driver);
	}
	return ret;
}

static void __exit pilotfb_exit(void)
{
	cleanup(dinfo);
	platform_device_unregister(&pilotfb_device);
	platform_driver_unregister(&pilotvgafb_driver);
}

uint32_t  GetPhyFBStartAddress( void ) {
	unsigned long DRAM_Data = 0;
	unsigned long TotalMemory = 0;
	unsigned long VGAMemory = 0;
	long ast_sdram_reg_virt_base;
	// unsigned long video_buffer_offset = 0;
	
	// if (current_video_mode.video_mode_flags == MGA_MODE ) {
	// 	video_buffer_offset = get_screen_offset(&current_video_mode);
	// }
	if ( !( ast_sdram_reg_virt_base = (unsigned long)ioremap(0x1E6E0000, SZ_128K) ) )
	{
		printk(KERN_WARNING "ast_videocap: ast_sdram_reg_virt_base ioremap failed\n");
		return -ENOMEM;
	}

	DRAM_Data = ioread32((void * __iomem)ast_sdram_reg_virt_base + 0x04);
	switch (DRAM_Data & 0x03) {
	case 0:
		TotalMemory = 0x10000000;
		break;
	case 1:
		TotalMemory = 0x20000000;
		break;
	case 2:
		TotalMemory = 0x40000000;
		break;
	case 3:
		TotalMemory = 0x80000000;
		break;
	}

	switch ((DRAM_Data >> 2) & 0x03) {
	case 0:
		VGAMemory = 0x800000;
		break;
	case 1:
		VGAMemory = 0x1000000;
		break;
	case 2:
		VGAMemory = 0x2000000;
		break;
	case 3:
		VGAMemory = 0x4000000;
		break;
	}
	dinfo->frame_buf_sz = VGAMemory;
	if ( !ast_sdram_reg_virt_base ){
		iounmap((void*)ast_sdram_reg_virt_base);
	}

	return ( (PILOT_GRAPHICS_REG_BASE+TotalMemory - VGAMemory) ) ;
}
module_init(pilotfb_init);
module_exit(pilotfb_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("PILOT frame buffer driver");
MODULE_LICENSE("GPL");
