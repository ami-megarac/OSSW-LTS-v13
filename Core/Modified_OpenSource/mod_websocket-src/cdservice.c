#include <mod_websocket.h>

IUSB_REQ_REL_DEVICE_INFO iUSBDeviceInfo[MAX_CD_INSTANCES];

/*
 * addAuthInfo: adds auth info for the given instance
 * cmd: usb cdrom command
 * data: iusb header/data
 * instance : cd usb instance
 */
u8*
addAuthInfo(int cmd, u8 *data, uint8 Instance) {
	if (Instance >= MAX_CD_INSTANCES) {
		TCRIT("Current CD instance exceeded the maximum allowed CD instances");
		return NULL;
	}

	switch (cmd) {
	case USB_CDROM_RES:
	case USB_CDROM_REQ:
		SetAuthInfo(iUSBDeviceInfo[Instance], data, 1);
		((IUSB_HEADER*) data)->Instance = Instance;
		return data;
	case USB_CDROM_EXIT:
	case USB_CDROM_ACTIVATE:
	case USB_DEVICE_DISCONNECT:
	case USB_DEVICE_RECONNECT:
		SetAuthInfo(iUSBDeviceInfo[Instance], (u8*) &ioctlData[Instance], 0);
		ioctlData[Instance].DevInfo.Instance = Instance;
		return (u8*) &ioctlData[Instance];
	default:
		TWARN("Error: AddAuthInfo() received unsupported IOCTL\n");
	}
	return NULL;
}

/*
 * releaseUsbDev: releases cd usb device for the given instance
 * 
 */
void releaseUsbDev(int instance) {
	uint8 InstanceIx = instance;
	if (&iUSBDeviceInfo[InstanceIx]) {
		ReleaseiUSBDevice(&iUSBDeviceInfo[InstanceIx]);
	}
}

/*
 * requestUsbDev: request cd usb device for the given instance
 * 
 */
int requestUsbDev(int instance) {
	uint8 InstanceIx = instance;

	iUSBDeviceInfo[InstanceIx].DevInfo.DeviceType = IUSB_CDROM_FLOPPY_COMBO;
	iUSBDeviceInfo[InstanceIx].DevInfo.LockType = LOCK_TYPE_SHARED;
	if (0 != RequestiUSBDevice(&iUSBDeviceInfo[InstanceIx])) {
		iUSBDeviceInfo[InstanceIx].DevInfo.DeviceType = IUSB_DEVICE_CDROM;
		iUSBDeviceInfo[InstanceIx].DevInfo.LockType = LOCK_TYPE_SHARED;
		if (0 != RequestiUSBDevice(&iUSBDeviceInfo[InstanceIx])) {
			TCRIT("Error Unable to find iUSB CDROM Device \n");
			return 1;
		}
	}

	return 0;
}

/*
 * processActivate: processes activate for the given cd usb instance
 * 
 */
int processActivate(uint8 Instance) {
	u8* ModifiedData;
	ModifiedData = (u8*) addAuthInfo(USB_CDROM_ACTIVATE, NULL, Instance);
	if (ioctl(mod_usbfd, USB_CDROM_ACTIVATE, ModifiedData)) {
		TWARN("ProcessActivate(): USB_CDROM_ACTIVATE ioctl failed for Instance %d\n", Instance);
	}
	return 0;
}

/*
 * processDisconnect: processes disconnect for the given cd usb instance
 * 
 */
int processDisconnect(uint8 Instance) {
	u8* ModifiedData;
	/*
	 * Issue Terminate Ioctl to driver, which releases the wait ioctl,
	 * which returns to the child thread  with error and the child
	 * thread will terminate itself
	 */
	TDBG(" Issuing Terminate Ioctl to the Driver\n");

	ModifiedData = (u8*) addAuthInfo(USB_CDROM_EXIT, NULL, Instance);
	if (ioctl(mod_usbfd, USB_CDROM_EXIT, ModifiedData)) {
		return -1;
	}
	return 0;
}

/*
 * sendDataToDriver: sends given *to_driver_date to usb driver
 * Instance : cd usb instance
 * to_driver_data : scsi packet to be sent
 */
int sendDataToDriver(uint8 Instance, char *to_driver_data) {
	u8* ModifiedData;
	IUSB_SCSI_PACKET *resPacket = NULL;
	int ret = -1;

	resPacket = (IUSB_SCSI_PACKET *) to_driver_data;
	ModifiedData = (u8*) addAuthInfo(USB_CDROM_RES, (u8*) resPacket, Instance);
	ret = ioctl(mod_usbfd, USB_CDROM_RES, ModifiedData);
	resPacket = NULL;
	return ret;
}

/*
 * sendCmdToCdserver : sends given SCSI cmd to cdserver
 * server : cdserver socket
 * cmd : scsi opcode
 * instance: cd instance number
 * 
 */
int sendCmdToCdserver(int server, int cmd, int instance) {

	int len;
	IUSB_SCSI_PACKET RemoteHBPkt;
	memset(&RemoteHBPkt, 0, sizeof(IUSB_SCSI_PACKET));

	RemoteHBPkt.Header.Instance = instance;
	memcpy(RemoteHBPkt.Header.Signature, IUSB_SIG, strlen(IUSB_SIG));
	RemoteHBPkt.CommandPkt.OpCode = cmd; // OpCode 
	RemoteHBPkt.Header.DataPktLen = mac2long(sizeof(IUSB_SCSI_PACKET) - sizeof(IUSB_HEADER));
	len = mac2long(RemoteHBPkt.Header.DataPktLen) + sizeof(IUSB_HEADER);
	sendDataToServer(server, (char *) &RemoteHBPkt, len);

	return 0;
}

