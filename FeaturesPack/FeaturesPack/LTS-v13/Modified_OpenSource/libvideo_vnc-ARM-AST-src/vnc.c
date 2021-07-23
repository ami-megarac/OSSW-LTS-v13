
/* Include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>


#include "videopkt_vnc_soc.h"
#include "decode.h"
#include "vnc.h"

tileinfo_t g_tile_info;
frame_hdr_t g_frame_hdr;

int init_video_resource_vnc();
void release_video_resource_vnc();

int vnc_capture_video (unsigned char** tile_info, void* frame_hdr);
int isNewSession = 0;
static int videocap_fd = 0;
static void *videocap_buf = NULL;
void* p_frame_hdr = NULL;
void* p_data_buf = NULL;
unsigned char *tile_info = NULL;
int maxx = VNC_MIN_RESX, maxy = VNC_MIN_RESY; //default video resolution
int pre_maxx = VNC_MIN_RESX, pre_maxy = VNC_MIN_RESY; // holds previous video resolution when showing empty screen
int actvSessCount = 0; //dummy

int init_video_resources(void)
{
	videocap_fd = open("/dev/videocap", O_RDONLY | O_SYNC);
	if (videocap_fd < 0) {
		printf("open videocap failed\n");
		return -1;
	}

	videocap_buf = mmap(0, VIDEO_BUF_SZ, PROT_READ, MAP_SHARED, videocap_fd, 0);
	if (videocap_buf == MAP_FAILED) {
		printf("map videocap buffer failed\n");
		close(videocap_fd);
		return -1;
	}

	return 0;
}
int captureVideo( void **frame_hdr,void **video_data) {
	ASTCap_Ioctl ioc;
	int ret;

	if ( isNewSession ) {
		ioc.OpCode = ASTCAP_IOCTL_START_CAPTURE;

		if (ioctl(videocap_fd, ASTCAP_IOCCMD, &ioc) < 0) {
			printf("IOCTL call failed videocap_fd\n");
			return -1;
		}

		if (ioc.ErrCode != (signed int)ASTCAP_IOCTL_SUCCESS) {
			return ioc.ErrCode;
		}
		isNewSession = FALSE;
	}

	ioc.OpCode = ASTCAP_IOCTL_GET_VIDEO;
	ioc.vPtr = (void *)&g_tile_info;
	ret = ioctl(videocap_fd, ASTCAP_IOCCMD, &ioc);
	if (ret != 0) {
		return CAPTURE_VIDEO_ERROR;
	}

	if (ioc.ErrCode == (signed int)ASTCAP_IOCTL_BLANK_SCREEN) {
		return CAPTURE_VIDEO_NO_SIGNAL;
	}

	if (ioc.ErrCode == (signed int)ASTCAP_IOCTL_NO_VIDEO_CHANGE) {
		return CAPTURE_VIDEO_NO_CHANGE;
	}

	*frame_hdr = videocap_buf;
	// *hdr_size = sizeof(frame_hdr_t);
	*video_data = videocap_buf + VIDEO_HDR_BUF_SZ;
	
	// *data_size = ioc.Size;

	return CAPTURE_VIDEO_SUCCESS;
}

int startCapture()
{
	int cap_status = CAPTURE_VIDEO_ERROR;

	memset(&g_tile_info, 0, sizeof(g_tile_info));
	cap_status = captureVideo(&p_frame_hdr, &p_data_buf);

	if (cap_status == CAPTURE_VIDEO_SUCCESS)
	{
		//res_x, res_y not used in vncserver. Updating res_x, res_y just to check resolution change.
		frame_hdr_vnc.res_x = g_tile_info.resolution_x;
		frame_hdr_vnc.res_y = g_tile_info.resolution_y;
		frame_hdr_vnc.frame_addr = p_data_buf;
		frame_hdr_vnc.params.BytesPerPixel = g_tile_info.bpp;

#if !defined(SOC_AST2600)
		frame_hdr_vnc.params.video_mode_flags = 0; 
		// decode_tile_coordinates(&tile_info, p_frame_hdr, p_data_buf);

		if (frame_hdr_vnc.params.video_mode_flags & TEXT_MODE)
		{
			isTextMode = 1;
		}
		else
		{
			isTextMode = 0;
		}
#endif
		//mark resolution change only if incoming resolution is valid
		if ((frame_hdr_vnc.res_x >= VNC_MIN_RESX) && (frame_hdr_vnc.res_y >= VNC_MIN_RESY) && ( cap_status != CAPTURE_VIDEO_NO_SIGNAL))
		{
			if ((frame_hdr_vnc.res_x != maxx) || (frame_hdr_vnc.res_y != maxy))
			{
				maxx = frame_hdr_vnc.res_x;
				maxy = frame_hdr_vnc.res_y;
				//bpp = ; update bpp here
				return RESOLUTION_CHANGED;
			}
		}
	}

	return cap_status;
}
void release_video_resources(void)
{
	if (videocap_fd > 0) {
		close(videocap_fd);
	}

	if (videocap_buf != NULL) {
		if (munmap(videocap_buf, VIDEO_BUF_SZ) != 0) {
			printf("unmap video buffer failed\n");
		}
	}
}
void getResolution(int *maxx_vnc,int *maxy_vnc)
{
	*maxx_vnc = maxx;
	*maxy_vnc = maxy;
}

void setResolution(int maxx_vnc, int maxy_vnc)
{
	maxx = maxx_vnc;
	maxy = maxy_vnc;
}



