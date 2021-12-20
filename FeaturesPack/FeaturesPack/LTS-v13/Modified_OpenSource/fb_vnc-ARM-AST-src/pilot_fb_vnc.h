#ifndef __PILOT_FB_VNC_H__
#define __PILOT_FB_VNC_H__

#define GET_DINFO(info)		(struct pilot_info *)(info->par)
#define GET_DISP(info, con)	((con) < 0) ? (info)->disp : &fb_display[con]

#define PILOT_FRAME_BUF_SZ					(9216 * 1024)//9MB frame size//0x00800000 /* 8 MB */
#define PILOT_GRAPHICS_CMD_Q_SZ				0x00100000 /* 1 MB */

#define PILOT_GRAPHICS_REG_BASE 0x80000000  // DDR begin
#define PILOT_GRAPHICS_REG_SZ		0x01600000// 16Mb

struct pilot_info {
    /* fb info */
    struct fb_info *info;
    struct device *dev;

    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    u32 pseudo_palette[17];

    /* driver registered */
    int registered;

    /* chip info */
    char name[16];

    /* resource stuff */
    unsigned long frame_buf_phys;
    unsigned long frame_buf_sz;
    void *frame_buf;

    unsigned long ulMMIOPhys;
    unsigned long ulMMIOSize;

    void __iomem *io;
    void __iomem *io_2d;

    /* Options */

    /* command queue */
    unsigned long cmd_q_sz;
    unsigned long cmd_q_offset;
    int use_2d_engine;

    /* mode stuff */
    int xres;
    int yres;
    int xres_virtual;
    int yres_virtual;
    int bpp;
    int pixclock;
    int pitch;
    int refreshrate;
};

#endif /* !__PILOT_FB_VNC_H__ */

