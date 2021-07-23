/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */

/* Helper Module Version */
#define HELPER_MAJOR	1
#define HELPER_MINOR 	2


#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	
#include <linux/version.h>
#include <linux/of_irq.h>
//#if defined(__arm__)
//#include <mach/platform.h>
//`#endif
#include "helper.h"

/* Debug Level for the driver - Controlled by sysctl */
static int DebugLevel = 0;		

/* Proc entry for /proc/ractrends . This is created in this 
 * module and  is exported to other modules */
struct proc_dir_entry *rootdir = NULL;

/* Local Variables */
static struct ctl_table_header *helper_sys 	= NULL;
static struct proc_dir_entry *moduledir = NULL;

unsigned long FlashStart;
unsigned long FlashSize;
unsigned long EnvStart;
unsigned long EnvSize;
unsigned long FlashSectorSize;
unsigned long UsedFlashStart;
unsigned long UsedFlashSize;
unsigned long jiffiesread=0;


extern void free_info_cache(void);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
static struct proc_info  *fwinfo_proc = NULL;
extern int fwinfo_read(struct file *file,  char __user *buffer, size_t count, loff_t *offset);

#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
static struct proc_info  *fwinfo1_proc = NULL;
static struct proc_info *fwinfo2_proc = NULL;

extern int fwinfo1_read(struct file *file,  char __user *buffer, size_t count, loff_t *offset);
extern int fwinfo2_read(struct file *file,  char __user *buffer, size_t count, loff_t *offset);
#endif

#else
static struct proc_dir_entry  *fwinfo_proc = NULL;
extern int fwinfo_read(char *buf, char **start, off_t offset, int count, int *eof, void *data);

#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
static struct proc_dir_entry *fwinfo1_proc = NULL;
static struct proc_dir_entry *fwinfo2_proc = NULL;

extern int fwinfo1_read(char *buf, char **start, off_t offset, int count, int *eof, void *data);
extern int fwinfo2_read(char *buf, char **start, off_t offset, int count, int *eof, void *data);
#endif

#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
static struct proc_info  *fbinfo_proc = NULL;
extern int flashbanksize_read(struct file *file,  char __user *buf, size_t count, loff_t *offset);
#else
static struct proc_dir_entry  *fbinfo_proc = NULL;
extern int flashbanksize_read(char *buf, char **start, off_t offset, int count, int *eof, void *data);
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static int proc_jiffies(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
#else
static int proc_jiffies(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,loff_t* pos);
#endif
/* Sysctl related tables */
static struct ctl_table HelperTable[] =
{

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,14,17))
        {
                .procname       = "DebugLevel",
                .data           = &DebugLevel,
                .maxlen         = sizeof(int),
                .mode           = 0644,
                .proc_handler   = &proc_dointvec,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "FlashStart",
                .data           = &FlashStart,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "FlashSize",
                .data           = &FlashSize,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "EnvStart",
                .data           = &EnvStart,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "EnvSize",
                .data           = &EnvSize,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "FlashSectorSize",
                .data           = &FlashSectorSize,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "UsedFlashStart",
                .data           = &UsedFlashStart,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "UsedFlashSize",
                .data           = &UsedFlashSize,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_doulongvec_minmax,
                .extra1         = NULL,
                .extra2         = NULL,
        },
        {
                .procname       = "Jiffies",
                .data           = &jiffiesread,
                .maxlen         = sizeof(unsigned long),
                .mode           = 0644,
                .proc_handler   = &proc_jiffies,
                .extra1         = NULL,
                .extra2         = NULL,
        },

		{}
#endif


#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,14,17))
        {"DebugLevel",&DebugLevel,sizeof(int),
                0644,NULL,&proc_dointvec,NULL},
        {"FlashStart",&(FlashStart),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"FlashSize",&(FlashSize),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"EnvStart",&(EnvStart),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"EnvSize",&(EnvSize),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"FlashSectorSize",&(FlashSectorSize),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"UsedFlashStart",&(UsedFlashStart),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"UsedFlashSize",&(UsedFlashSize),sizeof(unsigned long),
                0444,NULL,&proc_doulongvec_minmax,NULL},
        {"Jiffies",&(jiffiesread),sizeof(unsigned long),
                0644,NULL,&proc_jiffies,NULL},
		{}
#endif


#if (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,36))
        {"DebugLevel",&DebugLevel,sizeof(int),
                0644,NULL,NULL,&proc_dointvec,NULL},
        {"FlashStart",&(FlashStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"FlashSize",&(FlashSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"EnvStart",&(EnvStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"EnvSize",&(EnvSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"FlashSectorSize",&(FlashSectorSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"UsedFlashStart",&(UsedFlashStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"UsedFlashSize",&(UsedFlashSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax,NULL,NULL},
        {"Jiffies",&(jiffiesread),sizeof(unsigned long),
                0644,NULL,NULL,&proc_jiffies,NULL,NULL},
		{}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	{CTL_UNNUMBERED,"DebugLevel",&DebugLevel,sizeof(int),
				0644,NULL,NULL,&proc_dointvec},
	{CTL_UNNUMBERED,"FlashStart",&(FlashStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"FlashSize",&(FlashSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"EnvStart",&(EnvStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"EnvSize",&(EnvSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"FlashSectorSize",&(FlashSectorSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"UsedFlashStart",&(UsedFlashStart),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"UsedFlashSize",&(UsedFlashSize),sizeof(unsigned long),
                0444,NULL,NULL,&proc_doulongvec_minmax},
	{CTL_UNNUMBERED,"Jiffies",&(jiffiesread),sizeof(unsigned long),
                0644,NULL,NULL,&proc_jiffies},
	{0}		
#endif
};

#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
extern unsigned char broken_spi_banks;// specify the number of broken SPI flash bank
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static int proc_jiffies(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos)
#else
static int proc_jiffies(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,loff_t* pos)
#endif
{
	int retval=0;

	jiffiesread = jiffies;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	retval = proc_doulongvec_minmax(ctl,write,buffer,lenp,pos);
#else
	retval = proc_doulongvec_minmax(ctl,write,filep,buffer,lenp,pos);
#endif	
	return retval;
}



int GetIrqFromDT(char *devname, int def_irq)
{
    struct device_node *dt_node;
    int irq;

    dt_node = of_find_node_by_name(NULL,devname);
    if (!dt_node)
    {
        //printk("WARNING : Unable to get dt node for %s. Using default Irq %d\n",devname,def_irq);
    	printk("%s : Using Irq %d \n",devname,def_irq);
        return def_irq;
    }
    irq = irq_of_parse_and_map(dt_node, 0);
    printk("%s : Using Irq %d from DT\n",devname,irq);
    return irq;
}

void  GetMultipleIrqFromDT(char *devname, int count, int *def_irq, int *map_irq)
{
    struct device_node *dt_node;
    int i;

    dt_node = of_find_node_by_name(NULL,devname);
    if (!dt_node)
    {
		for (i=0;i<count;i++)
		{
			printk("%s : Using Default Irq %d \n",devname,def_irq[i]);
			map_irq[i] = def_irq[i];
		}
		return;
    }


	for (i=0;i<count;i++)
	{
    	map_irq[i] = irq_of_parse_and_map(dt_node, i);
		if (map_irq[i] < 0)
    		printk("ERROR: %s : Unable to get rq for index  %d . Check device tree\n",devname,i);
		else
    		printk("%s : Using Irq %d from DT\n",devname,map_irq[i]);
	}

    return;
}   


int
init_helper(void)
{
	printk(KERN_ALERT "Helper Module Driver Version %d.%d\n",HELPER_MAJOR,HELPER_MINOR);
	printk(KERN_ALERT "Copyright (c) 2009-2015 American Megatrends Inc.\n");

	/* Create a new entry under /proc */
	rootdir = proc_mkdir(PROC_DIR,NULL);
	FlashStart    	= CONFIG_SPX_FEATURE_GLOBAL_FLASH_START;
	FlashSize     	= CONFIG_SPX_FEATURE_GLOBAL_FLASH_SIZE;
	EnvStart      	= CONFIG_SPX_FEATURE_GLOBAL_UBOOT_ENV_START;
	EnvSize         = CONFIG_SPX_FEATURE_GLOBAL_UBOOT_ENV_SIZE;
	FlashSectorSize = CONFIG_SPX_FEATURE_GLOBAL_ERASE_BLOCK_SIZE;
	UsedFlashStart 	= CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_START;
	UsedFlashSize	= CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
	
	if(GetCurrentRunningImage()==2){
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
		EnvStart+=get_secondaryimage_offset();
#endif
	}

	/* Create this module's directory entry in proc and add a file "HwInfo" */
	moduledir = proc_mkdir("Helper",rootdir);
	fwinfo_proc = AddProcEntry(moduledir,"FwInfo",fwinfo_read,NULL,NULL);

#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 1)
#endif
	{
		fwinfo1_proc = AddProcEntry(moduledir,"FwInfo1",fwinfo1_read,NULL,NULL);
	}
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 2)
#endif
	{
		fwinfo2_proc = AddProcEntry(moduledir,"FwInfo2",fwinfo2_read,NULL,NULL);
	}
#endif

    fbinfo_proc = AddProcEntry(moduledir,"FbInfo",flashbanksize_read,NULL,NULL);	
	/* Add sysctl to access the DebugLevel */
	helper_sys  = AddSysctlTable("Helper",&HelperTable[0]);
	return 0;
}


void
exit_helper(void)
{
	printk(KERN_ALERT "Unloading Helper Module ..............\n");
	/* Remove driver related sysctl entries */
	RemoveSysctlTable(helper_sys);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36))
	/* Remove the driver's proc entries */
	RemoveProcEntry(fwinfo_proc);
    RemoveProcEntry(fbinfo_proc);
#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 1)
#endif
	{
		RemoveProcEntry(fwinfo1_proc);
	}
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 2)
#endif
	{
		RemoveProcEntry(fwinfo2_proc);
	}
#endif

#else
	/* Remove the driver's proc entries */
	RemoveProcEntry(moduledir,"FwInfo");
    RemoveProcEntry(moduledir,"FbInfo");
#ifdef CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 1)
#endif
	{
		RemoveProcEntry(moduledir,"FwInfo1");
	}
#ifdef CONFIG_SPX_FEATURE_HW_FAILSAFE_BOOT
	if (broken_spi_banks != 2)
#endif
	{
		RemoveProcEntry(moduledir,"FwInfo2");
	}
#endif
	
#endif

	/* Remove the driver's proc entries */
	remove_proc_entry("Helper",rootdir);
	
	/* Remove the rootdir under /proc */
	remove_proc_entry(PROC_DIR,NULL);

	free_info_cache();
	return;
}

module_init(init_helper);
module_exit(exit_helper);

/* Define the symbols to be exported to outside world */
EXPORT_SYMBOL(rootdir);
EXPORT_SYMBOL(GetIrqFromDT);
EXPORT_SYMBOL(GetMultipleIrqFromDT);


/*--------------------------- Module Information Follows --------------------------*/
//#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24))
MODULE_LICENSE("GPL");        // Need to be in GPL to use get_mtd_device function, which is exported as GPL symbol in 2.6.24
//#endif
MODULE_AUTHOR("Samvinesh Christopher - American Megatrends Inc");
MODULE_DESCRIPTION("Helper Functions for all other modules");


