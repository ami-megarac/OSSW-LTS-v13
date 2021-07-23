
#ifndef HID_H
#define HID_H

typedef char CHAR;
typedef char int8;
typedef signed char int8s;
typedef unsigned char uint8;

#ifndef __KERNEL__
typedef unsigned char u8;
#endif

typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef unsigned char UINT8;

typedef short int16;
typedef short INT16;
typedef unsigned short uint16;

#ifndef __KERNEL__
typedef unsigned short u16;
#endif

typedef int int32;
typedef unsigned int uint32;
typedef unsigned short TWOBYTES;
typedef unsigned short UINT16;
typedef unsigned short WORD;

#ifndef __KERNEL__
typedef unsigned int u32;
#endif

typedef unsigned int FOURBYTES;
typedef unsigned long long uint64;

/*Ioctls */

#define USB_KEYBD_DATA		_IOC(_IOC_WRITE,'U',0x11,0x3FFF)
#define USB_KEYBD_LED		_IOC(_IOC_READ ,'U',0x12,0x3FFF)
#define USB_KEYBD_EXIT		_IOC(_IOC_WRITE,'U',0x13,0x3FFF)
#define USB_KEYBD_LED_NO_WAIT	_IOC(_IOC_READ,'U',0x14,0x3FFF)
#define USB_KEYBD_LED_RESET	_IOC(_IOC_WRITE,'U',0x15,0x3FFF)

#define USB_MOUSE_DATA		_IOC(_IOC_WRITE,'U',0x21,0x3FFF)
#define MOUSE_ABS_TO_REL        _IOC(_IOC_WRITE,'U',0x22,0x3FFF)
#define MOUSE_REL_TO_ABS        _IOC(_IOC_WRITE,'U',0x23,0x3FFF)
#define MOUSE_GET_CURRENT_MODE  _IOC(_IOC_WRITE,'U',0x24,0x3FFF)



#define USB_DEVICE_DISCONNECT	_IOC(_IOC_WRITE,'U',0xE3,0x3FFF)
#define USB_DEVICE_RECONNECT	_IOC(_IOC_WRITE,'U',0xE4,0x3FFF)

#define USB_GET_INTERFACES	_IOC(_IOC_READ,'U',0xF1,0x3FFF)
#define USB_REQ_INTERFACE	_IOC(_IOC_READ,'U',0xF2,0x3FFF)
#define USB_REL_INTERFACE	_IOC(_IOC_WRITE,'U',0xF3,0x3FFF)


#define USB_DEVICE                    "/dev/usb"
#define IUSB_KEY_FILE "/var/adviser_resource"
#define REMOVE_IUSB_KEY_FILE "rm /var/adviser_resource"
#define MAX_STRING_SIZE 80

#define IUSB_DEVICE_KEYBD	0x30
#define IUSB_DEVICE_MOUSE	0x31

#define LOCK_TYPE_SHARED	  0x2
#define LOCK_TYPE_EXCLUSIVE	  0x1
#define MAX_MOUSE_REPORT_SIZE 6
#define MAX_KB_REPORT_SIZE    8
#define IUSB_PROTO_KEYBD_DATA 0x10

#define NUM_LOCK 	0x01
#define CAPS_LOCK 	0x02
#define SCR_LOCK 	0x04

#define CAPS_LOCK_KEY_CODE 	0xFFE5
#define NUM_LOCK_KEY_CODE 	0xFF7F

#define NUMPAD_0 			0xFFB0
#define NUMPAD_9 			0xFFB9
#define NUMPAD_DECIMAL		0xFFAE
#define NUMPAD_HOME			0xFF95
#define NUMPAD_DELETE		0xFF9F

#define RFB_Shift_L		0xFFE1	/* Left shift */
#define RFB_Shift_R		0xFFE2	/* Right shift */
#define RFB_Control_L		0xFFE3	/* Left control */
#define RFB_Control_R		0xFFE4	/* Right control */
#define RFB_Meta_L		0xFFE7	/* Left meta */
#define RFB_Meta_R		0xFFE8	/* Right meta */
#define RFB_Alt_L		0xFFE9	/* Left alt */
#define RFB_Alt_R		0xFFEA	/* Right alt */
#define RFB_Super_L		0xFFEB	/* Left super */
#define RFB_Super_R		0xFFEC	/* Right super */

#define  MOD_LEFT_CTRL		0x01
#define  MOD_RIGHT_CTRL		0x10
#define  MOD_LEFT_SHIFT		0x02
#define  MOD_RIGHT_SHIFT 	0x20
#define  MOD_LEFT_ALT		0x04
#define  MOD_RIGHT_ALT		0x40
#define  MOD_LEFT_WIN		0x08
#define  MOD_RIGHT_WIN		0x80

#define IUSB_SIG "IUSB    "

#ifdef __GNUC__
#define PACKED __attribute__ ((packed))
#else
#define PACKED
#pragma pack( 1 )
#endif

#define USB_GET_IUSB_DEVICES	_IOC(_IOC_READ, 'U',0x00,0x3FFF)
#define USB_GET_CONNECT_STATE	_IOC(_IOC_READ, 'U',0x01,0x3FFF)
#define USB_KEYBD_DATA		_IOC(_IOC_WRITE,'U',0x11,0x3FFF)
#define USB_KEYBD_LED		_IOC(_IOC_READ ,'U',0x12,0x3FFF)
#define USB_KEYBD_EXIT		_IOC(_IOC_WRITE,'U',0x13,0x3FFF)
#define USB_KEYBD_LED_NO_WAIT	_IOC(_IOC_READ,'U',0x14,0x3FFF)
#define USB_KEYBD_LED_RESET	_IOC(_IOC_WRITE,'U',0x15,0x3FFF)

#define USB_MOUSE_DATA		_IOC(_IOC_WRITE,'U',0x21,0x3FFF)
#define MOUSE_ABS_TO_REL        _IOC(_IOC_WRITE,'U',0x22,0x3FFF)
#define MOUSE_REL_TO_ABS        _IOC(_IOC_WRITE,'U',0x23,0x3FFF)
#define MOUSE_GET_CURRENT_MODE  _IOC(_IOC_WRITE,'U',0x24,0x3FFF)
#define USB_GET_ADDRESS  	_IOC(_IOC_WRITE,'U',0x25,0x3FFF) 

#define USB_CDROM_REQ		_IOC(_IOC_READ, 'U',0x31,0x3FFF)
#define USB_CDROM_RES		_IOC(_IOC_WRITE,'U',0x32,0x3FFF)
#define USB_CDROM_EXIT		_IOC(_IOC_WRITE,'U',0x33,0x3FFF)
#define USB_CDROM_ACTIVATE	_IOC(_IOC_WRITE,'U',0x34,0x3FFF)

#define USB_HDISK_REQ		_IOC(_IOC_READ, 'U',0x41,0x3FFF)
#define USB_HDISK_RES		_IOC(_IOC_WRITE,'U',0x42,0x3FFF)
#define USB_HDISK_EXIT		_IOC(_IOC_WRITE,'U',0x43,0x3FFF)
#define USB_HDISK_ACTIVATE	_IOC(_IOC_WRITE,'U',0x44,0x3FFF)
#define USB_HDISK_SET_TYPE	_IOC(_IOC_WRITE,'U',0x45,0x3FFF)
#define USB_HDISK_GET_TYPE	_IOC(_IOC_READ, 'U',0x46,0x3FFF)

#define USB_FLOPPY_REQ		_IOC(_IOC_READ, 'U',0x51,0x3FFF)
#define USB_FLOPPY_RES		_IOC(_IOC_WRITE,'U',0x52,0x3FFF)
#define USB_FLOPPY_EXIT		_IOC(_IOC_WRITE,'U',0x53,0x3FFF)
#define USB_FLOPPY_ACTIVATE	_IOC(_IOC_WRITE,'U',0x54,0x3FFF)

#define USB_VENDOR_REQ		_IOC(_IOC_READ, 'U',0x61,0x3FFF)
#define USB_VENDOR_RES		_IOC(_IOC_WRITE,'U',0x62,0x3FFF)
#define USB_VENDOR_EXIT		_IOC(_IOC_WRITE,'U',0x63,0x3FFF)

#define USB_LUNCOMBO_ADD_CD	_IOC(_IOC_WRITE,'U',0x91,0x3FFF) 
#define USB_LUNCOMBO_ADD_FD	_IOC(_IOC_WRITE,'U',0x92,0x3FFF) 
#define USB_LUNCOMBO_ADD_HD	_IOC(_IOC_WRITE,'U',0x93,0x3FFF) 
#define USB_LUNCOMBO_ADD_VF	_IOC(_IOC_WRITE,'U',0x94,0x3FFF) 
#define USB_LUNCOMBO_RST_IF	_IOC(_IOC_WRITE,'U',0x95,0x3FFF) 

#define USB_CDROM_ADD_CD	_IOC(_IOC_WRITE,'U',0xA1,0x3FFF) 
#define USB_CDROM_RST_IF	_IOC(_IOC_WRITE,'U',0xA2,0x3FFF) 

#define USB_FLOPPY_ADD_FD	_IOC(_IOC_WRITE,'U',0xA6,0x3FFF) 
#define USB_FLOPPY_RST_IF	_IOC(_IOC_WRITE,'U',0xA7,0x3FFF) 

#define USB_HDISK_ADD_HD	_IOC(_IOC_WRITE,'U',0xAB,0x3FFF) 
#define USB_HDISK_RST_IF	_IOC(_IOC_WRITE,'U',0xAC,0x3FFF) 

#define USB_DEVICE_DISCONNECT	_IOC(_IOC_WRITE,'U',0xE3,0x3FFF)
#define USB_DEVICE_RECONNECT	_IOC(_IOC_WRITE,'U',0xE4,0x3FFF)

#define USB_GET_INTERFACES	_IOC(_IOC_READ,'U',0xF1,0x3FFF)
#define USB_REQ_INTERFACE	_IOC(_IOC_READ,'U',0xF2,0x3FFF)
#define USB_REL_INTERFACE	_IOC(_IOC_WRITE,'U',0xF3,0x3FFF)
#define USB_DISABLE_ALL_DEVICE _IOC(_IOC_WRITE,'U',0xF4,0x3FFF)
#define USB_ENABLE_ALL_DEVICE _IOC(_IOC_WRITE,'U',0xF5,0x3FFF)
#define USB_GET_ALL_DEVICE_STATUS _IOC(_IOC_WRITE,'U',0xF6,0x3FFF)
//USB 2.0 enabling/disabling support
#define USB_DISABLE_MEDIA_DEVICE _IOC(_IOC_WRITE,'U',0xF7,0x3FFF)
#define USB_ENABLE_MEDIA_DEVICE _IOC(_IOC_WRITE,'U',0xF8,0x3FFF)

#define BPP 4
#define MAX_CURSOR_AREA 64
#define MIN_CURSOR_AREA 32
#define CURSOR_PKT_MIN_LENGTH 13
#define CURSOR_PKT_MAX_LENGTH (CURSOR_PKT_MIN_LENGTH + 8192)

#define QUEUE_FULL_WARNING 100
#define QUEUE_FULL_WAIT_TIME 3 /* Time in Seconds */

typedef struct {
	uint32 key;
	uint8 DevNo;
	uint8 IfNum;
} USBHid_T;

/**
 * @struct  IUSB_HEADER packet format
 **/
typedef struct {
	u8 Signature[8]; // signature "IUSB    "
	u8 Major;
	u8 Minor;
	u8 HeaderLen; // Header length
	u8 HeaderCheckSum;
	u32 DataPktLen;
	u8 ServerCaps;
	u8 DeviceType;
	u8 Protocol;
	u8 Direction;
	u8 DeviceNo;
	u8 InterfaceNo;
	u8 ClientData;
	u8 Instance;
	u32 SeqNo;
	u32 Key;
}PACKED IUSB_HEADER;

typedef struct {
	IUSB_HEADER Header;
	u8 DataLen;
	u8 Data;
}PACKED IUSB_HID_PACKET;

/*************************iUSB Device Information **************************/
typedef struct {
	uint8 DeviceType;
	uint8 DevNo;
	uint8 IfNum;
	uint8 LockType;
	uint8 Instance;
}PACKED IUSB_DEVICE_INFO;

typedef struct {
	IUSB_DEVICE_INFO DevInfo;
	uint32 Key;
}PACKED IUSB_REQ_REL_DEVICE_INFO;

typedef struct {
	IUSB_HEADER Header;
	uint8 DevCount;
	IUSB_DEVICE_INFO DevInfo; /* for each device */
}PACKED IUSB_DEVICE_LIST;

typedef struct {
	uint8 DeviceType;
	uint8 LockType;
	IUSB_DEVICE_LIST iUSBdevList;
}PACKED IUSB_FREE_DEVICE_INFO;

typedef struct {
	uint32 Key;
	IUSB_DEVICE_INFO DevInfo;
	uint8 Data;
}PACKED IUSB_IOCTL_DATA;

// Mouse report
typedef struct {
	uint8 Event;
	uint16 X;
	uint16 Y;
	uint8 Scroll;
}PACKED USB_ABS_MOUSE_REPORT_T;

// Keyboard report
typedef struct 
{
	unsigned char Modifier;
	unsigned char Reserved;
	unsigned char KeyCode[6];
} USB_KEYBD_PKT;

int do_usb_ioctl(int cmd, u8 *data);
int GetIUSBLEDPkt(u8 *LEDstatus, int no_wait);
void SetLEDStatus();
void SendKeyEvent(uint32 key, int8 down);
int get_usb_resource(USBHid_T *keybd, USBHid_T *mouse);
int OpenUSBDevice(void);
void CloseUSBDevice(void);
int SetAuthInfo(IUSB_REQ_REL_DEVICE_INFO iUSBDevInfo, u8* Data,u8 IsiUSBHeader);
int RequestiUSBDevice(IUSB_REQ_REL_DEVICE_INFO* iUSBDevInfo);
int init_usb_resources(void);
int save_usb_resource(void);
u8*AddAuthInfo(int cmd, IUSB_REQ_REL_DEVICE_INFO* pdev_info, u8* data);
u8 CalculateModulo100(u8 *Buff, int BuffLen);
int GetMouseEvent(uint32 nEvent);
int mousePkt(int buttonMask, int x, int y, int maxx, int maxy);
void handleKbdEvent(int8 down, uint32 key);
int ConvertRFBtoUSBKey(USB_KEYBD_PKT *KeybdInfo, int8 down, uint32 key);

#define DIRABS_MAX_SCALED_X  32767
#define DIRABS_MAX_SCALED_Y  32767

#endif
