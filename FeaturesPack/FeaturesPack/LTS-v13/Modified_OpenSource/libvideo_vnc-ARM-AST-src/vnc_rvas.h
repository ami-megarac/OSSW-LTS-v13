

#ifndef VNC_H
#define VNC_H
#include "videopkt_vnc_soc.h"

#define TRUE 1
#define FALSE 0

/* ----- Define ----- */
#define CURSOR_PKT_MIN_LENGTH 13
#define CURSOR_PKT_MAX_LENGTH (CURSOR_PKT_MIN_LENGTH + 8192)
#define TILE_WIDTH 32
#define TILE_HEIGHT 32
#define NO_SCREEN_CHANGE_SIZE      4

#define CAPTURE_VIDEO_ERROR	-1
#define CAPTURE_VIDEO_SUCCESS	 0
#define CAPTURE_VIDEO_NO_CHANGE  1
#define CAPTURE_VIDEO_NO_SIGNAL  2
//#define CAPTURE_VIDEO_CTRL_PACKET	4

#define VIDEO_ERROR -1
#define VIDEO_SUCCESS	 0
#define VIDEO_NO_CHANGE  1
#define VIDEO_NO_SIGNAL  2
#define VIDEO_FULL_SCREEN_CHANGE 3
#define RESOLUTION_CHANGED 100

// minimum video resolution (on text mode - 640x400)
#define VNC_MIN_RESX 640
#define VNC_MIN_RESY 400

#ifndef INIT_QLZW
#define INIT_QLZW    0x0004
#endif
//:int g_swap = TRUE;

#ifndef __KERNEL__
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#endif

extern int tilecount;
extern int isTextMode;
extern int TextFlag;

/* ----- Define Struct ----- */

typedef struct
{
	u8      BytesPerPixel;
	u8      video_mode_flags;
	u8      char_height;
} __attribute__ ((packed)) capture_param_vnc_t;

typedef struct
{
	u16             res_x;                  /**<  resolution - width */
	u16             res_y;                  /**<   resolution - height*/
	capture_param_vnc_t  params;            /**< Capture Parameters  */
	u8							Reserved[32];
} __attribute__ ((packed)) vnc_frame_hdr_t;

vnc_frame_hdr_t  frame_hdr_vnc;



extern int maxx, maxy; //default video resolution
extern int pre_maxx, pre_maxy; // holds previous video resolution when showing empty screen

extern int g_fullscreen;
extern int isNewSession;
extern int actvSessCount;
extern char *tile_info;
extern int maxx,maxxy,pre_maxx,pre_maxy;
void getResolution(int *maxx_vnc,int *maxy_vnc);
void setResolution(int maxx_vnc, int maxy_vnc);
void release_video_resources(void);
#endif //VNC_H
