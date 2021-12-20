#ifndef __PILOT_FB_VNC_MODE_H__
#define __PILOT_FB_VNC_MODE_H__


struct pilot_fb_mode_info_t {
	unsigned long horiz_total;
	unsigned long horiz_display_end;
	unsigned long horiz_front_porch;
	unsigned long horiz_sync;

	unsigned long verti_total;
	unsigned long verti_display_end;
	unsigned long verti_front_porch;
	unsigned long verti_sync;

	int refresh_rate;
	int clk_tab_index;

	unsigned int flags;
};


struct pilotfb_dfbinfo {
	unsigned long ulFBSize;
	unsigned long ulFBPhys;

	unsigned long ulCMDQSize;
	unsigned long ulCMDQOffset;

	unsigned long ul2DMode;
};

struct pilotfb_dispsrc {
	u32 val;
};

#endif /* !__PILOT_FB_VNC_MODE_H__ */


