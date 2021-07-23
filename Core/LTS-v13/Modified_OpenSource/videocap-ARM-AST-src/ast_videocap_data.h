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

#ifndef __AST_VIDEOCAP_DATA_H__
#define __AST_VIDEOCAP_DATA_H__

#include <linux/wait.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "ast_videocap_hw.h"

#define MEM_POOL_SIZE (0x1800000)

extern void *ast_videocap_reg_virt_base;
extern void *ast_videocap_video_buf_virt_addr;
extern void *ast_videocap_video_buf_phys_addr;

#ifdef SOC_AST2600
extern void *ast_sdram_reg_virt_base;
extern void *ast_scu_reg_virt_base;
extern void *dp_descriptor_base;

// extern struct reset_control *reset;
extern struct device_node *dt_node;
extern struct clk *vclk;
extern struct clk *eclk;
#endif

extern struct ast_videocap_engine_info_t ast_videocap_engine_info;

extern int WaitingForModeDetection;
extern int WaitingForCapture;
extern int WaitingForCompression;
extern int CaptureMode;

#endif  /* !__AST_VIDEOCAP_DATA_H__ */
