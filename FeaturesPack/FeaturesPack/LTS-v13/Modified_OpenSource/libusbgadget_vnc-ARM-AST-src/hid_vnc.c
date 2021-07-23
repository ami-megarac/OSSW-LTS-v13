#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include "libusbgadget_vnc.h"
#include "hid_vnc.h"
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>

static void hid_device_usb_init(void);
static void hid_device_usb_close(void);
static void hid_device_usb_open(void);
static int SetMouseReportDesc(uint8 *ReportDesc, uint8 size);
static int SetKeyboardReportDesc(void);
int safe_system( const char *command );
static int get_hid_handle_list(void);
static int verify_hid_handle_list(void);
static int IsProcRunning( char *procarg);


#define MOUSE_DEV_NODE		"/dev/hidg0"
#define KEYBOARD_DEV_NODE	"/dev/hidg1"

#define LED_DATA_SIZE		1

#define MOUSE_REPORT_DESC_PATH	"/sys/kernel/config/usb_gadget/hid/functions/hid.usb0/report_desc"
#define KEYBD_REPORT_DESC_PATH	"/sys/kernel/config/usb_gadget/hid/functions/hid.usb1/report_desc"

/************************* Mouse Report Descriptor **************************/
//Wheel Support for Mouse
static uint8 AbsoluteMouseReport[]  = {
    USAGE_PAGE1(GENERIC_DESKTOP) >> 8,
    USAGE_PAGE1(GENERIC_DESKTOP) & 0xFF,
    USAGE1(MOUSE) >> 8,
    USAGE1(MOUSE) & 0xFF,
    COLLECTION1(APPLICATION) >> 8,
    COLLECTION1(APPLICATION) & 0xFF,
    USAGE1(POINTER) >> 8,
    USAGE1(POINTER) & 0xFF,
    COLLECTION1(PHYSICAL) >> 8,
    COLLECTION1(PHYSICAL) & 0xFF,
    USAGE_PAGE1(BUTTON) >> 8,
    USAGE_PAGE1(BUTTON) & 0xFF,
    USAGE_MINIMUM1(1) >> 8,
    USAGE_MINIMUM1(1) & 0xFF,
    USAGE_MAXIMUM1(3) >> 8,
    USAGE_MAXIMUM1(3) & 0xFF,
    LOGICAL_MINIMUM1(0) >> 8,
    LOGICAL_MINIMUM1(0) & 0xFF,
    LOGICAL_MAXIMUM1(1) >> 8,
    LOGICAL_MAXIMUM1(1) & 0xFF,
    REPORT_COUNT1(3) >> 8,
    REPORT_COUNT1(3) & 0xFF,
    REPORT_SIZE1(1) >> 8,
    REPORT_SIZE1(1) & 0xFF,
    INPUT3(DATA, VARIABLE, ABSOLUTE) >> 8,
    INPUT3(DATA, VARIABLE, ABSOLUTE) & 0xFF,
    REPORT_COUNT1(1) >> 8,
    REPORT_COUNT1(1) & 0xFF,
    REPORT_SIZE1(5) >> 8,
    REPORT_SIZE1(5) & 0xFF,
    INPUT1(CONSTANT) >> 8,
    INPUT1(CONSTANT) & 0xFF,
    USAGE_PAGE1(GENERIC_DESKTOP) >> 8,
    USAGE_PAGE1(GENERIC_DESKTOP) & 0xFF,
    USAGE1(X) >> 8,
    USAGE1(X) & 0xFF,
    USAGE1(Y) >> 8,
    USAGE1(Y) & 0xFF,
    LOGICAL_MINIMUM1(0) >> 8,
    LOGICAL_MINIMUM1(0) & 0xFF,
    LOGICAL_MAXIMUM2(32767) >> 16,
    (uint8)(LOGICAL_MAXIMUM2(32767) >> 8),
    LOGICAL_MAXIMUM2(32767) & 0xFF,
    REPORT_SIZE1(16) >> 8,
    REPORT_SIZE1(16) & 0xFF,
    REPORT_COUNT1(2) >> 8,
    REPORT_COUNT1(2) & 0xFF,
    INPUT3(DATA, VARIABLE, ABSOLUTE) >> 8,
    INPUT3(DATA, VARIABLE, ABSOLUTE) & 0xFF,
	USAGE1(WHEEL) >> 8,
	USAGE1(WHEEL) & 0xFF,
    LOGICAL_MINIMUM1(-127) >> 8,
    LOGICAL_MINIMUM1(-127) & 0xFF,
    LOGICAL_MAXIMUM1(127) >> 8,
    LOGICAL_MAXIMUM1(127) & 0xFF,
    REPORT_SIZE1(8) >> 8,
    REPORT_SIZE1(8) & 0xFF,
    REPORT_COUNT1(1) >> 8,
    REPORT_COUNT1(1) & 0xFF,
	INPUT3(DATA, VARIABLE, RELATIVE) >> 8,
    INPUT3(DATA, VARIABLE, RELATIVE) & 0xFF,
    
    END_COLLECTION,
    END_COLLECTION
};

//Wheel Support for Mouse
static uint8 RelativeMouseReport[] = 
{
    USAGE_PAGE1(GENERIC_DESKTOP) >> 8,
    USAGE_PAGE1(GENERIC_DESKTOP) & 0xFF,
    USAGE1(MOUSE) >> 8,
    USAGE1(MOUSE) & 0xFF,
    COLLECTION1(APPLICATION) >> 8,
    COLLECTION1(APPLICATION) & 0xFF,
    USAGE1(POINTER) >> 8,
    USAGE1(POINTER) & 0xFF,
    COLLECTION1(PHYSICAL) >> 8,
    COLLECTION1(PHYSICAL) & 0xFF,
    USAGE_PAGE1(BUTTON) >> 8,
    USAGE_PAGE1(BUTTON) & 0xFF,
    USAGE_MINIMUM1(1) >> 8,
    USAGE_MINIMUM1(1) & 0xFF,
    USAGE_MAXIMUM1(3) >> 8,
    USAGE_MAXIMUM1(3) & 0xFF,
    LOGICAL_MINIMUM1(0) >> 8,
    LOGICAL_MINIMUM1(0) & 0xFF,
    LOGICAL_MAXIMUM1(1) >> 8,
    LOGICAL_MAXIMUM1(1) & 0xFF,
    REPORT_COUNT1(3) >> 8,
    REPORT_COUNT1(3) & 0xFF,
    REPORT_SIZE1(1) >> 8,
    REPORT_SIZE1(1) & 0xFF,
    INPUT3(DATA, VARIABLE, ABSOLUTE) >> 8,
    INPUT3(DATA, VARIABLE, ABSOLUTE) & 0xFF,
    REPORT_COUNT1(1) >> 8,
    REPORT_COUNT1(1) & 0xFF,
    REPORT_SIZE1(5) >> 8,
    REPORT_SIZE1(5) & 0xFF,
    INPUT1(CONSTANT) >> 8,
    INPUT1(CONSTANT) & 0xFF,
    USAGE_PAGE1(GENERIC_DESKTOP) >> 8,
    USAGE_PAGE1(GENERIC_DESKTOP) & 0xFF,
    USAGE1(X) >> 8,
    USAGE1(X) & 0xFF,
    USAGE1(Y) >> 8,
    USAGE1(Y) & 0xFF,

    LOGICAL_MINIMUM1(-127) >> 8,
    LOGICAL_MINIMUM1(-127) & 0xFF,
    LOGICAL_MAXIMUM1(127) >> 8,
    LOGICAL_MAXIMUM1(127) & 0xFF,
    REPORT_SIZE1(8) >> 8,
    REPORT_SIZE1(8) & 0xFF,
    REPORT_COUNT1(2) >> 8,
    REPORT_COUNT1(2) & 0xFF,
    INPUT3(DATA, VARIABLE, RELATIVE) >> 8,
    INPUT3(DATA, VARIABLE, RELATIVE) & 0xFF,

	USAGE1(WHEEL) >> 8,
	USAGE1(WHEEL) & 0xFF,
    LOGICAL_MINIMUM1(-127) >> 8,
    LOGICAL_MINIMUM1(-127) & 0xFF,
    LOGICAL_MAXIMUM1(127) >> 8,
    LOGICAL_MAXIMUM1(127) & 0xFF,
    REPORT_SIZE1(8) >> 8,
    REPORT_SIZE1(8) & 0xFF,
    REPORT_COUNT1(1) >> 8,
    REPORT_COUNT1(1) & 0xFF,
	INPUT3(DATA, VARIABLE, RELATIVE) >> 8,
    INPUT3(DATA, VARIABLE, RELATIVE) & 0xFF,

    END_COLLECTION,
    END_COLLECTION

    
};
/************************ Keyboard report Descriptor **************************/
static uint8 KeybdReport[] = {
	USAGE_PAGE1(GENERIC_DESKTOP) >> 8,
	USAGE_PAGE1(GENERIC_DESKTOP) & 0xFF,
	USAGE1(KEYBOARD) >> 8,
	USAGE1(KEYBOARD) & 0xFF,
	COLLECTION1(APPLICATION) >> 8,
	COLLECTION1(APPLICATION) & 0xFF,
			USAGE_PAGE1(KEYBOARD_KEYPAD) >> 8,
			USAGE_PAGE1(KEYBOARD_KEYPAD) & 0xFF,
			USAGE_MINIMUM1(224) >> 8,
			USAGE_MINIMUM1(224) & 0xFF,
 			USAGE_MAXIMUM1(231) >> 8,
 			USAGE_MAXIMUM1(231) & 0xFF,
 			LOGICAL_MINIMUM1(0) >> 8,
 			LOGICAL_MINIMUM1(0) & 0xFF,
 			LOGICAL_MAXIMUM1(1) >> 8,
 			LOGICAL_MAXIMUM1(1) & 0xFF,
 			REPORT_SIZE1(1) >> 8,
 			REPORT_SIZE1(1) & 0xFF,
 			REPORT_COUNT1(8) >> 8,
 			REPORT_COUNT1(8) & 0xFF,
 			INPUT3(DATA, VARIABLE, ABSOLUTE) >> 8,
 			INPUT3(DATA, VARIABLE, ABSOLUTE) & 0xFF,
 			REPORT_COUNT1(1) >> 8,
 			REPORT_COUNT1(1) & 0xFF,
 			REPORT_SIZE1(8) >> 8,
 			REPORT_SIZE1(8) & 0xFF,
 			INPUT1(CONSTANT) >> 8,
 			INPUT1(CONSTANT) & 0xFF,
 			REPORT_COUNT1(5) >> 8,
 			REPORT_COUNT1(5) & 0xFF,
 			REPORT_SIZE1(1) >> 8,
 			REPORT_SIZE1(1) & 0xFF,
 			USAGE_PAGE1(LEDS) >> 8,
 			USAGE_PAGE1(LEDS) & 0xFF,
 			USAGE_MINIMUM1(1) >> 8,
			USAGE_MINIMUM1(1) & 0xFF,
 			USAGE_MAXIMUM1(5) >> 8,
 			USAGE_MAXIMUM1(5) & 0xFF,
 			OUTPUT3(DATA, VARIABLE, ABSOLUTE) >> 8,
 			OUTPUT3(DATA, VARIABLE, ABSOLUTE) & 0xFF,
			REPORT_COUNT1(1) >> 8,
 			REPORT_COUNT1(1) & 0xFF,
 			REPORT_SIZE1(3) >> 8,
 			REPORT_SIZE1(3) & 0xFF,
 			OUTPUT1(CONSTANT) >> 8,
			OUTPUT1(CONSTANT) & 0xFF,
			REPORT_COUNT1(6) >> 8,
 			REPORT_COUNT1(6) & 0xFF,	
 			REPORT_SIZE1(8) >> 8,
 			REPORT_SIZE1(8) & 0xFF,
 			LOGICAL_MINIMUM1(0) >> 8,
 			LOGICAL_MINIMUM1(0) & 0xFF,
		    LOGICAL_MAXIMUM2(255) >> 16,
		    (uint8)(LOGICAL_MAXIMUM2(255) >> 8),
		    LOGICAL_MAXIMUM2(255) & 0xFF,
			USAGE_PAGE1(KEYBOARD_KEYPAD) >> 8,
			USAGE_PAGE1(KEYBOARD_KEYPAD) & 0xFF,
			USAGE_MINIMUM1(0) >> 8,
			USAGE_MINIMUM1(0) & 0xFF,
			USAGE_MAXIMUM2(255) >> 16,
			(uint8)(USAGE_MAXIMUM2(255) >> 8),
			USAGE_MAXIMUM2(255) & 0xFF,
			INPUT2(DATA, ARRAY) >> 8,
 			INPUT2(DATA, ARRAY) & 0xFF,
	END_COLLECTION
};

uint8 MouseMode = ABSOLUTE_MOUSE_MODE;
static char list[MAX_SUPPORT_PROCESS][20];

int hid_open(char *reg_process)
{
	int status;
	uint8 resource;

	hid_device_usb_init();
	status = get_hid_resource(&resource);

	if(status != 0)
		hid_device_usb_open();
	save_hid_resource(reg_process);
	return UDC_exist(HID_PATH, HID_UDC);
}

int hid_stop(char *reg_process)
{
	uint8 resource;
	int status;

	verify_hid_handle_list();
	unload_hid_resource(reg_process);
	status = get_hid_resource(&resource);

	if(status == SUCCESS) {
		if(resource > 0) {
			return RESOURCE_BUSY;
		} else {
			goto stop;
		}
	} else {
		goto stop;
	}
stop:
	hid_device_usb_close();
	if(UDC_exist(HID_PATH, HID_UDC) == SUCCESS) {
		return FAIL;
	} else {
		return SUCCESS;
	}
}

static void hid_device_usb_init(void)
{
	int status;

	safe_system("mount -t configfs none /sys/kernel/config");

    status = safe_system("[ -d /sys/kernel/config/usb_gadget/hid ]");

    if(status) {
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid");
    	safe_system("echo 0x046b >/sys/kernel/config/usb_gadget/hid/idVendor");
    	safe_system("echo 0xFF10 >/sys/kernel/config/usb_gadget/hid/idProduct");
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid/strings/0x409");
    	safe_system("echo 0123456789 > /sys/kernel/config/usb_gadget/hid/strings/0x409/serialnumber");
    	safe_system("echo American Megatreds Inc. > /sys/kernel/config/usb_gadget/hid/strings/0x409/manufacturer");
    	safe_system("echo Virtual Keyboard and Mouse Gadget > /sys/kernel/config/usb_gadget/hid/strings/0x409/product");
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid/configs/c.1");
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid/configs/c.1/strings/0x409");
    	safe_system("echo config 1 > /sys/kernel/config/usb_gadget/hid/configs/c.1/strings/0x409/configuration");
		//mouse usb 0
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid/functions/hid.usb0");
		//protocol 2:mouse 1:keyboard    	
    	safe_system("echo 2 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb0/protocol");
		//sub clasee 1:Boot Interface 
    	safe_system("echo 1 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb0/subclass");
		//report length    	
    	safe_system("echo 6 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb0/report_length");

		//report descriptor 
    	if(MouseMode == ABSOLUTE_MOUSE_MODE) {
    		SetMouseReportDesc(&AbsoluteMouseReport[0], sizeof(AbsoluteMouseReport));
    	} else if(MouseMode == RELATIVE_MOUSE_MODE) {
    		SetMouseReportDesc(&RelativeMouseReport[0], sizeof(RelativeMouseReport));
    	}

    	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb0 configs/c.1/");

		//keyboard usb 1
    	safe_system("mkdir /sys/kernel/config/usb_gadget/hid/functions/hid.usb1");

		//protocol 2:mouse 1:keyboard
    	safe_system("echo 1 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb1/protocol");
		//sub clasee 1:Boot Interface 
    	safe_system("echo 1 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb1/subclass");	   

		//report length 	
    	safe_system("echo 8 > /sys/kernel/config/usb_gadget/hid/functions/hid.usb1/report_length");
    	
		//report descriptor
    	SetKeyboardReportDesc();

    	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb1 configs/c.1/");
    	safe_system("echo 0xE0 > /sys/kernel/config/usb_gadget/hid/configs/c.1/bmAttributes");
    }
}


static void hid_device_usb_close(void)
{
	safe_system("echo > /sys/kernel/config/usb_gadget/hid/UDC");
}

static void hid_device_usb_open(void)
{
	safe_system("echo 1e6a0000.usb-vhub:p4 > /sys/kernel/config/usb_gadget/hid/UDC");
}

int SendKeybdData(uint8 *buf)
{
	int report_len;
	int fd = -1;

	fd = open(KEYBOARD_DEV_NODE, O_RDWR);//file_open(KEYBOARD_DEV_NODE);
	if(fd < 0) {
		printf("Cannot open %s",KEYBOARD_DEV_NODE);
		close(fd);
		return FAIL;
	}
	if(buf == NULL) {
		close(fd);
		return FAIL;
	}
	
	report_len = MAX_KEYBD_DATA_SIZE;


	if(write(fd, buf, report_len) != report_len) {
		close(fd);
		return FAIL;
	}
	
	close(fd);
	return SUCCESS;
}

int SendMouseData(uint8 *buf)
{
	int report_len;
	int fd = -1;

	fd = open(MOUSE_DEV_NODE, O_RDWR);
	if(fd < 0) {
		printf("Cannot open %s",MOUSE_DEV_NODE);
		close(fd);
		return FAIL;
	}

	if(buf == NULL) {
		close(fd);
		return FAIL;
	}

	report_len = MAX_MOUSE_DATA_SIZE;

	if(report_len > 0) {
		if(write(fd, buf, report_len) != report_len) {
			close(fd);
			return FAIL;
		}
	}
	
	close(fd);

	return SUCCESS;
}

static int SetMouseReportDesc(uint8 *ReportDesc, uint8 size)
{
	int len = 0;
	int fd = -1;
	
	fd = open(MOUSE_REPORT_DESC_PATH, O_RDWR);
	if(fd < 0) {
		printf("Cannot open %s",MOUSE_REPORT_DESC_PATH);
		close(fd);
		return FAIL;
	}

	len = size;
	if(write(fd, ReportDesc, len) != len) {
		close(fd);
		return FAIL;
	}
	close(fd);
	
	return SUCCESS;
}

static int SetKeyboardReportDesc(void)
{
	int len = 0;
	int fd = 0;
	fd = open(KEYBD_REPORT_DESC_PATH, O_RDWR);
	if(fd < 0) {
		printf("Cannot open %s",KEYBD_REPORT_DESC_PATH);
		close(fd);
		return FAIL;
	}

	len = sizeof(KeybdReport);

	if(write(fd, KeybdReport, len) != len) {
		close(fd);
		return FAIL;
	}
	close(fd);
	
	return SUCCESS;
}

int changeMouseAbs2Rel(void)
{
	int retVal = 0;

	hid_device_usb_close();

	safe_system("cd /sys/kernel/config/usb_gadget/hid/configs/c.1 && rm hid.usb0");
	safe_system("cd /sys/kernel/config/usb_gadget/hid/configs/c.1 && rm hid.usb1");
	safe_system("echo > /sys/kernel/config/usb_gadget/hid/functions/hid.usb0/report_desc");
	MouseMode = RELATIVE_MOUSE_MODE;
	SetMouseReportDesc(&RelativeMouseReport[0], sizeof(RelativeMouseReport));
	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb0 configs/c.1/");
	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb1 configs/c.1/");

	hid_device_usb_open();

	return retVal;
}

int changeMouseRel2Abs(void)
{
	int retVal = 0;

	hid_device_usb_close();

	safe_system("cd /sys/kernel/config/usb_gadget/hid && rm configs/c.1/hid.usb0");
	safe_system("cd /sys/kernel/config/usb_gadget/hid && rm configs/c.1/hid.usb1");
	safe_system("echo > /sys/kernel/config/usb_gadget/hid/functions/hid.usb0/report_desc");
	MouseMode = ABSOLUTE_MOUSE_MODE;
	SetMouseReportDesc(&AbsoluteMouseReport[0], sizeof(AbsoluteMouseReport));
	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb0 configs/c.1/");
	safe_system("cd /sys/kernel/config/usb_gadget/hid && ln -s functions/hid.usb1 configs/c.1/");

	hid_device_usb_open();
	return retVal;
}

int getCurrentMouseMode(uint8 *MouseModeStatus)
{
	int retVal = 0;
	uint8 *buf = NULL;
	int ret;
	int fd = -1;

	buf = (uint8 *)malloc(MAX_CMD_SIZE);

	if(!buf) {
		printf("Memory allocation Failed");
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}

	fd = open(MOUSE_REPORT_DESC_PATH, O_RDONLY);
	if(fd < 0) {
		printf("Cannot open %s",MOUSE_REPORT_DESC_PATH);
		close(fd);
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}

	ret = read(fd, buf, MAX_CMD_SIZE);
	if (MAX_CMD_SIZE <= ret || ret == -1) {
		printf("Buffer overflow !!!\n");
		free(buf);
		close(fd);
		return FAIL;
	}

	close(fd);
	
	if(memcmp(buf, AbsoluteMouseReport, sizeof(AbsoluteMouseReport)) == SUCCESS) {
		*MouseModeStatus = ABSOLUTE_MOUSE_MODE;
		MouseMode = ABSOLUTE_MOUSE_MODE;
	} else if(memcmp(buf, RelativeMouseReport, sizeof(RelativeMouseReport)) == SUCCESS) {
		*MouseModeStatus = RELATIVE_MOUSE_MODE;
		MouseMode = RELATIVE_MOUSE_MODE;
	} else {
		*MouseModeStatus = FAIL;
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}
	if(buf != NULL) {
		free(buf);
		buf = NULL;
	}
	return retVal;
}

int UDC_exist(char *filename,char *udc)
{
	uint8 *buf = NULL;
	int ret;
	int fd = -1;

	buf = (uint8 *)malloc(MAX_CMD_SIZE);
	if(!buf) {
		printf("Memory allocation Failed");
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}

	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		printf("Cannot open %s",filename);
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		close(fd);
		return FAIL;
	}

	ret = read(fd, buf, MAX_CMD_SIZE);

	if (MAX_CMD_SIZE <= ret || ret == -1) {
		printf("Buffer overflow !!!\n");
		close(fd);
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}
	close(fd);

	if(memcmp(buf, udc, UDC_SIZE) == 0) {
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return SUCCESS;
	} else {
		if(buf != NULL) {
			free(buf);
			buf = NULL;
		}
		return FAIL;
	}
	
}

int safe_system( const char *command )
/*@globals default_environment@*/
{
    /* Make sure command is not huge */
    if( (long)sizeof( command ) > MAX_CMDLINE )
        return( -1 );

    /* Set environment to something safe */
    //if( clean_environment() == -1 )
      //  return( -1 );

    /* Run command */
    return( system( command ) );
}

/**
 * @brief Get usb device hid resource
 * @param resource - get resource
 * @return int 0 on success, otherwise failure.
 */
int get_hid_resource(uint8 *resource)
{
    FILE *fp;
    char line[MAX_STRING_SIZE];

    fp = fopen(HID_RESOURCE_FILE,"r");
    if(fp == NULL)
    {
        return FAIL;
    }

    while(!feof(fp))
    {
        if(fgets(line,MAX_STRING_SIZE,fp) == NULL)
        {
            break;
        }

        if(strstr(line,"Hid resource :"))
        {
            sscanf(line,"Hid resource :%hhd",resource);
        }
    }
    fclose(fp);
    return 0;
}

/**
 * @brief verify process is exist,if not will unload hid resoure.
 *
 * @return int 0 on success, otherwise failure.
 */
static int verify_hid_handle_list(void)
{
    FILE *fp;
    char line[MAX_STRING_SIZE];
    char *list_ptr;
    int count = 0,status,i;
    uint8 resource;

    fp = fopen(HID_RESOURCE_FILE,"r");
    if(fp == NULL)
    {
        return FAIL;
    }

    while(!feof(fp))
    {
        if(fgets(line,MAX_STRING_SIZE,fp) == NULL)
        {
            break;
        }

        if(strstr(line,","))
        {
			list_ptr = &list[count][0];
            sscanf(line,"%s ,",list_ptr);
            count++;
        }
    }
	fclose(fp);
    status = get_hid_resource(&resource);
	if(status == 0) {
		for(i = 0;i < count;i++) {
			list_ptr = &list[i][0];
			if( IsProcRunning(list_ptr) == 0 ) {
				unload_hid_resource(list_ptr);
			}
		}
	}

    return 0;
}

/**
 * @brief get usb device hid handle list
 *
 * @return int 0 on success, otherwise failure.
 */
static int get_hid_handle_list(void)
{
    FILE *fp;
    char line[MAX_STRING_SIZE];
    char *list_ptr;
    int count = 0;

    fp = fopen(HID_RESOURCE_FILE,"r");
    if(fp == NULL)
    {
        return FAIL;
    }

    while(!feof(fp))
    {
        if(fgets(line,MAX_STRING_SIZE,fp) == NULL)
        {
            break;
        }

        if(strstr(line,","))
        {
			list_ptr = &list[count][0];
            sscanf(line,"%s ,",list_ptr);
            count++;
        }
    }
	fclose(fp);
    return 0;
}

/**
 * @brief save hid device usb resource.
 * @param reg_process - Process name
 * @return int 0 on success, otherwise failure.
 */
int save_hid_resource(char *reg_process)
{
	FILE *fp;
	uint8 resource = 0,exist = 0;
	int status,i;
    char *list_ptr;

	status = get_hid_resource(&resource);
	if(resource > MAX_SUPPORT_PROCESS) {
		printf("Maxmium process not support %s\n",reg_process);
		return FAIL;
	}

	status = get_hid_handle_list();

	fp = fopen(HID_RESOURCE_FILE,"w+");
	if(fp == NULL)
	{
		return FAIL;
	}

	if(status == 0) {
		fprintf(fp,"Handle list:{\n");

		for(i = 0;i < resource;i++) {
			list_ptr = &list[i][0];
			if(strcmp(reg_process,list_ptr) != 0) {
				fprintf(fp,"%s ,\n",list_ptr);
			} else {
				exist = 1;
			}
		}
		fprintf(fp,"%s ,\n",reg_process);
	} else {
		fprintf(fp,"Handle list:{\n");
		fprintf(fp,"%s ,\n",reg_process);
	}

	fprintf(fp,"}\n");
	if(!exist)
		resource++;
	fprintf(fp,"Hid resource :%x\n",resource);

	fclose(fp);
	return SUCCESS;
}

/**
 * @brief unload hid device usb resource.
 * @param reg_process - Process name
 * @return int 0 on success, otherwise failure.
 */
int unload_hid_resource(char *reg_process)
{
	FILE *fp;
	uint8 resource = 0;
	int i;
    char *list_ptr;

	get_hid_resource(&resource);
	fp = fopen(HID_RESOURCE_FILE,"w+");
	if(fp == NULL)
	{
		printf("Unable to open file %s \n",HID_RESOURCE_FILE);
		return FAIL;
	}

	for (i = 0;i < resource; i++) {
		list_ptr = &list[i][0];
		if(strcmp(reg_process,list_ptr) == 0) {
			*list_ptr = NULL;
		}
	}

    if(resource > 0) {
		fprintf(fp,"Handle list:{\n");
		for(i = 0;i < resource;i++) {
			list_ptr = &list[i][0];
			if(*list_ptr != NULL)
				fprintf(fp,"%s ,\n",list_ptr);
		}
		fprintf(fp,"}\n");
		resource--;

	}
	fprintf(fp,"Hid resource :%x\n",resource);
	fclose(fp);

	if(resource == 0) {
		safe_system(RM_HID_RESOURCE_FILE);
	}
	return SUCCESS;
}

/**
 * @brief This function helps to find whether a process in running or not
 *           Equivalent to 'ps ax'
 * @param procarg - Process name
 * @return Returns the instance of the running process
 *             Returns '-1' on error
 */
static int IsProcRunning( char *procarg)
{
    DIR *dirpath;
    struct dirent *result=NULL;
    char filename[FILENAME_MAX] = ".";
    struct stat finfo;
    FILE *fp;
    long pid;
    char processname [128] = {0};
    char state, *strptr;
    int inst=0;

    strptr = strrchr(procarg,'/');
    if(strptr == NULL)
    {
            strptr = procarg;
    }
    else
    {
        strptr++;
    }

    dirpath = opendir("/proc");
    if(dirpath == NULL)
    {
            printf("Error in Opening directory /proc");
            return -1;
    }

    do
    {
	errno = 0;
	result = readdir(dirpath); 
	if (result == NULL)
	{
	    if(errno != 0)/*Check if errno is set*/
	    {
		closedir(dirpath);
		printf("Function : readdir() failed\n");
		return -1;
	    }
	    /*Hope end of directory is reached */
	    break;
	}
        if((strcmp(".",result->d_name)== 0) || (strcmp("..",result->d_name) == 0))
        {
            continue;
        }

        snprintf(filename,FILENAME_MAX,"%s/%s","/proc",result->d_name);

        if(stat(filename,&finfo) == -1)
        {
            continue;
        }

        if(S_ISDIR(finfo.st_mode))
        {
            if(isalpha(result->d_name) < 0)
            {
                continue;
            }

            snprintf(filename,FILENAME_MAX,"%s/%s/%s","/proc",result->d_name,"stat");
            fp = fopen(filename,"r");
            if(fp == NULL)
            {
                continue;
            }
            if(fscanf(fp,"%ld (%[^)]) %c",&pid,processname,&state) != 3)
            {
                fclose(fp);
                continue;
            }

            if(strlen(processname) == strlen(strptr))
            {
                if(strncmp(processname,strptr,strlen(strptr))== 0)
                {
                    inst++;
                }
            }
            fclose(fp);
        }
        else
        {
            continue;
        }
    }while(result != NULL);
    closedir(dirpath);
    return inst;
}

/**
 * @brief This function helps to io control with USB HIDs.
 * @param cmd - determine the command of ioctl
 * @param data - data for ioctl.
 * @return Returns the value of ioctl().
 *             Returns '-1' on error
 */
static int usb_hid_ioctl(int cmd, uint8 *data)
{
	int32_t retval = 0;
	int usbg_fd = -1;

	usbg_fd = open(KEYBOARD_DEV_NODE, O_RDONLY | O_SYNC);

	if(0 > usbg_fd) {
		printf("no such devie: /dev/hidg1\n");
		printf("errno: %d [%s]\n", errno, strerror(errno));
		close(usbg_fd);
		return FAIL;
	}
	retval = ioctl(usbg_fd, cmd, data);

	close(usbg_fd);
	return retval;
}

/**
 * @brief This function helps to read LED status of HID keyboard.
 * @param Status - the LEDs status of HID keyboard, include numlock, caps lock, scroll lock, etc.
 * @param no_wait - determine if driver need to wait for host request during read status.
 * @return Returns the value of ioctl().
 *             Returns '-1' on error
 */
int ReadLEDStatus(uint8 *Status,uint8 no_wait)
{
	int32_t retval = 0;
	
	if(UDC_exist(HID_PATH, HID_UDC) == FAIL) {
		return FAIL;
	}
	
	if(no_wait) {
		retval = usb_hid_ioctl(USB_KEYBD_LED_NO_WAIT, Status);
	} else {
		retval = usb_hid_ioctl(USB_KEYBD_LED, Status);
	}
	if(retval != 0) {
		*Status = 0;
	}
	return retval;
}

