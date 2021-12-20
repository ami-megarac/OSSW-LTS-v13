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

 /*
 * File name: ast_jtag.c
 * JTAG hardware driver is implemented in software mode instead of hardware mode.
 * Driver controlls TCK, TMS, and TDIO by software directly when software mode is enable.
 * The CLK is 12KHz.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "jtag_ioctl.h"
#include "ast_jtag.h"


unsigned long g_jtag_base=0x1E6E4000;
// jtag pin set for ALTERA
#define TDI_PIN 0 
#define TDO_PIN	1 
#define TCK_PIN	2 
#define TMS_PIN	3 

extern int jtag_write_register(struct altera_io_xfer *io,int size);
extern int jtag_read_register(struct altera_io_xfer *io,int size);

typedef enum {
	SET_LOW = 0,
	SET_HIGH,
	READ_VAL,
	FORCE_SET_LOW = 0x10,
	FORCE_SET_HIGH	
}eJtagPinactions;

/*
 * ast_tdo_pin
 * Note: Connect a GPIO(G5) to replace TDO, and read it to get data back from device. A workaround for AST2300/AST1050.
 */
static int ast_tdo_pin(void){
	struct altera_io_xfer ioxfer;
	memset(&ioxfer,0,sizeof(struct altera_io_xfer));
    if(g_jtag_base==0x1E6E4000) 
    { 
        ioxfer.id=0;
    }
    else
    { 
        ioxfer.id=1;
    }
    ioxfer.Address=(JTAG_STATUS);
    jtag_read_register(&ioxfer,sizeof(struct altera_io_xfer));
	return (ioxfer.Data & SOFTWARE_TDIO_BIT);
}

int jtag_io(int pin, int act){

    // TDI_PIN 0(w), TDO_PIN 1(r), TCK_PIN 2(w), TMS_PIN 3(w)
    static unsigned char Pins[4]={0,};
    int reVal = 0;
    unsigned int op_code = 0;
    unsigned int force_action = 0;
    struct altera_io_xfer ioxfer;	

    switch(act)
    {
    
    	case FORCE_SET_LOW:
            //fallthrough
       	case FORCE_SET_HIGH:
       		force_action = 1;
            //fallthrough
    	case SET_LOW:
            //fallthrough
    	case SET_HIGH:
		if(force_action == 1 || Pins[pin] != act) 
		{
			Pins[pin] =  act;

			if (Pins[TCK_PIN]) 
			{	
				op_code |= SOFTWARE_TCK_BIT; 
			}
				
			if (Pins[TMS_PIN])
			{
				op_code |= SOFTWARE_TMS_BIT; 
			}
			
			if (Pins[TDI_PIN])
			{
				op_code |= SOFTWARE_TDIO_BIT;
			}

			memset(&ioxfer,0,sizeof(struct altera_io_xfer));
		    if(g_jtag_base==0x1E6E4000) 
            {
                //printk("g_jtag_base==0x1E6E4000\n");
                ioxfer.id=0;
            }
            else 
            {
                 //printk("g_jtag_base==0x1E6E4100\n");
                 ioxfer.id=1;
        	}
            ioxfer.Address=(JTAG_STATUS);
			ioxfer.Data=(SOFTWARE_MODE_ENABLE | op_code);
			jtag_write_register(&ioxfer,sizeof(struct altera_io_xfer));
		}
		break;
    	case READ_VAL:
    		if( pin == TDO_PIN){
			 reVal = ast_tdo_pin();
		 }
		 break;
    }
    
    return reVal;
}

void set_jtag_base(int id)
{
	if(id)
	{
		g_jtag_base=0x1E6E4100;
	}
	else
	{
		g_jtag_base=0x1E6E4000;
	}

	return;
}
EXPORT_SYMBOL(jtag_io);
