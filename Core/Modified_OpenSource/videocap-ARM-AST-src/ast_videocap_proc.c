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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "helper.h"

#include "ast_videocap_data.h"
#include "ast_videocap_functions.h"

#define AST_VIDECAP_PRCO_DIR			"videocap"
#define AST_VIDECAP_PRCO_STATUS_ENTRY	"status"
#define AST_VIDECAP_PRCO_REG_ENTRY		"regs"
#define AST_VIDECAP_PRCO_FULL_JPEG		"jpeg_enable"

#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
struct proc_info *statusproc;
struct proc_info *regproc;
struct proc_info *jpegproc;
#endif

static struct ctl_table_header *ast_videocap_sysctl;
static struct proc_dir_entry *ast_videocap_proc_dir = NULL;

/* /proc/sys entries list */
static struct ctl_table ast_videocap_sysctl_table[] = {
	{
		.procname = "CompressionMode",
		.data = &(ast_videocap_engine_info.FrameHeader.CompressionMode),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "JPEGTableSelector",
		.data = &(ast_videocap_engine_info.FrameHeader.JPEGTableSelector),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "JPEGScaleFactor",
		.data = &(ast_videocap_engine_info.FrameHeader.JPEGScaleFactor),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "JPEGYUVTableMapping",
		.data = &(ast_videocap_engine_info.FrameHeader.JPEGYUVTableMapping),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "DifferentialSetting",
		.data = &(ast_videocap_engine_info.INFData.DifferentialSetting),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "DigitalDifferentialThreshold",
		.data = &(ast_videocap_engine_info.INFData.DigitalDifferentialThreshold),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "AdvanceTableSelector",
		.data = &(ast_videocap_engine_info.FrameHeader.AdvanceTableSelector),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "AdvanceScaleFactor",
		.data = &(ast_videocap_engine_info.FrameHeader.AdvanceScaleFactor),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "SharpModeSelection",
		.data = &(ast_videocap_engine_info.FrameHeader.SharpModeSelection),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		.procname = "direct_mode",
		.data = &(ast_videocap_engine_info.INFData.DirectMode),
		.maxlen = sizeof(unsigned long),
		.mode = 0644,
		.child = NULL,
		.proc_handler = &proc_doulongvec_minmax
	}, {
		
	}
};

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 11))
int ast_videocap_write_jpeg(struct file *file, const char __user *buff, size_t len, loff_t *offset)
#else
int ast_videocap_write_jpeg(struct file *filp, const char __user *buff, unsigned long len, void *data)
#endif
{
	char buf_val[16];

	if (len > 16)
		return -EFAULT;

	if (copy_from_user(buf_val, buff, len))
		return -EFAULT;

	buf_val[len] = '\0';
	CaptureMode = (unsigned int)simple_strtol(buf_val, NULL, 10);/* SCA Fix [String not null terminated : Memory - corruptions]:: False Positive *//* Reason for False Positive - The source buffer is containing a NULL terminator*/

	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		printk("Switching to full JPEG capture mode\n");
		ast_init_jpeg_table();
	}
	else
	{
		printk("Switching to YUV capture mode\n");
	}

	return len;
}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 11))
int ast_videocap_print_jpeg(struct file *file, char __user *buf, size_t count, loff_t *offset)
#else
int ast_videocap_print_jpeg(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	if (CaptureMode == VIDEOCAP_JPEG_SUPPORT)
	{
		printk("Active capture mode - JPEG\n");
	}
	else
	{
		printk("Active capture mode - YUV\n");
	}

	return 0;
}
#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
int ast_videocap_print_status(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int ast_videocap_print_status(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,4,11))
	*start = buf;
	*eof = 1;
#endif

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,4,11))
	if (offset != 0) 
		return 0;
#else
	if (*offset != 0) 
		return 0;
#endif

	len = 0;

	len += sprintf(buf + len, "video buf phys addr: 0x%p\n", ast_videocap_video_buf_phys_addr);
	len += sprintf(buf + len, "video buf virt addr: 0x%p\n", ast_videocap_video_buf_virt_addr);
	len += sprintf (buf + len, "video source mode: (%d x %d) @ %d Hz\n",
					ast_videocap_engine_info.src_mode.x,
					ast_videocap_engine_info.src_mode.y,
					ast_videocap_engine_info.src_mode.RefreshRate);
	len += sprintf (buf + len, "compression mode: %ld\n", ast_videocap_engine_info.FrameHeader.CompressionMode);
	len += sprintf(buf + len, "driver waiting location: ");
	if (WaitingForModeDetection)
		len += sprintf(buf + len, "mode detection\n");
	if (WaitingForCapture)
		len += sprintf(buf + len, "capture\n");
	if (WaitingForCompression)
		len += sprintf(buf + len, "compression\n");
	if (!WaitingForCompression && !WaitingForCapture && !WaitingForModeDetection)
		len += sprintf(buf + len, "idle\n");


#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
		*offset+=len;
#endif

	return len;
}

#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
int ast_videocap_print_regs(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int ast_videocap_print_regs(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len;
	int i;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,4,11))
	*start = buf;
	*eof = 1;
#endif

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,4,11))
	if (offset != 0) 
		return 0;
#else
	if (*offset != 0) 
		return 0;
#endif

	len = 0;

	len += sprintf(buf + len, "----------------------------------------\n");

	for (i = 0; i < 0x98; i += 4) {
		len += sprintf(buf + len, "%03X: %08X\n", i, ast_videocap_read_reg(i));
	}

	for (i = 0x300; i < 0x340; i += 4) {
		len += sprintf(buf + len, "%03X: %08X\n", i, ast_videocap_read_reg(i));
	}

	len += sprintf(buf + len, "----------------------------------------\n");

#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
	*offset+=len;
#endif

	return len;
}

void ast_videocap_add_proc_entries(void)
{
	/* create directory entry of this module in /proc and add two files "info" and "regs" */
	ast_videocap_proc_dir = proc_mkdir(AST_VIDECAP_PRCO_DIR, rootdir);

#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
	statusproc=
#endif
	AddProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_STATUS_ENTRY, ast_videocap_print_status, NULL, NULL);

#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
	regproc=
#endif
	AddProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_REG_ENTRY, ast_videocap_print_regs, NULL, NULL);
#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
	jpegproc=
#endif
	AddProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_FULL_JPEG, ast_videocap_print_jpeg, ast_videocap_write_jpeg, NULL);

	/* add sysctl to access the DebugLevel */
	ast_videocap_sysctl = AddSysctlTable(AST_VIDECAP_PRCO_DIR, ast_videocap_sysctl_table);
}

void ast_videocap_del_proc_entries(void)
{
	/* Remove this driver's proc entries */
#if (LINUX_VERSION_CODE >  KERNEL_VERSION(3,4,11))
	RemoveProcEntry(statusproc);
	RemoveProcEntry(regproc);
	RemoveProcEntry(jpegproc);
#else
	RemoveProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_STATUS_ENTRY);
	RemoveProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_REG_ENTRY);
	RemoveProcEntry(ast_videocap_proc_dir, AST_VIDECAP_PRCO_FULL_JPEG);
#endif
	remove_proc_entry(AST_VIDECAP_PRCO_DIR, rootdir);

	/* Remove driver related sysctl entries */
	RemoveSysctlTable(ast_videocap_sysctl);
}
