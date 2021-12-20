
#ifndef VNC_H
#define VNC_H
#include <stdlib.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/uinput.h>

/* ----- Define ----- */
#define BPP 4
#define MAX_CURSOR_AREA 64
#define MIN_CURSOR_AREA 32
#define CURSOR_PKT_MIN_LENGTH 13
#define CURSOR_PKT_MAX_LENGTH (CURSOR_PKT_MIN_LENGTH + 8192)
#define TEXT_MODE       1 << 1
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define VNC_MIN_RESX 640
#define VNC_MIN_RESY 400
#define RESOLUTION_CHANGED 100
#define VIDEO_SUCCESS	 0
#define VIDEO_FULL_SCREEN_CHANGE 3

#define CAPTURE_VIDEO_ERROR	-1
#define CAPTURE_VIDEO_SUCCESS	 0
#define CAPTURE_VIDEO_NO_CHANGE  1
#define CAPTURE_VIDEO_NO_SIGNAL  2
#define CAPTURE_VIDEO_CTRL_PACKET	4

#define FALSE 0

#define BLOCK_SIZE_YUV444 (0x08)
#define BLOCK_SIZE_YUV420 (0x10)

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
	void *frame_addr;
} __attribute__ ((packed)) vnc_frame_hdr_t;

vnc_frame_hdr_t  frame_hdr_vnc;


typedef struct ast_videocap_jpeg_tile_info_t
{
	uint8_t pos_x;
	uint8_t pos_y;
	uint32_t width;
	uint32_t height;
	uint32_t compressed_size;
#ifdef SOC_AST2600
	uint32_t resolution_x;
	uint32_t resolution_y;
	uint8_t bpp;
#endif
} __attribute__((packed)) tileinfo_t;

extern tileinfo_t g_tile_info;


/* ----- Define function ----- */

void set_send_cursor_pattern_flag(int flag); //dummy function


/* ----- Define variable ----- */
int isTextMode;
extern int maxx,maxxy,pre_maxx,pre_maxy;
extern unsigned char *tile_info;
extern int isNewSession;
extern int actvSessCount;
void getResolution(int *maxx_vnc,int *maxy_vnc);
void setResolution(int maxx_vnc, int maxy_vnc);
#endif //VNC_H
