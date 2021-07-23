#include "hid.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#if defined(SOC_AST2600)
#include <errno.h>
#include "libusbgadget_vnc.h"
#endif

IUSB_IOCTL_DATA KeybdIoctlData;
IUSB_IOCTL_DATA MouseIoctlData;
IUSB_REQ_REL_DEVICE_INFO iUSBKeybdDevInfo;
IUSB_REQ_REL_DEVICE_INFO iUSBMouseDevInfo;
USB_KEYBD_PKT prevkey;

int gUsbDevice = 0;          // handle to the USB device
int m_btn_status;
int seqno;

u8 g_led_status = 0x0;

static unsigned char RFB2USB_Map[256] =
{
		/*X0   X1   X2   X3   X4   X5   X6   X7   X8   X9   XA   XB   XC   XD   XE   XF  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,0x2B,0x00,0x9C,0x00,0x58,0x00,0x00, /* X0  */
		0x00,0x00,0x00,0x48,0x47,0x9A,0x00,0x00,0x00,0x00,0x00,0x29,0x00,0x00,0x00,0x00, /* X1  */
		0x00,0x00,0x00,0x00,0x00,0x93,0x92,0x00,0x00,0x00,0x94,0x00,0x00,0x00,0x00,0x00, /* X2  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X3  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X4  */
		0x4A,0x50,0x52,0x4F,0x51,0x4B,0x4E,0x4D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X5  */
		0x77,0x46,0x74,0x49,0x00,0x7A,0x00,0x65,0x7E,0x78,0x75,0x00,0x00,0x00,0x00,0x00, /* X6  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x53, /* X7  */
		0x2C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2B,0x00,0x00,0x00,0x58,0x00,0x00, /* X8  */
		0x00,0x3A,0x3B,0x3C,0x3D,0x5F,0x5C,0x60,0x5E,0x5A,0x61,0x5B,0x59,0x00,0x62,0x63, /* X9  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x57,0x00,0x56,0x63,0x54, /* XA  */
		0x62,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,0x60,0x61,0x00,0x00,0x00,0x67,0x3A,0x3B, /* XB  */
		0x3C,0x3D,0x3E,0x3F,0x40,0x41,0x42,0x43,0x44,0x45,0x68,0x69,0x6A,0x6B,0x6C,0x6D, /* XC  */
		0x6E,0x6F,0x70,0x71,0x72,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XD  */
		0x00,0x00,0x00,0x00,0x00,0x39,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XE  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4C, /* XF  */
};

static unsigned char Ascii2USB_Map[256] =
{

		/*X0   X1   X2   X3   X4   X5   X6   X7   X8   X9   XA   XB   XC   XD   XE   XF  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A,0x2B,0x28,0x00,0x00,0x28,0x00,0x00, /* X0  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X1  */
		0x2C,0x1E,0x34,0x20,0x21,0x22,0x24,0x34,0x26,0x27,0x25,0x2E,0x36,0x2D,0x37,0x38, /* X2  */
		0x27,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x33,0x33,0x36,0x2E,0x37,0x38, /* X3  */
		0x1F,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12, /* X4  */
		0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x2F,0x31,0x30,0x23,0x2D, /* X5  */
		0x35,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12, /* X6  */
		0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x2F,0x31,0x30,0x35,0x63, /* X7  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X8  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* X9  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XA  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XB  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XC  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XD  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XE  */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* XF  */
};

int get_usb_resource(USBHid_T *keybd, USBHid_T *mouse) {
	FILE *fp;
	char line[MAX_STRING_SIZE] = {0};
	unsigned int dev, ifnum;

	fp = fopen(IUSB_KEY_FILE, "r");
	if (fp == NULL) {
		printf("\nUnable to open file %s \n", IUSB_KEY_FILE);
		return -1;
	}

	while (!feof(fp)) {
		if (fgets(line, MAX_STRING_SIZE, fp) == NULL) {
			break;
		}

		if (strstr(line, "Hid :")) {
			sscanf(line, "Hid :%x %x %x", &keybd->key, &dev, &ifnum);
			keybd->DevNo = (uint8) dev;
			keybd->IfNum = (uint8) ifnum;
		}

		if (strstr(line, "Mouse :")) {
			sscanf(line, "Mouse :%x %x %x", &mouse->key, &dev, &ifnum);
			mouse->DevNo = (uint8) dev;
			mouse->IfNum = (uint8) ifnum;
		}
	}
	fclose(fp);
	return 0;
}

int ReleaseiUSBDevice (IUSB_REQ_REL_DEVICE_INFO* iUSBDevInfo)
{
			if (gUsbDevice > 0) {
				if (ioctl (gUsbDevice, USB_REL_INTERFACE, iUSBDevInfo))
				{
					printf("\nError: ReleaseiUSBDevice() USB_REL_INTERFACE  IOCTL Failed\n");
					close(gUsbDevice);
					return -1;
				}
	}
	return 0;
}

int OpenUSBDevice(void) {
	gUsbDevice = open(USB_DEVICE, O_RDWR);
	if (gUsbDevice < 0) {
		printf("\nOpenUSBDevice failed \n");
		fflush(stdout);
		return -1;
	}
	return 0;
}
void CloseUSBDevice(void) {
	if (gUsbDevice > 0) {
		do_usb_ioctl(USB_KEYBD_EXIT, NULL);
		ReleaseiUSBDevice(&iUSBMouseDevInfo);
		ReleaseiUSBDevice (&iUSBKeybdDevInfo);
		close(gUsbDevice);
		if( system(REMOVE_IUSB_KEY_FILE) < 0 ) {
			printf("Unable to remove usb key source file");
		}
		gUsbDevice = -1;
	}
}

int SetAuthInfo(IUSB_REQ_REL_DEVICE_INFO iUSBDevInfo, u8* Data, u8 IsiUSBHeader) {
	IUSB_IOCTL_DATA *IoctlDataPtr;
	IUSB_HEADER *iUSBHeaderPtr;

	iUSBHeaderPtr = (IUSB_HEADER*) Data;
	IoctlDataPtr = (IUSB_IOCTL_DATA*) Data;

	if (IsiUSBHeader) {
		memcpy((void*) &iUSBHeaderPtr->Key, (void*) &iUSBDevInfo.Key,
				sizeof(iUSBDevInfo.Key));
		iUSBHeaderPtr->DeviceNo = iUSBDevInfo.DevInfo.DevNo;
		iUSBHeaderPtr->InterfaceNo = iUSBDevInfo.DevInfo.IfNum;
	} else {
		IoctlDataPtr->Key = iUSBDevInfo.Key;
		IoctlDataPtr->DevInfo.DevNo = iUSBDevInfo.DevInfo.DevNo;
		IoctlDataPtr->DevInfo.IfNum = iUSBDevInfo.DevInfo.IfNum;
	}

	return 0;
}

int RequestiUSBDevice(IUSB_REQ_REL_DEVICE_INFO* iUSBDevInfo) {
	int usbfd;
	u8 Buffer[2048];
	IUSB_DEVICE_LIST* iUSBDeviceList;

	/* Open the USB Device */
	usbfd = open(USB_DEVICE, O_RDWR);
	if (usbfd < 0) {
		printf("\n Error in opening USB Device\n");
		return -1;
	}

	memset(Buffer, 0, sizeof(Buffer));
	((IUSB_FREE_DEVICE_INFO*) Buffer)->DeviceType = iUSBDevInfo->DevInfo.DeviceType;
	((IUSB_FREE_DEVICE_INFO*) Buffer)->LockType = iUSBDevInfo->DevInfo.LockType;

	if (ioctl(usbfd, USB_GET_INTERFACES, Buffer)) {
		printf ("Error: ObtainIfcInfo() USB_GET_INTERFACES IOCTL failed\n");
		close(usbfd);
		return -1;
	}

	iUSBDeviceList = &((IUSB_FREE_DEVICE_INFO*) Buffer)->iUSBdevList;
	if (0 == iUSBDeviceList->DevCount) {
		printf("\nError: ObtainIfcInfo() iUSB Device of Type 0x%02x not available\n",
				iUSBDevInfo->DevInfo.DeviceType);
		close(usbfd);
		return -1;
	}

	iUSBDevInfo->DevInfo.DevNo = iUSBDeviceList->DevInfo.DevNo;
	iUSBDevInfo->DevInfo.IfNum = iUSBDeviceList->DevInfo.IfNum;

	if (ioctl(usbfd, USB_REQ_INTERFACE, iUSBDevInfo)) {
		printf("\nError: RequestiUSBDevice() USB_REQ_INTERFACE IOCTL failed\n");
		close(usbfd);
		return -1;
	}

	close(usbfd);
	return 0;
}

int save_usb_resource(void) {
	FILE *fp;

	fp = fopen(IUSB_KEY_FILE, "w");
	if (fp == NULL) {
		printf("Unable to open file %s \n", IUSB_KEY_FILE);
		return -1;
	}

	fprintf(fp, "Hid :%x %x %x\n", iUSBKeybdDevInfo.Key,
			iUSBKeybdDevInfo.DevInfo.DevNo, iUSBKeybdDevInfo.DevInfo.IfNum);
	fprintf(fp, "Mouse :%x %x %x", iUSBMouseDevInfo.Key,
			iUSBMouseDevInfo.DevInfo.DevNo, iUSBMouseDevInfo.DevInfo.IfNum);
	fclose(fp);
	return 0;
}

int init_usb_resources(void) {

	USBHid_T keybd = { 0, 0, 0 };
	USBHid_T mouse = { 0, 0, 0 };
	FILE *fp;

	if (0 != OpenUSBDevice()) {
		fprintf(stderr, "Unable to open USB device. Closing VNCServer\n");
		return -1;
	}

	iUSBKeybdDevInfo.DevInfo.DeviceType = IUSB_DEVICE_KEYBD;
	iUSBKeybdDevInfo.DevInfo.LockType = LOCK_TYPE_EXCLUSIVE;
	iUSBMouseDevInfo.DevInfo.DeviceType = IUSB_DEVICE_MOUSE;
	iUSBMouseDevInfo.DevInfo.LockType = LOCK_TYPE_EXCLUSIVE;

	fp = fopen(IUSB_KEY_FILE, "r");
	//kvm and vnc share the same usb key file which contain usb interface information.
	//if key file present, then kvm have acquired the kb and mouse usb interface.
	//So get that usb resource and use it.
	if (fp != NULL)
	{
		get_usb_resource(&keybd, &mouse);
		if (keybd.key != 0) {
			iUSBKeybdDevInfo.Key = keybd.key;
			iUSBKeybdDevInfo.DevInfo.DevNo = keybd.DevNo;
			iUSBKeybdDevInfo.DevInfo.IfNum = keybd.IfNum;
		}
		else{
			printf("\n Error: keyboard interface not found \n");
		}

		get_usb_resource(&keybd, &mouse);
		if (mouse.key != 0) {
			iUSBMouseDevInfo.Key = mouse.key;
			iUSBMouseDevInfo.DevInfo.DevNo = mouse.DevNo;
			iUSBMouseDevInfo.DevInfo.IfNum = mouse.IfNum;
		}
		else{
			printf("\n Error: mouse interface not found \n");
		}
		fclose(fp);
	}
	else{
		if (0 != RequestiUSBDevice(&iUSBKeybdDevInfo)) {
			/*Helps in retrieving usb resources that needs to be freed
			 properly before requesting */
			get_usb_resource(&keybd, &mouse);
			if (keybd.key != 0) {
				iUSBKeybdDevInfo.Key = keybd.key;
				iUSBKeybdDevInfo.DevInfo.DevNo = keybd.DevNo;
				iUSBKeybdDevInfo.DevInfo.IfNum = keybd.IfNum;
				ReleaseiUSBDevice(&iUSBKeybdDevInfo);
				if (0 != RequestiUSBDevice(&iUSBKeybdDevInfo)) {
					CloseUSBDevice();
					return -1;
				}
			} else {
				printf("\nError Unable to find iUSB KEYBD Device \n");
				CloseUSBDevice();
				return -1;
			}
		}

		if (0 != RequestiUSBDevice(&iUSBMouseDevInfo)) {
			/*Helps in retrieving usb resources that needs to be freed
			 properly before requesting */
			get_usb_resource(&keybd, &mouse);
			if (mouse.key != 0) {
				iUSBMouseDevInfo.Key = mouse.key;
				iUSBMouseDevInfo.DevInfo.DevNo = mouse.DevNo;
				iUSBMouseDevInfo.DevInfo.IfNum = mouse.IfNum;
				ReleaseiUSBDevice(&iUSBMouseDevInfo);
				if (0 != RequestiUSBDevice(&iUSBMouseDevInfo)) {
					CloseUSBDevice();
					return -1;
				}
			} else {
				printf("\nError Unable to find iUSB Mouse Device \n");
				CloseUSBDevice();
				return -1;
			}
		}
	}
	seqno = 0;
	m_btn_status = 0;
	return 0;
}

u8*
AddAuthInfo(int cmd, IUSB_REQ_REL_DEVICE_INFO* pdev_info, u8* data) {
	switch (cmd) {
	case USB_MOUSE_DATA:
		SetAuthInfo(iUSBMouseDevInfo, data, 1);
		return data;
	case MOUSE_ABS_TO_REL:
	case MOUSE_REL_TO_ABS:
	case MOUSE_GET_CURRENT_MODE:
		SetAuthInfo(iUSBMouseDevInfo, (u8*) &MouseIoctlData, 0);
		return (u8*) &MouseIoctlData;
	case USB_KEYBD_DATA:
	case USB_KEYBD_LED:
	case USB_KEYBD_LED_NO_WAIT:
	case USB_KEYBD_LED_RESET:
		SetAuthInfo(iUSBKeybdDevInfo, data, 1);
		return data;
	case USB_DEVICE_DISCONNECT:
	case USB_DEVICE_RECONNECT:
		SetAuthInfo(*pdev_info, data, 0);
		return data;
	case USB_KEYBD_EXIT:
		SetAuthInfo(iUSBKeybdDevInfo, (u8*) &KeybdIoctlData, 0);
		return (u8*) &KeybdIoctlData;
	default:
		printf("Error: AddAuthInfo() received unsupported IOCTL command\n");
	}
	return NULL;
}

int do_usb_ioctl(int cmd, u8 *data) {
	int va = -1;
	u8* ModifiedData;

	if (gUsbDevice > 0) {
		ModifiedData = (u8*) AddAuthInfo(cmd, NULL, data);
		if (ModifiedData == NULL)
			return va;
		va = ioctl(gUsbDevice, cmd, ModifiedData);
	}

	return va;
}

/**
 * Get the host keyboard LED status
 */
int GetIUSBLEDPkt(u8 *LEDstatus, int no_wait)
{
	int ioctl_request_code,ret = 0;

	char pBuffer[sizeof(IUSB_HID_PACKET)];
	IUSB_HID_PACKET* pHIDPacket = (IUSB_HID_PACKET*)&pBuffer[0];

	memset(pBuffer, 0, sizeof(pBuffer) );

	pHIDPacket->DataLen   = 1;
	pHIDPacket->Data   = 0;

	ioctl_request_code = no_wait ? USB_KEYBD_LED_NO_WAIT : USB_KEYBD_LED;
#if !defined(SOC_AST2600)
	if (0 != do_usb_ioctl(ioctl_request_code, (u8 *)pHIDPacket) )
#else
	ret = ReadLEDStatus((u8 *)&pHIDPacket->Data, no_wait);
	if (0 != ret )  // no_wait?
#endif
	{
		// printf("Error retrieving LED status from the driver\n");
		return -1;
	}

	*LEDstatus = pHIDPacket->Data;

	return 0;
}

/**
 * Get the Host keyboard LED status and set LED status value in the vnc server
 */
void SetLEDStatus(){
	u8 LEDstatus = 0;
	//Get Host keyboard LED status and set it as the LED status in vnc server
	if(GetIUSBLEDPkt(&LEDstatus, 1) == -1){
		LEDstatus = 0;
	}
	g_led_status = LEDstatus;
}
u8 CalculateModulo100(u8 *Buff, int BuffLen) {
	u8 ChkSum = 0;
	int i;

	for (i = 0; i < BuffLen; i++)
		ChkSum += Buff[i];
	return (-ChkSum);
}


int mousePkt(int buttonMask, int x, int y, int maxx, int maxy) {
	int ioctl_retval = -1;
	char pIusbPkt[sizeof(IUSB_HID_PACKET) - 1 + MAX_MOUSE_REPORT_SIZE] = { 0 };
	USB_ABS_MOUSE_REPORT_T mouseRep = { 0 };
#if defined(SOC_AST2600)
	int ret = -1;
#endif
	IUSB_HID_PACKET* pHIDPacket = (IUSB_HID_PACKET*) (pIusbPkt);
	IUSB_HEADER *pkt = (IUSB_HEADER*) (pIusbPkt);


	strncpy((char*) pkt->Signature, IUSB_SIG, 8);
	pkt->Major = 0x01;
	pkt->Minor = 0;
	pkt->HeaderLen = 32;
	pkt->HeaderCheckSum = 0;
	pkt->DataPktLen = 9; //IUSB_HID_HDR_SIZE(34) - 1 + USB_PKT_KEYBDREP_SIZE(8) - IUSB_HDR_SIZE(32);
	pkt->ServerCaps = 0;
	pkt->DeviceType = 0x31; //mouse
	pkt->Protocol = 0x20;//IUSB_PROTO_MOUSE_DATA
	pkt->Direction = 0x80; //from remote
	pkt->DeviceNo = 0x02;
	pkt->InterfaceNo = 0x01; //mouse interface
	pkt->ClientData = 0;
	pkt->Instance = 0;
	pkt->SeqNo = seqno++;
	pkt->Key = 0;

	pkt->HeaderCheckSum = CalculateModulo100((u8 *) pkt, sizeof(IUSB_HEADER));
	pHIDPacket->DataLen = MAX_MOUSE_REPORT_SIZE;

	//Absolute mouse mode conversion
	short X = (short) ((x * DIRABS_MAX_SCALED_X) / maxx/*width*/+ 0.5);
	short Y = (short) ((y * DIRABS_MAX_SCALED_Y) / maxy/*height*/+ 0.5);

	//if buttonmask is 4 update it with 2(right click)
	mouseRep.Event = (buttonMask == 4) ? 2 : buttonMask;
	mouseRep.X = X;
	mouseRep.Y = Y;
	mouseRep.Scroll = 0;
	memcpy(&pHIDPacket->Data, &mouseRep, MAX_MOUSE_REPORT_SIZE);
#if !defined(SOC_AST2600)
	if ((ioctl_retval = do_usb_ioctl(USB_MOUSE_DATA, (u8*) pIusbPkt)) < 0) {
		printf("\n Executing mouse ioctl cmd failed..\n");
	}
#else
	if((ret = SendMouseData((u8*)&(pHIDPacket->Data))) < 0) {
		printf("\n Executing mouse ioctl cmd failed..%d \n", ret);
	}
#endif
	return ioctl_retval;
}

void handleKbdEvent(int8 down, uint32 key) {

	//If CAPS lock is enabled then ascii value from 65 (A) to 90 (Z) will be received as keycode.
	//In this case, send CAPS lock key event to the host first, and then send the pressed key event.
	//This will trigger capital letter in the host.
	if ( key >= 'A' && key <= 'Z' && down == 1){
		//Shift modifier is not enabled, but still we have received a capital letter.
		if(((prevkey.Modifier & MOD_LEFT_SHIFT) != MOD_LEFT_SHIFT) &&
			((prevkey.Modifier & MOD_RIGHT_SHIFT) != MOD_RIGHT_SHIFT))
		{
			//If CAPS lock is not set, enable CAPS lock in the host first.
			if((g_led_status & CAPS_LOCK) != CAPS_LOCK)
			{
				//Send CAPS lock key code to host
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 1);//press
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 0);//release
			}
		}
		else{//Shift modifier is enabled
			//If CAPS lock is status set, disable caps lock in the host.
			if((g_led_status & CAPS_LOCK) == CAPS_LOCK)
			{
				//Send CAPS lock key code to host
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 1);//press
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 0);//release
			}
		}
	}
	else if( key >= 'a' && key <= 'z' && down == 1){
		// We have received keycodes for small letters, and shift is not enabled.
		if(((prevkey.Modifier & MOD_LEFT_SHIFT) != MOD_LEFT_SHIFT) &&
				((prevkey.Modifier & MOD_RIGHT_SHIFT) != MOD_RIGHT_SHIFT))
		{
			//If CAPS lock status is set, we need to disable caps lock in the host first, so that
			//small letters are triggered in the host.
			if((g_led_status & CAPS_LOCK) == CAPS_LOCK){
				//Send CAPS lock key code to host
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 1);//press
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 0);//release
			}
		}
		else{// Shift modifier is set means CAPS lock is enabled in the client
			//If CAPS lock status is not set, then we need to enable CAPS lock in the host.
			if((g_led_status & CAPS_LOCK) != CAPS_LOCK){
				//Send CAPS lock key code to host
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 1);//press
				SendKeyEvent(CAPS_LOCK_KEY_CODE, 0);//release
			}
		}
	}
	
	//If incoming key code corresponds to the values of number pad 0-9 or number pad decimal, 
	//that means number lock is enabled in the client.
	if((key >= NUMPAD_0 && key <= NUMPAD_9) || key == NUMPAD_DECIMAL){
		//If NUM lock status is not set, then we need to enable NUM lock in the host.
		if((g_led_status & NUM_LOCK) != NUM_LOCK){
			SendKeyEvent(NUM_LOCK_KEY_CODE, 1);//press
			SendKeyEvent(NUM_LOCK_KEY_CODE, 0);//release
		}
	}
	//If incoming key code belongs to the range between the values of number pad home to number pad delete, 
	//that means number lock is disabled in the client.
	else if(key >= NUMPAD_HOME && key <= NUMPAD_DELETE){
		//If NUM lock status is set, then we need to disable NUM lock in the host.
		if((g_led_status & NUM_LOCK) == NUM_LOCK){
			SendKeyEvent(NUM_LOCK_KEY_CODE, 1);//press
			SendKeyEvent(NUM_LOCK_KEY_CODE, 0);//release
		}
	}
	SendKeyEvent(key, down);
	return;
}

/**
 * Send keyboard event to the host.
 * key - RFB keyrep value.
 * down - if 1 key is pressed, if 0 key is released.
 */
void SendKeyEvent(uint32 key, int8 down){
	unsigned char KeybdPacket[sizeof(IUSB_HID_PACKET) + sizeof(USB_KEYBD_PKT)];
	IUSB_HID_PACKET *KeybdHeader = (IUSB_HID_PACKET *) KeybdPacket;
	memset(KeybdHeader, 0, sizeof(IUSB_HID_PACKET));
	USB_KEYBD_PKT *KeybdInfo = (USB_KEYBD_PKT *) (&(KeybdHeader->Data));
#if defined(SOC_AST2600)
	int ret = -1;
#endif
	KeybdHeader->Header.DeviceNo = 2;
	KeybdHeader->Header.Protocol = IUSB_PROTO_KEYBD_DATA;

	if (ConvertRFBtoUSBKey(KeybdInfo, down, key) == 0){
		memcpy(&prevkey, KeybdInfo, sizeof(prevkey));
	}
#if !defined(SOC_AST2600)
	if (do_usb_ioctl(USB_KEYBD_DATA, (u8*) KeybdPacket) < 0) {
		printf("\n Unable to send keyboard events\n");
	}
#else
	if((ret = SendKeybdData(&KeybdInfo[0])) < 0) {
		printf("\n Unable to send keyboard events %d\n", ret);
	}
#endif
	return;
}

int ConvertRFBtoUSBKey(USB_KEYBD_PKT *KeybdInfo, int8 down, uint32 key) {
	unsigned char UsbKey;
	int i, j, Duplicate;

	memset(KeybdInfo, 0, sizeof(USB_KEYBD_PKT));

	/* Check for modifiers */
	switch (key) {
	case RFB_Shift_L:
		KeybdInfo->Modifier = MOD_LEFT_SHIFT;
		break;
	case RFB_Shift_R:
		KeybdInfo->Modifier = MOD_RIGHT_SHIFT;
		break;
	case RFB_Control_L:
		KeybdInfo->Modifier = MOD_LEFT_CTRL;
		break;
	case RFB_Control_R:
		KeybdInfo->Modifier = MOD_RIGHT_CTRL;
		break;
	case RFB_Meta_L:
	case RFB_Alt_L:
		KeybdInfo->Modifier = MOD_LEFT_ALT;
		break;
	case RFB_Meta_R:
	case RFB_Alt_R:
		KeybdInfo->Modifier = MOD_RIGHT_ALT;
		break;
	case RFB_Super_L:
		KeybdInfo->Modifier = MOD_LEFT_WIN;
		break;
	case RFB_Super_R:
		KeybdInfo->Modifier = MOD_RIGHT_WIN;
		break;
	default:
		KeybdInfo->Modifier = 0;
		break;
	}
	if (down) /* Key Down: Add to the existing Modifier */
		KeybdInfo->Modifier |= prevkey.Modifier;
	else
		/* Key Up  : Remove from existing Modifier */
		KeybdInfo->Modifier = prevkey.Modifier & (~KeybdInfo->Modifier);

	/* Modifiers are always reported even if phantom state is reached */
	prevkey.Modifier = KeybdInfo->Modifier;

	/* Some Special Keys of RFB */
	if ((key & 0xFF00) == 0xFF00) {
		UsbKey = RFB2USB_Map[key & 0xFF];
	} else {
		UsbKey = Ascii2USB_Map[key];
	}

	/* If Unknown key, ignore it, (i.e) Send the previous status again */
	if (UsbKey == 0) {
		memcpy(KeybdInfo, &prevkey, sizeof(prevkey));
		return 0; /* Success ?. Does not matter even if send failure */
	}

	/* Check if the key to be released */
	if (!down) {
		for (i = 0, j = 0; i < 6; i++) {
			if (prevkey.KeyCode[i] == UsbKey)
				continue;
			KeybdInfo->KeyCode[j++] = prevkey.KeyCode[i];
		}
		return 0; /* Success */
	}

	/* Comes here if a new key is pressed */
	Duplicate = 0;
	for (i = 0; i < 6; i++) {
		if (prevkey.KeyCode[i] == UsbKey)
			Duplicate = 1;
		if (prevkey.KeyCode[i] == 0)
			break;
		KeybdInfo->KeyCode[i] = prevkey.KeyCode[i];
	}
	if (i != 6) {
		if (!Duplicate) /* Check if key is kept pressed. If so don't add entry */
			KeybdInfo->KeyCode[i] = UsbKey;
		return 0; /* Success */
	}

	/* if i == 6 , we reached phantom state (roll over) */
	for (i = 0; i < 6; i++)
		KeybdInfo->KeyCode[i] = 1;
	return 1; /* Rollover Error, so we will mantain the prev state */
}

