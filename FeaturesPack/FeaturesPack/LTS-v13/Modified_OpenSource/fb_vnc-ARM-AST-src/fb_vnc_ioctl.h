#ifndef __FB_VNC_IOCTL_H__
#define __FB_VNC_IOCTL_H__

#define AMIFB_GET_DFBINFO	_IO(0xF3, 0x00)

#define AMIFB_SET_DISPSRC	_IO(0xF3, 0x01)
#define AMIFB_GET_DISPSRC	_IO(0xF3, 0x02)
#define DISPSRC_FB			0x1
#define DISPSRC_VGA			0x0
#define DISPSRC_FBUNKNOW	0xFF

#endif /* !__FB_VNC_IOCTL_H__ */
