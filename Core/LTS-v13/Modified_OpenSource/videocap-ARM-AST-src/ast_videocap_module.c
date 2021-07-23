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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/aio.h>
#include <linux/bigphysarea.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#ifndef SOC_AST2600
#include <mach/platform.h>
#else
// #include <linux/reset.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#endif

#include "helper.h"
#include "ast_videocap_data.h"
#include "ast_videocap_hw.h"
#include "ast_videocap_functions.h"


#define AST_VIDEOCAP_DEV_NAME	"videocap"
#define AST_VIDEOCAP_DRV_NAME	"ast_videocap"
#define AST_VIDEOCAP_DEV_MAJOR	15
#define AST_VIDEOCAP_DEV_MINOR	0
#define AST_VIDEOCAP_DEV_NUM	1

static dev_t ast_videocap_devno = MKDEV(AST_VIDEOCAP_DEV_MAJOR, AST_VIDEOCAP_DEV_MINOR);
static struct cdev ast_videocap_cdev;

void *ast_videocap_reg_virt_base;
void *ast_videocap_video_buf_virt_addr;
void *ast_videocap_video_buf_phys_addr;

#ifdef SOC_AST2600
#define DP_BASE							0x1e6eb000
void *ast_sdram_reg_virt_base;
void *ast_scu_reg_virt_base;
void *dp_descriptor_base;

// struct reset_control *reset;
struct device_node *dt_node;
struct clk *vclk;
struct clk *eclk;
#endif

static int ast_videocap_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ast_videocap_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int ast_videocap_alloc_bigphysarea(void)
{
	unsigned long addr;
	unsigned long size;

	ast_videocap_video_buf_virt_addr = bigphysarea_alloc_pages(MEM_POOL_SIZE / PAGE_SIZE, 0, GFP_KERNEL);
	if (ast_videocap_video_buf_virt_addr == NULL) {
		return -1;
	}

	addr = (unsigned long) ast_videocap_video_buf_virt_addr;
	size = MEM_POOL_SIZE;
	while (size > 0) {
		SetPageReserved(virt_to_page(addr));
		addr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	memset(ast_videocap_video_buf_virt_addr, 0x00, MEM_POOL_SIZE);

	ast_videocap_video_buf_phys_addr = (void *) virt_to_phys(ast_videocap_video_buf_virt_addr);

	//printk(KERN_INFO "got physical memory pool for size (%d on %p virtual/%08lx bus)\n", MEM_POOL_SIZE, (unsigned long*)ast_videocap_video_buf_virt_addr, ast_videocap_video_buf_phys_addr);
	return 0;
}

static void ast_videocap_free_bigphysarea(void)
{
	unsigned long addr;
	unsigned long size;

	//printk (KERN_INFO "Releasing Big Physical memory starting :0x%lx\n", ast_videocap_video_buf_virt_addr);

	if (ast_videocap_video_buf_virt_addr != NULL) {
		size = MEM_POOL_SIZE;
		addr = (unsigned long) ast_videocap_video_buf_virt_addr;
		while (size > 0) {
			ClearPageReserved(virt_to_page(addr));
			addr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		bigphysarea_free_pages(ast_videocap_video_buf_virt_addr);
	}
}

static struct file_operations ast_videocap_fops = {
	.owner = THIS_MODULE,
	.open = ast_videocap_open,
	.release = ast_videocap_release,
#ifdef USE_UNLOCKED_IOCTL 
	.unlocked_ioctl = ast_videocap_ioctl,
#else
	.ioctl = ast_videocap_ioctl,
#endif
	.mmap = ast_videocap_mmap,
};

static int ast_video_irq;
int __init ast_videocap_module_init(void)
{
	int ret;

	printk("AST Video Capture Driver, (c) 2009-2020 American Megatrends Inc.\n");

	ret = register_chrdev_region(ast_videocap_devno, AST_VIDEOCAP_DEV_NUM, AST_VIDEOCAP_DEV_NAME);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to register char device\n", AST_VIDEOCAP_DEV_NAME);
		return ret;
	}

	cdev_init(&ast_videocap_cdev, &ast_videocap_fops);

	ret = cdev_add(&ast_videocap_cdev, ast_videocap_devno, AST_VIDEOCAP_DEV_NUM);
	if (ret < 0) {
		printk(KERN_ERR "%s: failed to add char device\n", AST_VIDEOCAP_DEV_NAME);
		goto out_cdev_register;
	}

	if (request_mem_region(AST_VIDEOCAP_REG_BASE, AST_VIDEOCAP_REG_SIZE, AST_VIDEOCAP_DRV_NAME) == NULL) {
		printk(KERN_WARNING "%s: request memory region failed\n", AST_VIDEOCAP_DRV_NAME);
		ret = -EBUSY;
		goto out_cdev_add;
	}

	ast_videocap_reg_virt_base = ioremap(AST_VIDEOCAP_REG_BASE, AST_VIDEOCAP_REG_SIZE);
	if (ast_videocap_reg_virt_base == NULL) {
		printk(KERN_WARNING "%s: ioremap failed\n", AST_VIDEOCAP_DRV_NAME);
		ret = -ENOMEM;
		goto out_mem_region;
	}

#ifdef SOC_AST2600
	dt_node = of_find_node_by_name(NULL,"ami_videocap");
	if (!dt_node)
	{
		printk(KERN_WARNING "failed to get device tree node for ami_videocap\n");
		goto out_iomap;
	}
	/* Not needed now. But keeping the reference for furture usage purpose
	** check ast_videocap_reset_hw()/ast_videocap_hw_init() for modification info */
	// reset = of_reset_control_get(dt_node, NULL);
	// if (IS_ERR(reset)) {
	// 	printk(KERN_WARNING "can't get video reset for ami_videocap\n");
	// 	goto out_iomap;
	// }

	eclk = of_clk_get_by_name(dt_node, "eclk");
	if (IS_ERR(eclk)) {
		printk(KERN_WARNING "no eclk clock defined for ami_videocap\n");
		goto out_iomap;
	}

	clk_prepare_enable(eclk);

	vclk = of_clk_get_by_name(dt_node, "vclk");
	if (IS_ERR(vclk)) {
		printk(KERN_WARNING "no vclk clock defined for ami_videocap\n");
		goto out_iomap;
	}

	clk_prepare_enable(vclk);
#endif

	ast_video_irq = GetIrqFromDT("ami_videocap", AST_VIDEOCAP_IRQ);

#ifndef SOC_AST2600
	IRQ_SET_HIGH_LEVEL(ast_video_irq);
	IRQ_SET_LEVEL_TRIGGER(ast_video_irq);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,2,0))
	ret = request_irq(ast_video_irq, ast_videocap_irq_handler, IRQF_DISABLED, AST_VIDEOCAP_DRV_NAME, NULL);
#else
	ret = request_irq(ast_video_irq, ast_videocap_irq_handler,0, AST_VIDEOCAP_DRV_NAME, NULL);
#endif
	if (ret != 0) {
		printk(KERN_WARNING "%s: request IRQ failed\n", AST_VIDEOCAP_DRV_NAME);
		goto out_iomap;
	}

	ret = ast_videocap_alloc_bigphysarea();
	if (ret != 0) {
		printk(KERN_WARNING "failed to allocate physical memory pool\n");
		goto out_irq;
	}

#ifdef SOC_AST2600
	if ((ast_sdram_reg_virt_base = ioremap(0x1E6E0000, AST_VIDEOCAP_REG_SIZE)) == NULL)
	{
		printk(KERN_WARNING "ast_videocap: ast_sdram_reg_virt_base ioremap failed\n");
		goto out_bigphys;
	}
	if ((ast_scu_reg_virt_base = ioremap(0x1E6E2000, AST_VIDEOCAP_REG_SIZE)) == NULL)
	{
		printk(KERN_WARNING "ast_videocap: ast_scu_reg_virt_base ioremap failed\n");
		goto out_bigphys;
	}
	if ((dp_descriptor_base = ioremap(DP_BASE, SZ_512)) == NULL)
	{
		printk(KERN_WARNING "ast_videocap: dp_descriptor_base ioremap failed\n");
		goto out_bigphys;
	}
#endif

	ret = ast_videocap_remap_vga_mem();
	if (ret != 0) {
		printk(KERN_WARNING "remap VGA memory failed\n");
		goto out_bigphys;
	}

	ast_videocap_engine_data_init();

	ast_videocap_reset_hw();
	ast_videocap_hw_init();

	ast_videocap_add_proc_entries();

	return 0;

out_bigphys:
	ast_videocap_free_bigphysarea();
#ifdef SOC_AST2600
	if (ast_sdram_reg_virt_base != NULL)
	iounmap(ast_sdram_reg_virt_base);

	if (ast_scu_reg_virt_base != NULL)
		iounmap(ast_scu_reg_virt_base);

	if (dp_descriptor_base != NULL)
		iounmap(dp_descriptor_base);
#endif
out_irq:
	free_irq(ast_video_irq, NULL);
out_iomap:
	iounmap(ast_videocap_reg_virt_base);
out_mem_region:
	release_mem_region(AST_VIDEOCAP_REG_BASE, AST_VIDEOCAP_REG_SIZE);
out_cdev_add:
	cdev_del(&ast_videocap_cdev);
out_cdev_register:
	unregister_chrdev_region(ast_videocap_devno, AST_VIDEOCAP_DEV_NUM);

	return ret;
}

void __exit ast_videocap_module_exit(void)
{
	ast_videocap_del_proc_entries();

	free_irq(ast_video_irq, NULL);
	iounmap(ast_videocap_reg_virt_base);
	release_mem_region(AST_VIDEOCAP_REG_BASE, AST_VIDEOCAP_REG_SIZE);
	cdev_del(&ast_videocap_cdev);
	unregister_chrdev_region(ast_videocap_devno, AST_VIDEOCAP_DEV_NUM);

	ast_videocap_free_bigphysarea();
	ast_videocap_unmap_vga_mem();
}

module_init(ast_videocap_module_init);
module_exit(ast_videocap_module_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("AST video capture engine driver");
MODULE_LICENSE("GPL");
