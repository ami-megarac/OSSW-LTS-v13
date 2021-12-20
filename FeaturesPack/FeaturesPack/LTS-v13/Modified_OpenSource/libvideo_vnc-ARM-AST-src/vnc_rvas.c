
/* Include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
// #include "videoctl.h"
#include "vnc_rvas.h"
#include "videopkt_vnc_soc.h"
#include "fge.h"
//
static void soc_setflags_before_videocapture (u32* flags );
static void soc_setflags_after_videocapture (u32* flags, int cap_status);
static void CloseFpgaDriver(int fd);

static char*	mmap_address = NULL;
static char*	palt_address = NULL;

int isTextMode = 0;
int TextFlag = 0;
char *tile_info;
int gVideoDevice;
int tilecount = 0;
u8 BytesPP = 0;
u8 bSendCursor = 0;
u8 runxcursorthread = 0;
u8 send_xcursor_packet = 0;
frame_hdr_t  g_frame_hdr;
u32	g_cap_flags;
int g_fullscreen = 0;
int isNewSession = 0;
u32	bChangePalette = 0;
int maxx = VNC_MIN_RESX, maxy = VNC_MIN_RESY; //default video resolution
int pre_maxx = VNC_MIN_RESX, pre_maxy = VNC_MIN_RESY;
unsigned long video_buffer_offset = 0;
int g_swap = TRUE;
int actvSessCount = 0;



static int OpenFpgaDriver(void)
{
    int fd;

    //TDBG("OpenFpgaDriver() : Called\n");

    fd = open("/dev/videocap", O_RDWR|O_SYNC);
    if (fd < 0)
    {
        //printf("OpenVideo cannot open VIDEO device\n");
        return -1;
    }

    mmap_address = mmap(0, SCREEN_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_address == MAP_FAILED)
    {
        //printf("Unable to map screen buffer to user space\n");
        close(fd);
        return -1;
    }
    //TDBG("mmap_address is 0x%lx\n",(unsigned long)mmap_address);

    /**
     * Previously both Palette and Video data were mmap'ed to the same memory region
     * Since both palette and video are interrupt based, palette interrupt may come at any time
     * This is sometimes causing a junk to be displayed in the redirection.
     * Hence, using a seperate allocated user-space area and sending this to the driver
     * We copy the palette data from driver to user end
    **/
    palt_address = malloc(PALETTE_BUFFER_SIZE);
    if (palt_address == NULL)
    {
	//printf("Unable to allocate 1kB for Storing the Palette data\n");
	close(fd);
	return -1;
    }

    return fd;
}


int init_video_resources(void)
{
	gVideoDevice = OpenFpgaDriver();
	if (gVideoDevice < 0)
		return -1;
	return 0;
}



int DoVideo_ioctl(int fd, u32 flags, frame_hdr_t *frame_hdr)
{
    int capture_status;
    VIDEO_Ioctl_AVICAA  mioc = {0};
    
    mioc.OpCode = VIDEO_IOCTL_CAPTURE_VIDEO;
    mioc.Capture_Param.Cap_Flags = flags;
    mioc.Get_Screen.TileColumnSize = TILE_COLUMN_SIZE;
    mioc.Get_Screen.TileRowSize = TILE_ROW_SIZE;

    //ioctl to cature video
    capture_status = ioctl (fd, VIDEO_IOCCMD, &mioc);
    
    frame_hdr->params.BytesPerPixel = mioc.Get_Screen.BytesPerPixel;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)
    frame_hdr->params.Actual_Bpp = mioc.Get_Screen.Actual_Bpp;
#endif
    frame_hdr->params.video_mode_flags = mioc.Get_Screen.video_mode_flags ;
    frame_hdr->params.char_height = mioc.Get_Screen.char_height ;
    frame_hdr->params.left_x     = mioc.Get_Screen.left_x ;
    frame_hdr->params.right_x    = mioc.Get_Screen.right_x ;
    frame_hdr->params.top_y  = mioc.Get_Screen.top_y ;
    frame_hdr->params.bottom_y   = mioc.Get_Screen.bottom_y ;
    frame_hdr->params.text_snoop_status = mioc.Get_Screen.text_snoop_status;
    frame_hdr->Width         = mioc.Get_Screen.HDispEnd;
    frame_hdr->Height        = mioc.Get_Screen.VDispEnd;
    frame_hdr->tile_cap_mode     = mioc.Get_Screen.tile_cap_mode;
    frame_hdr->res_x         = mioc.Get_Screen.FrameWidth;
    frame_hdr->res_y         = mioc.Get_Screen.FrameHeight;
    frame_hdr->size      = mioc.Get_Screen.FrameSize;
    frame_hdr->TextBuffOffset    = mioc.Get_Screen.TextBuffOffset;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)
    frame_hdr->bandwidth_mode 	= mioc.LowBandwidthMode;
#endif
 
    BytesPP = frame_hdr->params.BytesPerPixel;
 
    if (capture_status != 0) {
        //ioctl failed
        capture_status = VIDEO_IOCTL_INTERNAL_ERROR;
        return capture_status;
    }
    capture_status = mioc.ErrCode;
    return capture_status;
}


int capture_video( void **frame_hdr,void **video_data){
	VIDEO_Ioctl_AVICAA 	mioc={0};
	int 			capture_status;
	u16* 			p_primary_buf;
	//u16*                    p_comp_buf_dest;

	*frame_hdr = (void*) &g_frame_hdr;	
	soc_setflags_before_videocapture( &g_cap_flags );

	capture_status = DoVideo_ioctl(gVideoDevice, g_cap_flags, &g_frame_hdr);

	if (capture_status == VIDEO_IOCTL_INTERNAL_ERROR) {
		return capture_status;
	}

	if (capture_status != VIDEO_IOCTL_SUCCESS)
	{
//		*data_buf_size = g_frame_hdr.size;
		soc_setflags_after_videocapture( &g_cap_flags, capture_status );
                g_swap           = FALSE;
	}
	else
	{
		p_primary_buf = (u16*) (mmap_address);
//		sw_compress_videodata( &p_primary_buf, &p_comp_buf_dest , &g_frame_hdr);
		g_frame_hdr.compression_type = SOFT_COMPRESSION_NONE;
		soc_setflags_after_videocapture( &g_cap_flags, capture_status );
		g_frame_hdr.flags = g_cap_flags;
		*video_data = (void*) p_primary_buf;
		//*data_buf_size = g_frame_hdr.size;
//		*frame_hdr_size = sizeof(g_frame_hdr);
	}

	if (g_frame_hdr.size  == NO_SCREEN_CHANGE_SIZE)
		sleep(0.1);

	switch (capture_status)
	{
		case VIDEO_IOCTL_NO_INPUT:
		case VIDEO_IOCTL_HOST_POWERED_OFF:
			return CAPTURE_VIDEO_NO_SIGNAL;	
		case VIDEO_IOCTL_NO_VIDEO_CHANGE:
			return CAPTURE_VIDEO_NO_CHANGE;
		case VIDEO_IOCTL_SUCCESS:
			return CAPTURE_VIDEO_SUCCESS;
		case VIDEO_IOCTL_BLANK_SCREEN:
			if (mioc.Get_Screen.ModeChange) {
				g_fullscreen = 1;
			}
			return CAPTURE_VIDEO_NO_SIGNAL;
		default:
			return CAPTURE_VIDEO_ERROR;
	}
	return CAPTURE_VIDEO_ERROR;
}


/*********************************
 *    Local Functions		 * 
 *********************************/

static void soc_setflags_before_videocapture (u32* flags)
{
	*flags = CAPTURE_EVEN_FRAME;

	if ( isNewSession )
	{
		*flags |= CAPTURE_NEW_SESSION;
		*flags |= CAPTURE_CLEAR_BUFFER;
//		bSendPalette = 1;
		bSendCursor = 1;
		send_xcursor_packet = TRUE;
	}
	else 
	{
		*flags &= ~(CAPTURE_NEW_SESSION);
		*flags &= ~(CAPTURE_CLEAR_BUFFER);
	
		if ( g_swap )
		{
			if (0 != (*flags & CAPTURE_ODD_FRAME))
				*flags       |= CAPTURE_EVEN_FRAME;
			else
				*flags       |= CAPTURE_ODD_FRAME;
		}
	}
	
	if (bChangePalette == 1)
	{
		bChangePalette = 0;
//		bSendPalette = 1;
		*flags |= CAPTURE_PALETTE_UPDATED;
		*flags |= CAPTURE_CLEAR_BUFFER;
	}

	if ( isNewSession )
	{
		*flags       |= INIT_QLZW;
		g_swap            = FALSE;
	}

	if (actvSessCount <= 1) {
		g_swap = TRUE;
	}
}

static void soc_setflags_after_videocapture (u32* flags, int cap_status)
{
	if ((cap_status != VIDEO_IOCTL_SUCCESS) && (cap_status != VIDEO_IOCTL_NO_VIDEO_CHANGE))
	{
		*flags = CAPTURE_EVEN_FRAME + CAPTURE_CLEAR_BUFFER;
	}
	else
	{
                *flags |= INIT_QLZW;
	}

	*flags &= ~(CAPTURE_NEW_SESSION);
}

/** Get Video Offset value from video driver
 ** return 
 **	successs - video buffer offset value
 ** failure  - zero
**/
unsigned long getVideoOffsetFromDriver(void)
{
	VIDEO_Ioctl_addr mioc= {0};
    mioc.OpCode = VIDEO_IOCTL_GET_VIDEO_OFFSET;

    //ioctl to fetch video offset
    if ( ioctl (gVideoDevice, VIDEO_START_CMD, &mioc) !=0 )
    {
		printf("Error: unable to get video address offest \n");
		return 0;
	}
	else
	{
		return mioc.video_buffer_offset;
	}
}


int startCapture( ) {
	int capture_status = 0;
//	u32 data_buf_size=0, frame_hdr_size=0;
	void* p_data_buf = NULL;

	capture_status = capture_video ((void*)&g_frame_hdr, &p_data_buf);

	if (capture_status == VIDEO_IOCTL_INTERNAL_ERROR)
		return capture_status;

	if (capture_status == VIDEO_IOCTL_SUCCESS)
	{
		//Update resolution information
		frame_hdr_vnc.res_x = g_frame_hdr.res_x;
		frame_hdr_vnc.res_y = g_frame_hdr.res_y;
		// update char height information
		frame_hdr_vnc.params.char_height = g_frame_hdr.params.char_height;
		//Assign tile map address
		tile_info = (char*) mmap_address;//et_mmap_address();
		// update video mode flag information
		frame_hdr_vnc.params.video_mode_flags = g_frame_hdr.params.video_mode_flags;
		if ( isNewSession )
			isNewSession = FALSE;
	}
	
	if(frame_hdr_vnc.params.video_mode_flags & TEXT_MODE)
	{
		isTextMode = 1;
		TextFlag = g_frame_hdr.params.text_snoop_status;
	}
	else
	{
		isTextMode = 0;
	}

	//mark resolution change only if incoming resolution is valid
	if((frame_hdr_vnc.res_x >= VNC_MIN_RESX) && (frame_hdr_vnc.res_y >= VNC_MIN_RESY)  && (capture_status != VIDEO_NO_SIGNAL))
	{
		if((frame_hdr_vnc.res_x != maxx) || (frame_hdr_vnc.res_y != maxy))
		{
			maxx = frame_hdr_vnc.res_x;
			maxy = frame_hdr_vnc.res_y;
			video_buffer_offset = getVideoOffsetFromDriver();
			if( video_buffer_offset < 0 )
			{
				video_buffer_offset = 0; //video buffer offset can't be negative. so reset the offset
			}
			//bpp = ; update bpp here
			return RESOLUTION_CHANGED;
		}
	}

	if ((capture_status != VIDEO_SUCCESS) || (tile_info == NULL) || isTextMode)
	{/*when no screen change it will happen*/
		return capture_status;
	}

	tilecount = *((unsigned short*)tile_info);
	// For resolution 800x600, the total no of tiles will be ((800x600x4)/(16x16)) = 7500
	// 800x600 resolution, 16x16 tile width, tile height.
	// send full screen change if the tile count is more than 50% of total tile count
	if(tilecount >= (((frame_hdr_vnc.res_x * frame_hdr_vnc.res_y * 4) / (TILE_HEIGHT*TILE_WIDTH)) * 0.5))
	{
		capture_status = VIDEO_FULL_SCREEN_CHANGE;
		return capture_status;
	}
		
	return capture_status;
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

void getVideoBufferOffset(unsigned long int *video_buffer_offset_vnc){
	*video_buffer_offset_vnc = video_buffer_offset;
}

void release_video_resources(void)
{
	CloseFpgaDriver(gVideoDevice);
}

static void CloseFpgaDriver(int fd)
{
    if (fd > 0) close(fd);

    if (mmap_address)
    {
        if (munmap(mmap_address,SCREEN_BUFFER_SIZE) != 0) {
                //TCRIT("Unable to unmap screen buffer from user space\n");
        }
        mmap_address = NULL;
    }

    if (palt_address != NULL)
    {
	free(palt_address);
	palt_address = NULL;
    }

    return;
}




