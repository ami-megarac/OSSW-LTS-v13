
#ifndef LIBUSBGADGET_VNC_H
#define LIBUSBGADGET_VNC_H

#ifdef __GNUC__
#define PACKED __attribute__ ((packed))
#else
#define PACKED
#pragma pack( 1 )
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define MAX_CMD_SIZE		(1024)

#ifndef SUCCESS
#define SUCCESS			0
#endif
#ifndef FAIL
#define FAIL			-1
#endif
#ifndef RESOURCE_BUSY
#define RESOURCE_BUSY	-2
#endif
#ifndef MAX_CMDLINE
#define MAX_CMDLINE 1024
#endif

/* device folder name*/
#define HID_PATH   				"/sys/kernel/config/usb_gadget/hid/UDC"

#define HID_UDC					"1e6a0000.usb-vhub:p4"

/* USB HID Resource file*/
#define HID_RESOURCE_FILE 		"/var/usb_hid_resource"
#define RM_HID_RESOURCE_FILE 	"rm /var/usb_hid_resource"

#define MAX_STRING_SIZE 	80
#define MAX_SUPPORT_PROCESS	3

#define UDC_SIZE			20
#define ASPEED_PORT			7
/* Mouse mode descrptions */
#define RELATIVE_MOUSE_MODE 1
#define ABSOLUTE_MOUSE_MODE 2
#define OTHER_MOUSE_MODE 	3

#define ENABLE	0x01
#define DISABLE	0x00
#define MAX_MOUSE_DATA_SIZE		6
#define MAX_KEYBD_DATA_SIZE		8

//USB HID
int hid_open(char *reg_process);
int hid_stop(char *reg_process);
int SendKeybdData(uint8 *buf);
int SendMouseData(uint8 *buf);
int changeMouseAbs2Rel(void);
int changeMouseRel2Abs(void);
int getCurrentMouseMode(uint8 *MouseModeStatus);
int UDC_exist(char *filename,char *udc);
int save_hid_resource(char *reg_process);
int unload_hid_resource(char *reg_process);
int get_hid_resource(uint8 *resource);
int ReadLEDStatus(uint8 *Status,uint8 no_wait);

#endif
