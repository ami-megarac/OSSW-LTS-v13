/****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

#ifndef _JTAG_IOCTL_H_
#define _JTAG_IOCTL_H_

#include <linux/socket.h>
#include <linux/tcp.h>

#define dbgprintf(fmt,args...) printk(KERN_INFO fmt,##args)

typedef struct IO_ACCESS_DATA_T {
    unsigned int     id; //jtag device id 0:jtag0, 1:jtag1
    unsigned char    IsBackground;
    unsigned long    size;
    unsigned char    *buf; 
} IO_ACCESS_DATA_T, IO_ACCESS_DATA;

typedef IO_ACCESS_DATA_T jtag_ioaccess_data;

#define  RELOCATE_OFFSET        0x380


#define  IOCTL_JTAG_UPDATE_JBC         _IOW('j', 1, int)
#define  IOCTL_JTAG_VERIFY_JBC         _IOW('j', 2, int)
#define  IOCTL_JTAG_ERASE_JBC          _IOW('j', 3, int)


struct altera_io_xfer {
    unsigned int     mode;        //0 :HW mode, 1: SW mode    
    unsigned long    Address;
    unsigned long    Data;
    int id;
};

#endif /* _JTAG_IOCTL_H_ */

