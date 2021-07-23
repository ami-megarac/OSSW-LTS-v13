/****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/fs.h>
//#include <asm/segment.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <linux/module.h>
#include "header.h"
#include "usb_hw.h"
#include "usb_ioctl.h"
#include "coreusb.h"
#include "helper.h"
#include "dbgout.h"
#include "usb_module.h"
#include "entry.h"

#define USB_MAJOR 100

#ifdef HAVE_UNLOCKED_IOCTL
  #if HAVE_UNLOCKED_IOCTL
	#define USE_UNLOCKED_IOCTL
  #endif
#endif

/****************************Driver Entry Points ******************************/
#ifdef USE_UNLOCKED_IOCTL 
static long    UsbDriverIoctlUnlocked (struct file*,unsigned int,unsigned long);
#else
static int     UsbDriverIoctl (struct inode*, struct file*,unsigned int,unsigned long);
#endif
static ssize_t UsbDriverRead  (struct file *, char *, size_t, loff_t *);
static ssize_t UsbDriverWrite(struct file * file , const char __user *buf, size_t count, loff_t *ppos);
static int 	   UsbDriverOpen (struct inode *inode, struct file *file);
static int	   UsbDriverRelease (struct inode *inode, struct file *file);
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
extern int VirtualDeviceStatus;
#endif
/************************ Driver File Operations Structure********************/
struct file_operations usb_fops = 
{
	.owner	=	THIS_MODULE,
	.read 	=	UsbDriverRead,	/* pipe read */
	.write 	=	UsbDriverWrite,	/* pipe write */
#ifdef USE_UNLOCKED_IOCTL 
       	.unlocked_ioctl  =     UsbDriverIoctlUnlocked,
#else
       	.ioctl  	=      UsbDriverIoctl,  /*ioctl */
#endif
	.open   =	UsbDriverOpen,
	.release =  UsbDriverRelease,
};

/*************************** Driver Entry Point *****************************/
int
UsbDriverInit(void)
{
	printk("USB Device Endpoint Driver \n");
	printk("Copyright 2006 American Megatrends Inc.\n");
	
	if (register_chrdev(USB_MAJOR, "usb",  &usb_fops) < 0) 
	{
		TCRIT("USB Driver can't get Major %d\n",USB_MAJOR);
		return -EBUSY;
	} 

	/* Call Initialization Routine */
	if (Entry_UsbInit() == 0)
	{
		printk("USB Driver is Successfully Initialized\n");
		return 0;
	}

	/* Initialization Failed , unregister driver */	
	unregister_chrdev(USB_MAJOR,"usb");
	TCRIT("Error in Initializing USB Driver.Unregistered!\n");
	return -EBUSY;
}

void
UsbDriverExit(void)
{
	Entry_UsbExit();
	unregister_chrdev(USB_MAJOR,"usb");
}

static int
UsbDriverOpen (struct inode *inode, struct file *file)
{
	return 0;
}

static int
UsbDriverRelease (struct inode *inode, struct file *file)
{
	return 0;
}


/******************* Driver IOCTL Entry Point ************************/
static 
long
UsbDriverIoctlUnlocked(struct file * file,
						unsigned int cmd,unsigned long arg)
{
	int RetVal;
	int DevNo;
	int ret = 0;

	for (DevNo = 0; DevNo < MAX_USB_HW; DevNo++)
	{
			if (!DevInfo[DevNo].UsbCoreDev.DevUsbIOCTL) continue;
			ret = DevInfo[DevNo].UsbCoreDev.DevUsbIOCTL (cmd, arg, &RetVal);
			if ((0 == ret) || (QUEUE_FULL_WARNING == ret))
			{
					return RetVal;
			}
	}
	
	switch (cmd)
	{
		case USB_GET_CONNECT_STATE:
			DevNo = get_user (DevNo, (uint8*)arg);
			return UsbGetDeviceConnectState (DevNo);
			
		case USB_GET_IUSB_DEVICES:
			/* This call will form a complete IUSB Packet */
			return GetiUsbDeviceList((IUSB_DEVICE_LIST *)arg);

		case USB_GET_INTERFACES:
			return GetFreeInterfaces (((IUSB_FREE_DEVICE_INFO*)arg)->DeviceType,
								((IUSB_FREE_DEVICE_INFO*)arg)->LockType,
							&((IUSB_FREE_DEVICE_INFO*)arg)->iUSBdevList);
		
		case USB_REQ_INTERFACE:
			return RequestInterface ((IUSB_REQ_REL_DEVICE_INFO*)arg);
	
		case USB_REL_INTERFACE:
			return ReleaseInterface ((IUSB_REQ_REL_DEVICE_INFO*)arg);
			break;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
		case USB_DISABLE_ALL_DEVICE:
			return UsbUpstreamDisable_Device_all();
			break;   
		case USB_ENABLE_ALL_DEVICE:
			return UsbUpstreamEnable_Device_all();
			break; 
		case USB_GET_ALL_DEVICE_STATUS:
			return UsbUpstreamGet_Device_all_status((IUSB_IOCTL_DEVICE_STATUS*)arg);
			break;
#endif
		//USB 2.0 enabling/disabling support
		case USB_DISABLE_MEDIA_DEVICE:
				return UsbUpstreamDisable_MediaDevice();
			break;   
		case USB_ENABLE_MEDIA_DEVICE:
				return UsbUpstreamEnable_MediaDevice();
			break; 
		default:
			TDBG_FLAGGED(usbe, DEBUG_ARCH,"UsbDriverIoctl(): Unknown ioctl");
		        return(-EINVAL);
	}
	return 0;
}

#ifndef USE_UNLOCKED_IOCTL 
static int     
UsbDriverIoctl(struct inode * inode, struct file * file, unsigned int cmd,unsigned long arg)
{
	return UsbDriverIoctlUnlocked(file,cmd,arg);
}
#endif

/******************** Driver Interrupt Service Routine ******************/
irqreturn_t 
UsbDriverIsr(int irq, void *dev_id)
{
	unsigned long DevNo = (unsigned long)dev_id;
	return Entry_UsbIsr((uint8)(DevNo & 0xFF),(uint8)((DevNo & 0xFF00)>>8)); 
}



static 
ssize_t  
UsbDriverRead(struct file * file , char * buf, size_t count, loff_t *ppos)
{
	int DevNo;
	for (DevNo = 0; DevNo < MAX_USB_HW; DevNo++)
	{
		if (!DevInfo[DevNo].UsbCoreDev.DevUsbRead) 
			continue;
		return DevInfo[DevNo].UsbCoreDev.DevUsbRead (file,buf,count,ppos);
	}
	return -EINVAL;
}

static ssize_t 
UsbDriverWrite(struct file * file , const char __user *buf, size_t count, loff_t *ppos)
{
	int DevNo;
	for (DevNo = 0; DevNo < MAX_USB_HW; DevNo++)
	{
		if (!DevInfo[DevNo].UsbCoreDev.DevUsbWrite) 
			continue;
		return DevInfo[DevNo].UsbCoreDev.DevUsbWrite (file,buf,count,ppos);
	}
	return -EINVAL;
}


