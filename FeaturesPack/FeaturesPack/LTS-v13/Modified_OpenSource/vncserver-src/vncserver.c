#include <rfb/rfb.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <rfb/keysym.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/sysmacros.h> /* For makedev() */
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <libvncserver/private.h>
#include <semaphore.h>
#include <sys/sysinfo.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>

#include "examples/radon.h"
#include "vncserver.h"
//#include "videopkt_vnc_soc.h"
#ifdef CONFIG_SPX_FEATURE_VIDEO_RVAS
#include "vnc_rvas.h"
#else
#include "vnc_ast.h"
#endif
#if defined(SOC_AST2600)
#include "libusbgadget_vnc.h"
#endif
#ifdef TRUE
#undef TRUE
#define TRUE 1
#endif

#define FRAME_RECT_FORM_TILES 
#define VNC_SERVER "vncserver"

int DisableAutoSwitch =0;
int IsProcRunning = 1;
int powerCons = 0;
int IsPowerconsumptionmode = 0;
sem_t	mSessionTimer;
int emptyScreenSize;
pthread_t timerThread;
char titleString[TITLE_STRING_LEN]={0};
char titleString_viewOnly[TITLE_STRING_LEN+VIEW_ONLY_STRING_LEN] ={0};
int isResolutionChanged = 0;
int maxRectColumn = 0;
vnc_conf_t vncConf;
int g_master_present = 0;
pthread_t cmdThread = NULL;
int session_management = -1;
sem_t getResponse;
sem_t waitForAMIVncResponse;
extern int maxx, maxy, pre_maxx, pre_maxy;// holds previous video resolution when showing empty screen
extern int gUsbDevice;
int isUSBRunning = 0;
extern int actvSessCount;



#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
int bsodCapture = 0;
#else
unsigned long video_buffer_offset; 
#endif

/* Contains last time (in seconds) since mouse queue became full */
static long mouse_queue_full = 0;
// In some cases of text mode, the value of rect size is greater than maxRectColumn.
// Added a bigger size 72 (multiples of 8) to hold the rectangle and prevent out bound access.
#define RECT_UPDATE_SIZE 72
extern rfbClientIteratorPtr rfbGetClientIteratorWithClosed(rfbScreenInfoPtr rfbScreen);
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
void saveBsodCapturedata(void* p_data_buf);
#endif
int fb_setup_status = -1;
#if !defined(SOC_AST2600)
int init_usb = -1;
int save_usb = -1;
#endif
int frameRectFromTiles(rectUpdate_t* recUpdate);
//cOMMON CODE ( ie., code to be executed with/without AMI_VNC_EXT)

static void clientgone(rfbClientPtr cl) {
	SOCKET* active_sock_list = NULL;
	if ( cl->clientData != NULL ) {
		//Check for master session exit
		if(cl->viewOnly == FALSE){
			g_master_present = MASTER_GONE; //confirm master exit so mark as gone.
		}
		actvSessCount--;
		setActiveSessionCount(actvSessCount);//updating activesession count in extension pakages
	}
	 if(active_sock_list == NULL){
	 	active_sock_list = (SOCKET *)malloc((sizeof(SOCKET) * actvSessCount));
	 }
	//get the list of active sessions
	 getActiveSocketList(active_sock_list);
	
	//When a client is closed, use the list of active sockets to identify which session needs to be cleared.
	 onClientGone(active_sock_list); // Client gone handler for extension package.
	 if(active_sock_list != NULL){
	 	free(active_sock_list);
	 	active_sock_list = NULL;
	 }

	if(actvSessCount == 0)
	{
		memset((char *)sessionInfo, 0, (sizeof(VNCSessionInfo) * vnc_max_session));
		//Reset to default setting when all of the VNC session gone.
		maxx = VNC_MIN_RESX;
		maxy = VNC_MIN_RESY;
		if(rfbScreen->frameBuffer != emptyScreen){
			rfbScreen->frameBuffer = emptyScreen;
		}
		adoptResolutionChange( cl);
		//update resolution in library file. This will avoid vnc 
		setResolution(maxx,maxy);
	}
	free(cl->clientData);
	cl->clientData = NULL;
}


rfbBool
vncrfbProcessEvents(rfbScreenInfoPtr screen,long usec)
{
	if(usec<0)
		usec = screen->deferUpdateTime*1000;

	rfbCheckFds(screen,usec);
	rfbHttpCheckFds(screen); 

	return TRUE;
}

void
vnc_session_timercall()
{
	double timedout = 0;
	time_t update_time;
	struct sysinfo sys_info;
	rfbClientIteratorPtr i;
	rfbClientPtr cl;

	i = rfbGetClientIteratorWithClosed(rfbScreen);
	cl = rfbClientIteratorHead(i);
	while(cl)
	{
		timedout = 1;
		update_time = 0;
		if(cl->sock != -1){
			update_time = getLastPacketTime(cl->sock);

			if( !sysinfo(&sys_info)) {
				timedout = difftime(sys_info.uptime, update_time);
			}

			//If timeout value is greater the configured sessiont timeout value, send session timeout packet to the particular session
			if(timedout >= vncConf.SessionInactivityTimeout ) {
				if (setTimedOut(cl->sock, 1) != 0) {
					printf("\nUnable to update timedout for : %d \n", cl->sock);
				}
			}
			cl=rfbClientIteratorNext(i);
		}
		if(actvSessCount <= 0)
			break;
	}
	rfbReleaseClientIterator(i);
	return;
}

void rfbDrawStringPerLine(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
		int x,int y,const char* string,rfbPixel colour)
{

	while(*string) {
		// on new line character, modify x and y to move the text to next line.
		if(*string == NEW_LINE) {
			// draw starting from left side of the screen.
			// x is 2 so that there is some space between left margin of the screen and text.
			x  = SCREEN_POS_X;
			// move to next line.
			y += SCREEN_POS_Y;
		}
		x+=rfbDrawChar(rfbScreen,font,x,y,*string,colour);
		string++;
	}
}

int setupFb(struct fb_info_t* fb_info) {
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
	int fd = -1;

	fd = open(FB_FILE, O_RDONLY);
	if (fd < 0) {
		printf("Open %s error!\n", FB_FILE);
		return -1;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_info->fix_screen_info) < 0) {
		printf("\n FBIOGET_FSCREENINFO failes \n");
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_info->var_screen_info) < 0) {
		printf("\n FBIOGET_VSCREENINFO failes \n");
	}

	fb_info->fbmem = mmap(0, fb_info->fix_screen_info.smem_len, PROT_READ, MAP_PRIVATE, fd, 0);
	fbmmap = fb_info->fbmem;
	close(fd);
	if (fbmmap == MAP_FAILED) {
		printf("\nmmap failed\n");
		vncUnloadResource();
		onStartStopVnc(STOP);
		exit(EXIT_FAILURE);
	}
#endif
	emptyScreen= (char*)malloc(maxx*maxy*bpp);
	emptyScreenSize = maxx*maxy*bpp;
	return 0;
}

void fbUnmap(void) {
	munmap(fb_info.fbmem, fb_info.fix_screen_info.smem_len);
}

void adoptResolutionChange(rfbClientPtr cl) {

	rfbPixelFormat prevFormat;
	rfbBool updateFormat = FALSE;
	rfbClientIteratorPtr iterator;

	//resolution changed, so get new resolution.
	if( isResolutionChanged ) {
		getResolution(&maxx,&maxy);
		isResolutionChanged = 0;
#if defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
		getVideoBufferOffset(&video_buffer_offset);
		rfbScreen->frameBuffer = (char*) fb_info.fbmem+video_buffer_offset;
#endif
	}
	//update screen information and intimate all active clients
	prevFormat = cl->screen->serverFormat;
	if (maxx & 3)
		printf("\n updated width [%d] is not a multiple of 4.\n", maxx);

	cl->screen->width= maxx;
	cl->screen->height = maxy;
	cl->screen->bitsPerPixel = cl->screen->depth = 8*bpp;
	cl->screen->paddedWidthInBytes = maxx*bpp;

	//rfbInitServerFormat(screen, bitsPerSample);

	if (memcmp(&cl->screen->serverFormat, &prevFormat,
	     sizeof(rfbPixelFormat)) != 0) {
		updateFormat = TRUE;
	}

	/* update pointer values */

	if (cl->screen->cursorX >= maxx)
		cl->screen->cursorX = maxx - 1;
	if (cl->screen->cursorY >= maxy)
		cl->screen->cursorY = maxy - 1;

	/* intimate active clients */
	iterator = rfbGetClientIterator(cl->screen);
	while ((cl = rfbClientIteratorNext(iterator)) != NULL) {

		if (updateFormat)
			  cl->screen->setTranslateFunction(cl);

#if defined(SOC_AST2400) || defined(SOC_AST2500) ||( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
	rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)cl->updateBuf;
#endif

		LOCK(cl->updateMutex);
		sraRgnDestroy(cl->modifiedRegion);
		cl->modifiedRegion = sraRgnCreateRect(0, 0, maxx, maxy);
		sraRgnMakeEmpty(cl->copyRegion);
		cl->copyDX = 0;
		cl->copyDY = 0;

		if (cl->useNewFBSize)
			cl->newFBSizePending = TRUE;

#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
		fu->type = rfbFramebufferUpdate;
		fu->nRects = Swap16IfLE(1);
		cl->ublen = sz_rfbFramebufferUpdateMsg;
		if (!rfbSendNewFBSize(cl, cl->scaledScreen->width, cl->scaledScreen->height))
		{
			if (cl->screen->displayFinishedHook)
				cl->screen->displayFinishedHook(cl, FALSE);
			return;
		}
		rfbSendUpdateBuf(cl);
#endif

		TSIGNAL(cl->updateCond);
		UNLOCK(cl->updateMutex);
	}
	rfbReleaseClientIterator(iterator);
	//update maxRectColumn for framerectfromtiles
	maxRectColumn = (abs(maxx/TILE_WIDTH)) /2;
}

/* Here the pointer events are handled */
static void handleMouseEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	struct sysinfo sys_info;
	int ioctl_retval = -1;
	if ((powerCons == 0) && (cl->viewOnly == FALSE)) // Process only for master session
	{
		if (mouse_queue_full != 0)
		{
			/* Wait for host to read the data already present in mouse queue.
			** Followed the same wait time as other KVM clients. */
			if ((!sysinfo(&sys_info)) && (difftime(sys_info.uptime, mouse_queue_full) > QUEUE_FULL_WAIT_TIME))
				mouse_queue_full = 0;
		}
		if (mouse_queue_full == 0)
		{
			ioctl_retval = mousePkt(buttonMask, x, y, maxx, maxy);
			if (!sysinfo(&sys_info))
			{
				if (setLastPacketTime(cl->sock, sys_info.uptime) != 0)
					printf("\nUnable to update last packet time\n");

				if (ioctl_retval == QUEUE_FULL_WARNING)
					mouse_queue_full = sys_info.uptime;
			}
		}
	}
}

void handlekeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl) {
	struct sysinfo sys_info;
	if((powerCons == 0) && (cl->viewOnly == FALSE)){ //Process only for master session
		handleKbdEvent(down, key);
		if(!sysinfo(&sys_info))
		{
			 if(setLastPacketTime(cl->sock, sys_info.uptime) != 0)
			 	printf("\nUnable to update last packet time\n");
		}
	}
}

static void init_fb_server(int argc, char **argv) {
	if (rfbScreen == NULL) {
		rfbScreen = rfbGetScreen(&argc, argv, maxx, maxy, 8, 3, bpp);
		if (!rfbScreen)
			return;
		rfbPixelFormat pixfmt = { 32, //U8  bitsPerPixel;
				32, //U8  depth;
				0, //U8  bigEndianFlag;
				1, //U8  trueColourFlag;
				255, //U16 redMax;
				255, //U16 greenMax;
				255, //U16 blueMax;
				16, //U8  redShift;
				8, //U8  greenShift;
				0, //U8  blueShift;
				0, //U8  pad 1;
				0 //U8  pad 2
				};
		rfbScreen->serverFormat = pixfmt;
		rfbScreen->alwaysShared = TRUE;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
		rfbScreen->frameBuffer = (char*) fb_info.fbmem;//(char*)vncbuf;
#endif
		rfbScreen->ptrAddEvent = handleMouseEvent;
		rfbScreen->kbdAddEvent = handlekeyEvent;/* Here the key events are handled */
		rfbScreen->newClientHook = newclient;
		rfbScreen->httpDir = NULL;
		rfbScreen->httpEnableProxyConnect = TRUE;
		//Make vnc to listen to user specified port value.
		if( g_nonsecureport != 0 ){
			rfbScreen->ipv6port = rfbScreen->port = g_nonsecureport;
		} else { // if non secure port is not set, no need to call rfbInitServer
			return;
		}

		/* Check if bit value for secure connection is set.
		** If so listen in loopback interface alone. */

		switch ( vncConf.connection_type )
		{
			case INADDR_LOOPBACK:
				rfbScreen->listen6Interface = "lo";
				break;
			case INADDR_NONE:
				printf("Error: Unable to get listenInterface. Stopping VNC server.\n");
				vncUnloadResource();
				onStartStopVnc(STOP);
				break;
			case INADDR_ANY: /* Default listen interface */
			default:
				break; /* nothing to do !! */
		}
		rfbInitServer(rfbScreen);
	}
}

void vncUnloadResource() {

	if(emptyScreen != NULL)
		free(emptyScreen);
	if( rfbScreen )
		rfbScreenCleanup(rfbScreen);

	if(sessionInfo != NULL)
		free(sessionInfo);

	if(vnc_session_guard != NULL)
		free(vnc_session_guard);

#if !defined(SOC_AST2600)
	//If adviserd service is running, then it may use usb resource. So don't release usb resource.
	// if( IsProcRunning(ADVISERD) == 0 ) {
	// if( IsProcRunning ) {
	// 	CloseUSBDevice();
	// }
#else
	hid_stop(VNC_SERVER);
#endif
	release_video_resources();
	if (vncpipe != -1) {
		closeVncPipe(vncpipe);
	}
	fbUnmap();
}

void onStartStopVnc(int mode) {
	struct stat StatBuf;

	if (mode == STOP){
		if( DisableAutoSwitch == 1 ) {
			if(stat(SET_CMD_IN_PROGRESS, &StatBuf) == 0) {
				if ( unlink(SET_CMD_IN_PROGRESS) != 0 ) {
					printf("\n Unable to remove file %s \n",SET_CMD_IN_PROGRESS);
					perror("Error");
				}
			}
		}
		exit(1);
	}
}

#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
int checkJpegSoi(char * src_buff, int src_len)
{
	int i;

	for(i = 0; i < src_len ; i += 2)
	{
		if( (src_buff[i] == 0xff) &&  (src_buff[i+1] == 0xd8) )
		{
			return (i + 1);
		}
	}
	return 0;
}

int getJpegEoiIndex(char * src_buff, int src_len)
{
	int i = 0;

	if(rfbTightEncodingSupported())
		i = src_len - 8;

	for(; i < src_len + 3; i++)
	{
		if( (src_buff[i] == 0xff)  &&  (src_buff[i+1] == 0xd9) )
			return (i + 1);
	}
	return -1;
}

static void sendJpegFrameToClients(rfbClientPtr cl)
{

	u8 block_size = frame_hdr_vnc.params.BytesPerPixel;

	if (rfbScreen->frameBuffer == NULL)
		return;

	if (!rfbTightEncodingSupported())
	{
		/* Search start index of JPEG image */
		if (checkJpegSoi(rfbScreen->frameBuffer, g_tile_info.compressed_size) <= 0)
		{
			rfbScreen->frameBuffer = NULL;
			return;
		}

		/* Search end index of JPEG image */
		if ((g_tile_info.compressed_size = getJpegEoiIndex(rfbScreen->frameBuffer, g_tile_info.compressed_size)) <= 0)
		{
			rfbScreen->frameBuffer = NULL;
			return;
		}
	}

	if (g_tile_info.compressed_size > 0)
	{
		rfbFramebufferUpdateMsg *fu =
			(rfbFramebufferUpdateMsg *)cl->updateBuf;

		fu->type = rfbFramebufferUpdate;
		fu->pad = 0;

		if (cl->enableLastRectEncoding)
			fu->nRects = 0xFFFF;
		else
			fu->nRects = Swap16IfLE(1);

		cl->ublen = sz_rfbFramebufferUpdateMsg;

		if (rfbTightEncodingSupported())
		{
			rfbSendUpdateBuf(cl);
			cl->tightEncoding = rfbEncodingTight;
		}
		else
		{
			cl->tightEncoding = rfbEncodingJPEG;
			g_tile_info.compressed_size++;
		}

		if (FALSE == rfbSendTightHeader(cl, g_tile_info.pos_x * block_size, g_tile_info.pos_y * block_size, g_tile_info.width, g_tile_info.height))
		{
			printf("\n[%s:%d] Error: rfbSendTightHeader failed!", __FUNCTION__, __LINE__);
			return;
		}

		if (rfbTightEncodingSupported())
			cl->updateBuf[cl->ublen++] = (char)(rfbTightJpeg << 4);

		if (FALSE == rfbSendCompressedDataTight(cl, rfbScreen->frameBuffer, g_tile_info.compressed_size))
		{
			printf("\n[%s:%d] Error: rfbSendCompressedDataTight failed!", __FUNCTION__, __LINE__);
			return;
		}

		if (cl->enableLastRectEncoding)
			rfbSendLastRectMarker(cl);

		rfbSendUpdateBuf(cl);
	}
}
#endif

void* updateVideo()
{
	pthread_t self;
	int capture = VIDEO_ERROR, cleanUp = 0;
	rfbClientIteratorPtr i;
	rfbClientPtr cl;
	struct sysinfo incoming,current;
	unsigned int bCounter = 0;//Blank screen counter
	double timedout = 0;

	struct rectUpdate_t recUpdate[RECT_UPDATE_SIZE];
	struct timespec tv;
	#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)	|| defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
	int initialVideoUpdate = 0;
	#endif


	#ifndef FRAME_RECT_FORM_TILES
		int height=0,width=0,index=0,pos= sizeof( unsigned short );
		int posx=0,posy=0;
		int x=0,y=0;
		int tilecount = 0;
	#else
	int j = 0;
	int count = 0;
#endif //FRAME_RECT_FORM_TILES
			if (sem_init (&getResponse, 0, 0) < 0){
				printf("\n Error: Unable to get Power cons status \n");
			}

		maxRectColumn = (abs(maxx/TILE_WIDTH)) /2;
	while(1){
		sem_wait(&mStartCapture);
		sysinfo(&incoming);
		if( WriteCmdInAMIVncPipe( GET_POWER_CONS_STATUS,0 ) < 0) {
			printf("\n Error:  Unable to get Power cons status\n");
		}
		memset(&tv, 0, sizeof(tv));
		time(&tv.tv_sec);
		tv.tv_sec += 5; // Sometimes it takes more than 3 secs to get response
		//Don't wait more than 5 seconds for response
		sem_timedwait(&getResponse,&tv);

		if( IsPowerconsumptionmode )
		{
			powerCons = 1;
		}
		#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)	|| defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
		else
		{
			initialVideoUpdate = 1;//Libvncserver takes sometime,to update the video data when the initial connection comes.
		}
		#endif	

		//enable usb devices
		updateUsbStatus(1);
#if defined(SOC_AST2600)
		if (gUsbDevice > 0)
		{
			close(gUsbDevice);
			gUsbDevice = -1;
		}
#endif
		while(1){

			if( actvSessCount <= 0)
			{
				printf("VNC client(s) exited...\n");
				g_prv_capture_status = 0;
				capture=VIDEO_ERROR;
				on_vnc_no_session();
				break;
			}
			
			//Need to display HID inititlation info in vnc client
			#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
			if(powerCons == 1 || initialVideoUpdate == 1){
			#else
			if(powerCons == 1){
			#endif
				if( !sysinfo(&current)) {
					timedout = difftime(current.uptime, incoming.uptime);
				}
				
				if(timedout <= 5) /* Same value as other KVM clients */
				{
					if(g_prv_capture_status != capture){
						//X and Y position has been calculated accordingly to show text in middle of the screen
						//showBlankScreen(HID_INITILIZATION,(VNC_MIN_RESX/2) - (sizeof(HID_INITILIZATION)),VNC_MIN_RESY/2);
						#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)	|| defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
						if(powerCons == 1)
						{
							showBlankScreen(HID_INITILIZATION,(maxx/2) - 100,maxy/2);
						}
						else if(initialVideoUpdate == 1)
						{
							showBlankScreen(ESTABLISHING_VNC_SESSION,(maxx/2) - 100,maxy/2);
						}
						#else
						showBlankScreen(HID_INITILIZATION,(maxx/2) - 100,maxy/2);
						#endif
						
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
						i = rfbGetClientIteratorWithClosed(rfbScreen);
						cl=rfbClientIteratorHead(i);
						while (cl)
						{
							rfbUpdateClient(cl);
							cl=rfbClientIteratorNext(i);
						}
						rfbReleaseClientIterator(i);
						continue;
#endif
					}
				}
				else
				{
					powerCons = 0;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
					initialVideoUpdate =0;
					if(rfbScreen->frameBuffer != (char*) fb_info.fbmem){
						rfbScreen->frameBuffer = (char*) fb_info.fbmem;
					}
#endif
				}
			}
			else{
				capture = startCapture();

				/*Adding 0.1 sec sleep to prevent unnecessary screen capture in case if we receive continuous blank screen. 
				This will help reduce VNC Server CPU usage in case of blank screen in host.*/
				if( (VIDEO_NO_CHANGE == capture) || (VIDEO_NO_SIGNAL== capture) )
				{
					
					bCounter++;
					if(bCounter >= 10 )
					{
						usleep(100000);
					}
				}
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
				//while running vnc service,if the BSOD capture signale comes,after capture the video data, saving in to the file
				if((bsodCapture == 1) && (g_tile_info.compressed_size > 0))
				{
					saveBsodCapturedata(frame_hdr_vnc.frame_addr);
					bsodCapture = 0;
				}
#endif
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
				if(RESOLUTION_CHANGED == capture){
					if(rfbScreen->frameBuffer != (char*) fb_info.fbmem){
                                                rfbScreen->frameBuffer = (char*) fb_info.fbmem;
                                        }
					isResolutionChanged = 1;
					adoptResolutionChange(pCl);
				}
				else if(VIDEO_ERROR == capture || VIDEO_FULL_SCREEN_CHANGE == capture){
					rfbMarkRectAsModified(rfbScreen, 0, 0, maxx, maxy);
				}
				else if(VIDEO_SUCCESS == capture){
					if(isTextMode)
					{
						showBlankScreen((char*)tile_info, SCREEN_POS_X, SCREEN_POS_Y);
					}
					else if(rfbScreen->frameBuffer != (char*) fb_info.fbmem+video_buffer_offset ){
						rfbScreen->frameBuffer = (char*) fb_info.fbmem+video_buffer_offset;
						// restore previous resolution and call resolution change when switching between textmode/hid/blankscreen and video
						maxx = pre_maxx;
						maxy = pre_maxy;
						isResolutionChanged = 1;
						adoptResolutionChange(pCl);
					}
					#ifdef FRAME_RECT_FORM_TILES
						count = frameRectFromTiles(&recUpdate[0]);
						if ((count > 0) && (count < RECT_UPDATE_SIZE))
						{
							for (j = 0; j < count; j++)
							{
								rfbMarkRectAsModified(rfbScreen, recUpdate[j].x, recUpdate[j].y, recUpdate[j].width, recUpdate[j].height);
							}
						}
						else
						{
							printf("\n[%s:%d] Unable to form rectangle from tile changes, sending full screen update | count:%d\n", __FUNCTION__, __LINE__, count);
							rfbMarkRectAsModified(rfbScreen, 0, 0, maxx, maxy);
						}
					#else
						tilecount = *((unsigned short*)tile_info);

						for (index=0;i<tilecount;index++)
						{
							x = (*(tile_info+pos+1));
							y = (*(tile_info+pos));
							posy = y *TILE_HEIGHT;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
							posx = x *TILE_WIDTH;
							width = posx+TILE_WIDTH;
							height = posy+TILE_HEIGHT;
#elif defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
							posx = (x-1) *TILE_WIDTH;
							width = TILE_WIDTH;
							height = TILE_HEIGHT;
#endif
							rfbMarkRectAsModified(rfbScreen,posx,posy,width,height);
							pos +=2;
						}
					#endif//FRAME_RECT_FORM_TILES
				}
#else
				if ((RESOLUTION_CHANGED == capture) || (VIDEO_SUCCESS == capture))
				{
					rfbScreen->frameBuffer = frame_hdr_vnc.frame_addr;
					if (RESOLUTION_CHANGED == capture)
					{
						isResolutionChanged = 1;
					}
				}
				else if(VIDEO_NO_CHANGE == capture)
				{
					usleep(100);
				}
#endif
				else if(VIDEO_NO_SIGNAL== capture){
					if(g_prv_capture_status != capture){
						showBlankScreen(EMPTY_STRING,VNC_MIN_RESX/2,VNC_MIN_RESY/2);
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
						i = rfbGetClientIteratorWithClosed(rfbScreen);
						cl = rfbClientIteratorHead(i);
						while (cl)
						{
							rfbUpdateClient(cl);
							cl = rfbClientIteratorNext(i);
						}
						rfbReleaseClientIterator(i);
#endif
					}
				}
				
				if( g_prv_capture_status == VIDEO_NO_SIGNAL && ( capture != VIDEO_NO_SIGNAL && capture != RESOLUTION_CHANGED ) ) {
		            //On no signal, video driver will reset the cursor pattern. So set send_xcursor_packet flag to fetch cursor pattern from driver on video update
					// if( set_send_cursor_pattern_flag_sym ) {
					// 	set_send_cursor_pattern_flag_sym(TRUE);
					// }
				}
			}

			//Ressting bCounter if the curent capture is changed.
			if(g_prv_capture_status != capture)
			{
				bCounter = 0;
			}
			
			g_prv_capture_status =capture;
			i = rfbGetClientIteratorWithClosed(rfbScreen);
			cl=rfbClientIteratorHead(i);

			#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
			
		    /* If master session preferred encoding is tight(UltraVNC/TightVNC clients), and slave session doesn't support
			** tight encoding(RealVNC client) then video update is not happening in mater session. To fix this issue updating the
			** master session preferred encoding to same as slave session(ZRLE).
			**
			** Note: Following logic is assumed for `vnc_max_session` value 2. In case if the value is modified, below logic
			** needs to be updated accordingly. */

			if((cl->next != NULL) && (cl->next->preferredEncoding == rfbEncodingTight))
			{
				cl->next->preferredEncoding = rfbEncodingZRLE;
			}
			#endif

			while(cl) {
#ifdef DEFER_VIDEO_UPDATE
	#if defined(SOC_AST2400) || defined(SOC_AST2500) ||( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
				if (cl->state >= RFB_NORMAL)
				{
					if ( isResolutionChanged == 1) {
						adoptResolutionChange(pCl);
					}
					if ((powerCons == 0) && (g_tile_info.compressed_size > 0))
					{
						sendJpegFrameToClients(cl);
					}
				}
	#else
				rfbUpdateClient(cl);
	#endif
#else
				if (cl->sock >= 0 && !cl->onHold && FB_UPDATE_PENDING(cl) &&
				    !sraRgnEmpty(cl->requestedRegion)) {
					rfbSendFramebufferUpdate(cl,cl->modifiedRegion);
				}
#endif
				//This function call will help to identify the rfb client which has to be closed in the case of
				//session timeout/session disconnect. This function call will check if there is a session with socket fd 
				//which matches the client socket fd, and whether the status is set to TIMED_OUT, or DISCONNECTED of that
				//session. If so isCleanUpSession() will return true.
				 cleanUp = isCleanUpSession(cl->sock);
				// //Close the active VNC clients, if g_kvm_client_state is other than VNC
				if ( ( ( g_kvm_client_state != VNC ) && ( cl->sock != -1 ) ) ||
				 ( (cl->sock < 0 ) && (cl->clientData != NULL)) || (TRUE == cleanUp)) {
					rfbCloseClient(cl);
					rfbClientConnectionGone(cl);
					cleanUp = FALSE;
				}
				cl=rfbClientIteratorNext(i);
			}
			//set Host keyboard LED status in vnc server. 
			SetLEDStatus();
			rfbReleaseClientIterator(i);
			select_sleep(0,10000);// make cpu happy , increasing this value will impact video smooth
		}
	}

	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);
	return NULL;
}


/* Initialization */
int main(int argc, char** argv) {
	printf("\n INFO: VNC server is starting... \n ");
	struct stat StatBuf;
#if defined(SOC_AST2600)
	int ret = -1;
#endif
#ifdef CONFIG_SPX_FEATURE_DISABLE_KVM_AUTO_SWITCH
	DisableAutoSwitch = 1;
#endif

	InitSignals();
	if( DisableAutoSwitch == 1 ) {
		if(stat(SET_CMD_IN_PROGRESS, &StatBuf) == 0) {
			if ( unlink(SET_CMD_IN_PROGRESS) != 0 ) {
				printf("\n Unable to remove file %s \n",SET_CMD_IN_PROGRESS);
				perror("Error ");
			}
		}
	}

	vncpipe = open(VNC_CMD_PIPE,O_RDWR|O_NONBLOCK);
	if (vncpipe == -1) {
		printf("Error: Unable to open VNC_CMD_PIPE.\n");
		perror("Error ");
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}
	AmiVNCPipe = open(VNC_AMI_CMD_PIPE,O_RDWR|O_NONBLOCK);
	if (AmiVNCPipe == -1) {
		printf("Error: Unable to open VNC_AMI_CMD_PIPE.\n");
		perror("Error ");
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}

	if (0 != pthread_create (&cmdThread,NULL,checkIncomingCmd,NULL))
	{
		printf("Error: VNC cmdThread creation failed.\n");
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}
	if (sem_init (&waitForAMIVncResponse, 0, 0) < 0){
		printf("\n Init sem failed \n");
	}


	if( WriteCmdInAMIVncPipe(GET_VNC_CONF,0) < 0 )
	{
		printf("\n Unable to the request vnc conf information");
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}

	if( requestKVMClientstate() < 0)
	{
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
		printf("\n Unable to request kvm client state \n");
	}

	if( WriteCmdInAMIVncPipe (GET_SESSION_MANAGEMENT_STATUS,0 ) <0 ) {
		printf("\n Unable to get Session Management details ");
	}
	//disable default rfb log
	rfbLogEnable(!ENABLED);
	fb_setup_status = setupFb(&fb_info);

	if (sem_init (&mStartCapture, 0, 0) < 0){
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}

	if (sem_init (&mSessionTimer, 0, 0) < 0) {
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}

	init_video_resources();
#if !defined(SOC_AST2600)
	init_usb = init_usb_resources();
	save_usb = save_usb_resource();
#else
	if ((ret = hid_open(VNC_SERVER)) < 0)
	{
		printf("\n Unable to open hid devices %d\n", ret);
		vncUnloadResource();
        loadDefaultVNC();
        exit(1);
	}
#endif

	/* Implement our own loop to detect changes in the video and transmit to client. */
	if (0 != pthread_create (&videoThread, NULL,updateVideo,NULL))
	{
		printf("\n Thread creation failed...\n");
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}

	if (0 != pthread_create(&timerThread, NULL,checkTimer,NULL))
	{
		vncUnloadResource();
		loadDefaultVNC();
		exit(1);
	}
	if (sem_init (&waitForConfUpdate, 0, 0) < 0){
		printf("\n Error: Unable to update vnc configuration\n");
	}

	//if (0 != pthread_create(&cursorThread, NULL,updateCursor,NULL))
//	{
//		vncUnloadResource();
//		onStartStopVnc(STOP);
//	}
	sem_wait(&waitForConfUpdate);
	printf("\n INFO: GPL VNC server is Started... \n ");
	while (1) {

		while (rfbScreen->clientHead == NULL)
		{
			rfbProcessEvents(rfbScreen, 10000);
		}
		
		vncrfbProcessEvents(rfbScreen, 1000);
	}
	vncUnloadResource();
	onStartStopVnc(STOP);
	return (0);
}

/* Function to switch vnc screen to blank screen. If message is passed will be used to display it */
void showBlankScreen(char *message,int xPos,int yPos) {
	char formattedString[TEXT_LEN] = {0};
	int resChanged = 0;
	if(rfbScreen->frameBuffer != emptyScreen){
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
		rfbScreen->frameBuffer = emptyScreen;

		if(maxx != VNC_MIN_RESX)
		{
			pre_maxx = maxx;
			maxx = VNC_MIN_RESX;
			resChanged = 1;
		}
		if(maxy != VNC_MIN_RESY)
		{
			pre_maxy = maxy;
			maxy = VNC_MIN_RESY;
			resChanged = 1;
		}
		if(resChanged)
			adoptResolutionChange(pCl);
#elif defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
		if( maxx != VNC_MIN_RESX || maxy != VNC_MIN_RESY ) {
			if(emptyScreen != NULL) {
				free(emptyScreen);
				emptyScreen= (char*)malloc(maxx*maxy*bpp);
				emptyScreenSize = maxx*maxy*bpp;
			}
		}
		rfbScreen->frameBuffer = emptyScreen;
		isResolutionChanged = 1;
#endif
	}

	memset(emptyScreen, 0, emptyScreenSize);

	if(message != NULL && (message[0] != '\0')) {

		if(isTextMode)
		{
			// clear destination string and screen.
			memset(formattedString, 0, TEXT_LEN);
			// Format the text to include newline
			formatString(message, formattedString);
			// paint the formatted string
			rfbDrawStringPerLine(rfbScreen,&radonFont,xPos,yPos,formattedString,0xffffff);
		}
		else
		{
			rfbDrawString(rfbScreen,&radonFont,xPos,yPos,message,0xffffff);
		}
	}
	rfbMarkRectAsModified(rfbScreen,0,0,maxx,maxy);
	return ;
}

/**
 * Get the list of active sockets.
 * 
 * @param active_sock_list - the array of active sockets.
 * 
 */
void getActiveSocketList(SOCKET* active_sock_list){
	
	rfbClientIteratorPtr itr;
	rfbClientPtr cli;
	int count = 0;
	if(active_sock_list == NULL)
		return;
	itr = rfbGetClientIteratorWithClosed(rfbScreen);
	cli = rfbClientIteratorHead(itr);
	while(cli)
	{
		if(count > actvSessCount)
			break;
		active_sock_list[count] = cli->sock;
		count++;
		cli=rfbClientIteratorNext(itr);
	}
	rfbReleaseClientIterator(itr);
}

/**
 * checkTimer - calls vnc_session_timercall on regular interval (VNC_TIMER_TICKLE) seconds
 * to check if the given sessions are timedout or not.
 * 
 */
void *checkTimer()
{
	pthread_t self;
	int SleepIntervalSecs = VNC_TIMER_TICKLE;
	int timeleftSecs;
	prctl(PR_SET_NAME,__FUNCTION__,0,0,0);

	while(1)
	{
		// wait till a session is registered
		sem_wait(&mSessionTimer);

		while(1)
		{
			timeleftSecs = SleepIntervalSecs;
			while(timeleftSecs)
			{
				timeleftSecs = sleep(timeleftSecs);
			}

			//call the Timer
			vnc_session_timercall();

			if(actvSessCount <= 0)
				break;
		}
	}

	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);
	return NULL;
}

//Newjkdjdjfakdjfakldddddddddddddd;

/**
 * getLastPacketTime - returns last packet sent time
 * 
 * input:
 * fd - socket id
 * 
 * output:
 * lastPacketTime - last packet sent time
 */
time_t getLastPacketTime (SOCKET fd)
{
	int i = 0;
	time_t lastPacketTime = 0;

	for(i = 0; i < vnc_max_session; i++)
	{
		pthread_mutex_lock(&vnc_session_guard[i]);
		if(sessionInfo[i].sock == fd)
		{
			lastPacketTime = sessionInfo[i].lastPacketTime;
			pthread_mutex_unlock(&vnc_session_guard[i]);
			break;
		}
		pthread_mutex_unlock(&vnc_session_guard[i]);
	}

	return lastPacketTime;
}



/**
 * setTimedOut - updates timed out
 * 
 * input:
 * fd - socket id
 * timedout - 1 or 0 (whether the current session is timedout or not)
 * 
 * output:
 * 0 on success, -1 on failure
 */
int setTimedOut(SOCKET fd, int timedout)
{
	int i = 0, status = -1;

	for(i = 0; i < vnc_max_session; i++)
	{
		pthread_mutex_lock(&vnc_session_guard[i]);
		if(sessionInfo[i].sock == fd)
		{
			if(timedout == TRUE){
				sessionInfo[i].status = TIMED_OUT;
			}
			else{
				sessionInfo[i].status = ACTIVE;
			}
			status = 0;
			pthread_mutex_unlock(&vnc_session_guard[i]);
			break;
		}
		pthread_mutex_unlock(&vnc_session_guard[i]);
	}

	return status;
}

int setLastPacketTime (SOCKET fd, time_t lastPacketTime)
{
	int i = 0, status = -1;

	for(i = 0; i < vnc_max_session; i++)
	{
		pthread_mutex_lock(&vnc_session_guard[i]);
		if(sessionInfo[i].sock == fd)
		{
			sessionInfo[i].lastPacketTime = lastPacketTime;
			status = 0;
			pthread_mutex_unlock(&vnc_session_guard[i]);
			break;
		}
		pthread_mutex_unlock(&vnc_session_guard[i]);
	}

	return status;
}

int updateTitleString(char* dest, int isViewOnly)
{
	int ret = -1;
	char titleString[TITLE_STRING_LEN];
	FILE *fp;
	size_t len = 0;
	char *temp = NULL;

	// fetch titleString from configuration
	// get vnc title string
	memset(titleString,0,TITLE_STRING_LEN);
	fp = fopen(VNC_TITLE_STRING, "r");
	if ( fp == NULL ) {
		//printf("\n Unable to open %s \n",VNC_TITLE_STRING);
	}
	else
	{
		if ( getline(&temp, &len, fp) != -1 )
		 {
			 if ( temp != NULL ){
			 		ret = snprintf(titleString, len, "%s", temp );
					if( ret< 0 && ret >=(signed)len )
					{
						printf("Buffer Error\n");
					}
			 }
		}
		else
		{
			printf("\n Unable to get title string \n");
		}
		fclose(fp);
	}

	// if configured, then append configured string to dest.
	if('\0' != titleString[0])
	{
		ret = snprintf(dest,TITLE_STRING_LEN,"%s", titleString);
		if ((ret < 0) || (ret >= TITLE_STRING_LEN )) {
			printf("Buffer Overflow while setting VNC viewer title string");
			return ret;
		}
	}
	else{ // if not configured, then append default string to dest.
		ret = snprintf(dest,TITLE_STRING_LEN,"%s", DEFAULT_TITLE_STRING);
		if ((ret < 0) || (ret >= TITLE_STRING_LEN )) {
			printf("Buffer Overflow while setting VNC viewer title string");
			return ret;
		}
	}

	// append view only for slave VNC session.
	if(isViewOnly)
	{
	    ret = snprintf(dest+strlen(dest), VIEW_ONLY_STRING_LEN,"%s", VIEW_ONLY);
		if ((ret < 0) || (ret >= VIEW_ONLY_STRING_LEN)) {
			printf("Buffer Overflow while setting VNC viewer title string");
			return ret;
		}
	}

	return 0;
}

void setActiveSessionCount(int activeSessCnt){

	if( WriteCmdInAMIVncPipe(SET_ACTIVE_VNC_SESSION_COUNT,actvSessCount ) < 0){
		printf("\n Unable to set active session info \n");
	}
}

int setSessionInfo(SOCKET fd, time_t lastPacketTime, char *clientIP)
{
	int i = 0, retval = 0;

	for(i = 0; i < vnc_max_session; i++)
	{
		pthread_mutex_lock(&vnc_session_guard[i]);

		if(sessionInfo[i].sock <= 0)
		{
			sessionInfo[i].sock = fd;
			sessionInfo[i].status = ACTIVE;
			sessionInfo[i].lastPacketTime = lastPacketTime;

			if(session_management == ENABLED)
			{
				if(clientIP != NULL)
				{
					retval = snprintf((char *)&sessionInfo[i].clientIP, sizeof(sessionInfo[i].clientIP), "%s", clientIP);
					/* snprintf() will modify existing value. So need to update the variable accordingly post
					** snprintf() execution. */
					if((retval < 0) || (retval >= (signed)sizeof(sessionInfo[i].clientIP)))
					{
						if(retval < 0)
							printf("Output error encountered while parsing VNC client IP Address !!!!");
						else
							printf("Buffer Overflow in parsing VNC client IP Address !!!!");
						retval = -1;
					}
					else {
						retval = 0;
						//clear rac session info
						// /* TODO: Replace default values with user management implementation */
						if ( send_session_info(i) < 0  ) {
							printf("\n send_session inf failed \n");
							retval = -1;
						}
					}
				}
				else
				{
					retval = -1;
				}
			}
			pthread_mutex_unlock(&vnc_session_guard[i]);
			break;
		}
		pthread_mutex_unlock(&vnc_session_guard[i]);
	}

	if (session_management != ENABLED && session_management >=0 )
	{
		// update active session
		if( WriteCmdInAMIVncPipe( SET_ACTIVE_VNC_SESSION,actvSessCount ) < 0 ){
			printf("\n Unable to set active session info \n");
		}
	}

	return retval;
}

void closeVncPipe(int fd) {
	if (fd != -1)
		close(fd);
}

void select_sleep(time_t sec,suseconds_t usec)
{
    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    while(select(0, NULL, NULL, NULL, &tv) < 0);
}

int formatString (char* src , char* formattedString)
{
	int index, aIndex, charLen;
	int len, textWidth,textHeight;
	int y = 0;
	charLen = aIndex = 0;
	len = textWidth = textHeight = -1;

	textWidth = (frame_hdr_vnc.res_x);

	textHeight = ( frame_hdr_vnc.res_y  / frame_hdr_vnc.params.char_height);
	len =  (textWidth * textHeight * 2) ;

	for(y = 0; y < TEXT_LEN; y++)
	{
		if(src[y] == BEL)
		{
			// In text mode, if there is no non-printable characters the text are aligned improperly.
			// fix that by adjusting textWidth
			textWidth -=1;
			break;
		}
	}


	for (index = 0; index < len; index++)
	{
		//textWidth character per line include new line on textWidth+1.
		if(charLen == textWidth)
		{
			formattedString [aIndex] = NEW_LINE;
			aIndex++;
			charLen = 0;
			continue;
		}

		//total no.of characters need to be verified
		if(aIndex > ((textWidth*textHeight)+textHeight))
		{
			break;
		}
		//Skip non-prinitng Characters
		if( ( src [index] <= NON_PRINT_CHAR_RANGE) )
			continue;
		charLen++;
		formattedString [aIndex] = src [index];
		aIndex ++;
	}

	formattedString [aIndex] = '\0';
	return aIndex;
}

int isCleanUpSession(SOCKET fd)
{
        int sessionIndex = vnc_max_session;
        int cleanup = FALSE;
        if(session_management != ENABLED){
        	// update active session
        	if( WriteCmdInAMIVncPipe(SET_ACTIVE_VNC_SESSION,actvSessCount ) < 0 ){
        		printf("\n Unable to set active session info \n");
        	}
        	if ((sessionInfo[sessionIndex].status == TIMED_OUT) || (sessionInfo[sessionIndex].status == DISCONNECTED)){
        		cleanup = TRUE;
        	}
        }
        else{
        	if (fd > 0)
        	{
        		while(sessionIndex--)
        		{
        			pthread_mutex_lock(&vnc_session_guard[sessionIndex]);
        			if(sessionInfo[sessionIndex].sock == fd)
        			{
        				if ((sessionInfo[sessionIndex].status == TIMED_OUT) || (sessionInfo[sessionIndex].status == DISCONNECTED)){
        					cleanup = TRUE;
        				}
        				pthread_mutex_unlock(&vnc_session_guard[sessionIndex]);
        				break;
        			}
        			pthread_mutex_unlock(&vnc_session_guard[sessionIndex]);
        		}
        		if(sessionIndex >= 0 && ( (sessionInfo[sessionIndex].status == TIMED_OUT) || (sessionInfo[sessionIndex].status == DISCONNECTED) ) ){
        			cleanUpInvalidSession(sessionIndex, sessionInfo[sessionIndex].status);
        		}
        	}
        }
        return cleanup;
}

/**
* cleanUpInvalidSession - cleans up invalid session register entries.
* @param sessionIndex - The index of the session to be cleaned up in the session info list.
* 						If there is any valid session index available, then the function will
* 						try to clean up the session info at that index. If the index ins unknown,
* 						then -1 should be passed, and the function will unregister all the rac session
* 						entries in this case.  
* @param reason - session cleanup reason. Either TIMED_OUT, DISCONNECTED, or CLEAN_UP.
*/
void cleanUpInvalidSession(int sessionIndex, int reason)
{
	if (session_management != ENABLED)
	{
		// update active session
		if( WriteCmdInAMIVncPipe(SET_ACTIVE_VNC_SESSION,actvSessCount ) < 0 ){
			printf("\n Unable to set active session info \n");
		}
	}
	else{
		if( clear_vnc_session(sessionIndex,reason) < 0 ){
			printf("\n Unable to clean VNC session Info \n ");
		}
	}
	sessionInfo[sessionIndex].status = UNUSED;
	sessionInfo[sessionIndex].sock = -1;
}

void onClientGone(SOCKET* active_sock_list) {
	int sess_index = -1;
		int sock_index;
		if (session_management != ENABLED)
		{
			// update active session
			if( WriteCmdInAMIVncPipe( SET_ACTIVE_VNC_SESSION,actvSessCount ) < 0 ){
				printf("\n Unable to set active session info \n");
			}
		}
		else
		{
			if(active_sock_list != NULL){
				for(sess_index = 0; sess_index < vnc_max_session; sess_index++){
					pthread_mutex_lock(&vnc_session_guard[sess_index]);
					//Check the sock fd corresponding to the sessions in the session info structure against entries in the
					//active socket list. If a match is found then break the loop.
					for(sock_index = 0; sock_index < actvSessCount; sock_index++){
						if(sessionInfo[sess_index].sock ==  (int)active_sock_list[sock_index]){
							break;
						}
					}
					//If a match is not found for a socket in the session info structure against the active socket list,
					//for a particular index; then clean up that session(since the socket corresponding to it is no more valid).
					if(sock_index >= actvSessCount){
						//Update session status only of the socket FD is valid
						if(sessionInfo[sess_index].sock > 0){
							sessionInfo[sess_index].status = DISCONNECTED;
						}
						pthread_mutex_unlock(&vnc_session_guard[sess_index]);
						break;
					}
					pthread_mutex_unlock(&vnc_session_guard[sess_index]);
				}
				cleanUpInvalidSession(sess_index, DISCONNECTED);
			}
		}
}

void on_vnc_no_session(){
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
	memset(&g_tile_info, 0, sizeof(g_tile_info));
#endif
	//combine it with client gone	
	updateUsbStatus(0);
#if defined(SOC_AST2600)
	if (gUsbDevice > 0)
	{
		close(gUsbDevice);
		gUsbDevice = -1;
	}
#endif
}


VNCSessionInfo  *getSessionInfo()
{
	sessionInfo = (VNCSessionInfo *) malloc ((sizeof(VNCSessionInfo) * vnc_max_session));
	memset((char *)sessionInfo, 0, (sizeof(VNCSessionInfo) * vnc_max_session));
	return sessionInfo;
}
pthread_mutex_t  *getVncSessionGuard()
{
	vnc_session_guard = (pthread_mutex_t *)calloc(vnc_max_session, sizeof(pthread_mutex_t));
	return vnc_session_guard;
}

int openVncPipe(char *pipename) {
	int fd = -1;
	if (pipename != NULL) {
		if(unlink(pipename) == -1)
			printf("[%d]:could not unlink the pipe %s\n", errno, pipename);
		if (mkfifo(pipename, PERMISSION_RWX) == -1)
			printf("[%d]:could not create the pipe %s\n", errno, pipename);
		fd = open(pipename, O_RDWR|O_NONBLOCK);
		if (fd < 0) {
			printf("[%d]:could not open the pipe %s\n", errno, pipename);
			if(unlink(pipename) == -1)
				printf("[%d]:could not unlink the pipe %s\n", errno, pipename);
		}
	}
	return fd;
}

#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
/*updateBsodCapturedate: write the capture data in to a file. 
*/
void saveBsodCapturedata(void* p_data_buf)
{
	FILE * fp = NULL;
	if( (p_data_buf != NULL )&& (g_tile_info.compressed_size !=0))
	{
		fp=fopen (CRASH_SCREEN_FILE_JPEG, "w");
		if( fp == NULL )
		{
			printf("\n[%s:%d] Error in opening the file ", __FUNCTION__, __LINE__);
			return;
		}
		if(g_tile_info.compressed_size > fwrite(p_data_buf , sizeof(char) , g_tile_info.compressed_size , fp ))
		{
			printf("\n[%s:%d] Error:Unable to write bsod cpature data in to jpeg file ", __FUNCTION__, __LINE__);
			fclose(fp);
			return;
		}
		fclose(fp);
	}	
}
#endif
void* checkIncomingCmd() {
	cmd_info_t cmd;
	pthread_t self;
	#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
	int capture = VIDEO_ERROR;
	void* p_frame_hdr = NULL;
        void* p_data_buf = NULL;
	#endif
	memset(&cmd, 0, sizeof(cmd));
#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
	char pswd[MAXPWLEN+1]; // rfbproto.h, +1 for null termination
#endif
	// session_disconect_record_t *disconnect_data;
	while (vncpipe != -1) {

		if (read(vncpipe, &cmd, sizeof(cmd)) > 0) {

			switch (cmd.cmd)
			{
				case KVM_CLIENT_STATE:
					if( ( cmd.status == ADVISER ) || ( cmd.status == VNC ) ) {
						g_kvm_client_state = cmd.status;
						if( DisableAutoSwitch == 1 ) {
							if( g_kvm_client_state != VNC ) {
								vncUnloadResource();
								onStartStopVnc(STOP);
							}
						}
					}
					else{
						printf("\n Invalid KVM client option recevied in VNC pipe ");
					}
				break;
				case RECV_VNC_CONF:
					if(cmd.datalen > 0)
					{
						if(read(vncpipe, &vncConf, cmd.datalen) > 0)
						{
							// update vnc_max_session if it's changed
							if(vnc_max_session != vncConf.MaxAllowSession)
								vnc_max_session = vncConf.MaxAllowSession;
							// no need to re init rfb server if nonsecure port didn't change
							if(g_nonsecureport == vncConf.NonSecureAccessPort)
								break;
							g_nonsecureport = vncConf.NonSecureAccessPort;
							//Initialize vnc server
							init_fb_server(0, NULL);
							if (rfbScreen == NULL) {
								printf("\nUnable to init_fb_server. Launching default VNC application.\n");
								vncUnloadResource();
								loadDefaultVNC();
								exit(1);
							}
							if( vnc_session_guard)
								free(vnc_session_guard);
							vnc_session_guard = getVncSessionGuard();

							if (vnc_session_guard == NULL)
							{
								printf("unable to allocate resource for vnc session guard\n");
								vncUnloadResource();
								onStartStopVnc(STOP);
							}
							
							if( sessionInfo)
								free(sessionInfo);
							sessionInfo = getSessionInfo();
							if (sessionInfo == NULL)
							{
								printf("unable to allocate resource for vnc session info\n");
								vncUnloadResource();
								onStartStopVnc(STOP);
							}
						}
						else
						{
							printf("Error in reading VNC conf data");
						}	
					}
					sem_post(&waitForConfUpdate);
					break;
				case POWER_CONS_STATUS:
						if( cmd.status == 0 || cmd.status == 1 ){
							IsPowerconsumptionmode = cmd.status;
							sem_post(&getResponse);
						}
						else{
							printf("\n Invalid power consumption mode \n");
						}
					break;
				case SESSION_MANAGEMENT_STATUS:
					if( cmd.status == 0 || cmd.status == 1 )
						session_management =  cmd.status;
					break;
				case DISCONNECT_VNC_SESSION:

					memset(&vnc_session,0,sizeof( vnc_session_index_t ));
					if(read(vncpipe, &vnc_session, sizeof(vnc_clean_session_t)) > 0)
					{
						sessionInfo[ vnc_session.session_index ].status = vnc_session.reason;
					}
					else
					{
						printf("\nError in reading VNC session data to be killed \n");fflush(stdout);
					}	
					break;
				case USB_STATUS_RESPONSE:
#if !defined(SOC_AST2600)
					isUSBRunning = TRUE;
#endif
					sem_post(&getResponse);
					break;
#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
				case VNC_PASSWORD_CMD:
					if (cmd.datalen > 0)
					{
						memset(&pswd, 0, sizeof(pswd));
						if (read(vncpipe, &pswd, cmd.datalen) > 0)
						{
							if (rfbEncryptAndStorePasswd((char *)&pswd, (char *)VNC_PASSWORD_FILE) != 0)
							{
								printf("\n[%s:%d] Error in storing VNC password data", __FUNCTION__, __LINE__);
							}
						}
						else
						{
							printf("\n[%s:%d] Error in reading VNC password data", __FUNCTION__, __LINE__);
						}
					}
					break;
#endif
					#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
					case VNC_BSOD_CAPTURE:
						isNewSession = TRUE;
						if( actvSessCount == 0 )
						{
							//Intimate video driver to switch JPEG capture mode
							FILE *fp = NULL;
							fp= fopen(VIDEOCAP_JPEG_ENABLE, "w");
							if (fp != NULL)
							{
								fprintf(fp, "%d\n", g_kvm_client_state);
								fclose(fp);
							}
							else
							{
								printf("\n[%s:%d] Error in opening the file ", __FUNCTION__, __LINE__);
								break;
							}
							memset(&g_tile_info, 0, sizeof(g_tile_info));
							capture = captureVideo(&p_frame_hdr, &p_data_buf);
							if(RESOLUTION_CHANGED == capture || CAPTURE_VIDEO_SUCCESS == capture )
							{
								saveBsodCapturedata(p_data_buf);
							}
							else
							{
								printf("\n[%s:%d] Error:Unable to cpature the vedio data capture=%d\n ", __FUNCTION__, __LINE__,capture);
							}		
						}
						else
						{
							/*If the VNC session is running setting the flag,based on this value ,updatevideo save the
							capture data in to a file*/
							bsodCapture = 1;
						}	
					#endif	
					break;

				default:
					printf("Unsupported command received in VNC pipe [%d]\n", cmd.cmd);
					break;
			}
		}
		select_sleep(1,0);
	}
	self = pthread_self();
	pthread_detach(self);
	pthread_exit(NULL);
	return NULL;
}

int WriteCmdInAMIVncPipe(int cmd, int status){
	    cmd_info_t comm_info;
	
	    if( access(VNC_AMI_CMD_PIPE, F_OK) < 0 ) {
	        printf("\n Unale to acess pipe %s \n ",VNC_AMI_CMD_PIPE);
					return -1;
	    }

	    if(AmiVNCPipe != -1)
	    {
	        comm_info.cmd = cmd;
	        comm_info.datalen = sizeof(cmd_info_t);
	        comm_info.status = status;
	        if(write(AmiVNCPipe,&comm_info,sizeof(cmd_info_t)) == -1)
	        {
	        	printf("Unable to write into %s pipe\n",VNC_AMI_CMD_PIPE);
	        	return -1;
	        }
	    }
	    else
	    {
	        printf("Unable to open the command pipe::%s\n",VNC_AMI_CMD_PIPE);
	        return -1;
	    }
	    return 0;
}

int requestKVMClientstate()
{

	if( WriteCmdInAMIVncPipe(GET_KVM_CLIENT_VALUE,0) == -1 )
	{
		printf("\n requestKVMclientState fails \n");
		return -1;
	}
	return 0;
}

static enum rfbNewClientAction newclient(rfbClientPtr cl) {
	struct sysinfo sys_info;
	struct timespec tv;
#if defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
	FILE *fp=NULL;
#endif
	if(fb_setup_status == -1)
	{
		setupFb(&fb_info);
		fb_setup_status = 0;
	}
#if !defined(SOC_AST2600)
	if(init_usb == -1)
	{
		init_usb_resources();
		init_usb = 0;
	}
	if(save_usb == -1)
	{
		save_usb_resource();
		save_usb = 0;
	}
#endif
	if( DisableAutoSwitch != 1 ) {
		//No need to update KVM client state if there is already active VNC session and kvm client state is VNC
		if( actvSessCount <=0 && g_kvm_client_state != VNC )
		{
			if( WriteCmdInAMIVncPipe(SET_VNC_AS_CLIENT_TYPE, 0) < 0 )
			{
				printf("\n Request to update kvm client type as VNC failed \n");
				rfbCloseClient(cl);
				return RFB_CLIENT_REFUSE;
			}
			memset(&tv, 0, sizeof(tv));
			time(&tv.tv_sec);
			//adding 5 secs wait to get response. If there is not response in 5 seconds, then we will proceed further
			tv.tv_sec += 5;
			sem_timedwait(&waitForAMIVncResponse,&tv);
		}
	}

	if ( g_kvm_client_state == VNC ) {
		//Check for maximum session reach
		if(actvSessCount >= vnc_max_session){
			rfbCloseClient(cl);
			return RFB_CLIENT_REFUSE;
		}
#if defined(SOC_AST2400) || defined(SOC_AST2500) ||  ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
		// Intimate video driver to switch JPEG capture mode
		fp = fopen(VIDEOCAP_JPEG_ENABLE, "w");
		if (fp != NULL)
		{
			fprintf(fp, "%d\n", g_kvm_client_state);
			fclose(fp);
		}
		else
		{
			printf("\n[%s:%d] Error: Unable intimate video driver to switch jpeg capture mode\n", __FUNCTION__, __LINE__);
			rfbCloseClient(cl);
			return RFB_CLIENT_REFUSE;
		}
#endif
#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
		/* Password authendication support */
		if (vncConf.connection_type == INADDR_ANY) { // Check if nonsecure option is enabled
			VncPasswordCheck(cl);
		}
#endif
		cl->clientData = (void*) calloc(sizeof(ClientData), 1);
		cl->clientGoneHook = clientgone;
		cl->enableSupportedMessages = TRUE;
		seqno = 0;
		m_btn_status = 0;
		pCl = cl;

		if(actvSessCount < 0)
			actvSessCount = 1;
		else
			actvSessCount++;
		/*new incoming session so post to resume update video thread.
		if there is only one session then wakeup thread else it will
		be already running.
		*/
		setActiveSessionCount(actvSessCount);//updating activesession count in extension pakages
		if(actvSessCount == 1){
			isNewSession = TRUE;
			sem_post(&mStartCapture);
			g_master_present = MASTER_PRESENT;
			if(updateTitleString(titleString, FALSE) == 0)
				cl->screen->desktopName = titleString;
		}
		else {
		
			/*If second session comes ,
			*always check whether master session avaible before giving permission.
			*may be in previsous session master might exited when active "view-only" was present.
			*current active session might be a "view-only".
			*if So incoming session will become a master.
			*/
			if(g_master_present == MASTER_PRESENT){
				//Master session avaible.
				cl->viewOnly = TRUE;//By setting TRUE we are making it a "view-only"
				if(updateTitleString(titleString_viewOnly, TRUE) == 0)
					cl->screen->desktopName = titleString_viewOnly;
			}
			else	{	//master is not present
				cl->viewOnly = FALSE; //by setting FALSE ,we are making him a master
				g_master_present = MASTER_PRESENT;//Set master present.
				if(updateTitleString(titleString, FALSE) == 0)
					cl->screen->desktopName = titleString;
			}
		}

		if(actvSessCount > 0) {
			/* If incoming connection is granted with master privilege, then we
			** have to wake up thread to capture cursor pattern change.
			** (Applicable only if hardware cursor feature is enabled) */
			// if((cl->viewOnly == FALSE) )
			// 	sem_post(&mStartCursorCapture);
		}
		if(!sysinfo(&sys_info))
		{
			if(setSessionInfo(cl->sock, sys_info.uptime, cl->host) != 0) {
				printf("\nerror while setting session info...\n");
			}else{
				sem_post(&mSessionTimer);
			}
		}
		return RFB_CLIENT_ACCEPT;
	}
	else if ( g_kvm_client_state == -1 )
	{
		vncUnloadResource();
		loadDefaultVNC(0,NULL);
		exit(1);
	}
	else
	{
		//TODO:close connection as VNC is not selected as KVM client
		rfbCloseClient(cl);
		return RFB_CLIENT_REFUSE;
	}
	return RFB_CLIENT_ACCEPT;
}

int send_session_info( int index) {

	cmd_info_t comm_info;
	if( access(VNC_AMI_CMD_PIPE, F_OK) < 0 ) {
		return -1;
	}

	if(AmiVNCPipe != -1)
	{
		memset(&comm_info, 0, sizeof(cmd_info_t) );
		comm_info.cmd = SEND_SESSION_INFO;
		comm_info.datalen = sizeof(VNCSessionInfo);
		comm_info.status = index;
		if(write(AmiVNCPipe,&comm_info,sizeof(cmd_info_t)) == -1)
		{
			printf("Unable to write into %s pipe\n",VNC_AMI_CMD_PIPE);
			return -1;
		}
		if(write(AmiVNCPipe,&sessionInfo[index],sizeof(VNCSessionInfo)) == -1 )
		{
			printf("\n session session failed\n");
			return -1;
		}
	}
	else
	{
		printf("Unable to open the command pipe::%s\n",VNC_AMI_CMD_PIPE);
		return -1;
	}
	return 0;
}

int clear_vnc_session( int sessionIndex, int reason)
{
	vnc_clean_session_t vnc_clean_session;
	
	if( access(VNC_AMI_CMD_PIPE, F_OK) < 0 ) {
		return -1;
	}
	if(AmiVNCPipe != -1)
	{
		memset(&vnc_clean_session, 0, sizeof(vnc_clean_session_t));

		vnc_clean_session.comm_info.cmd = CLEANUP_VNC_SESSION;
		vnc_clean_session.comm_info.datalen = sizeof(vnc_clean_session_t);
		vnc_clean_session.comm_info.status = 0;
		vnc_clean_session.session.session_index = sessionIndex;
		vnc_clean_session.session.reason = reason;
		
		if(write(AmiVNCPipe,&vnc_clean_session,sizeof(vnc_clean_session_t)) == -1 )
		{
			printf("\n session session failed\n");
			return -1;
		}
	}
	else
	{
		printf("Unable to open the command pipe::%s\n",VNC_AMI_CMD_PIPE);
		return -1;
	}
	return 0;
}

void vnc_signal_handler(int signum)
{
	switch (signum) {
		// This signal is for, when we are stopping this vnc server under non-configuration changes
		case SIGHUP:
		case SIGKILL:
			// This signal is for, when any configuration change occurs in BMC
		case SIGUSR1:
			/* In case of active session, stop takes more time so service will get stopped before
			** clearing entry in ProcMonitor. It will result in continuous re-spawn of VNC process.
			** To avoid this following fix is added. */
//			if(actvSessCount > 0) { // Not needed when no active session.
//				deRegisterVnc();
//			}
			// printf("\n VNC_siganl Handler %d \n",signum);
			vncUnloadResource();
			onStartStopVnc(STOP);
			break;
			// This signal is for, when we want to start auto-video recording
		case SIGUSR2:
			// This signal is for, when we receive any service information change and to send it to clients
		case SIGALRM:
		default:
			break;
		}
}

void InitSignals(){
	signal(SIGINT, vnc_signal_handler);
	signal(SIGHUP, vnc_signal_handler);
	signal(SIGKILL, vnc_signal_handler);
	signal(SIGUSR1, vnc_signal_handler);
	signal(SIGUSR2, vnc_signal_handler);
	signal(SIGALRM, vnc_signal_handler);
}

int updateUsbStatus(int enable)
{
	char devicestatus = 0;
	int retval = -1;
	int cmd = 0;
	struct timespec tv;
	


	if((enable != 0) && (enable != 1))
		return retval;


	// if power consumption mode is disabled, then devices will be always enabled. So no need to update
	if(!IsPowerconsumptionmode )
	{
		return 0;
	}
#if defined(SOC_AST2600)
	gUsbDevice = open ("/dev/usbg0", O_RDWR);
	if (gUsbDevice < 0) {
		printf("\nOpenUSBDevice failed \n");
		fflush(stdout);
		return -1;
	}
#endif

	if (ioctl(gUsbDevice, USB_GET_ALL_DEVICE_STATUS, &devicestatus) < 0)
	{
		printf("Unable to get the usb devices sttaus\n");
		return retval;
	}

	if(((enable == 1) && (devicestatus == 1))
		|| ((enable == 0) && (devicestatus == 0))){
		return 0; //device is already in expected state . do nothing.
	}

	//check usb is used by any othe application
	if(enable == 0)
	{
		if( WriteCmdInAMIVncPipe( CHECK_USB_STATUS ,0 ) < 0) {
			printf("\n Error: Unable to check usb state\n");
		}
		memset(&tv, 0, sizeof(tv));
		time(&tv.tv_sec);
		tv.tv_sec += 5;
		sem_timedwait(&getResponse,&tv);

		/* USB devices are in use so no need to disable*/
		if( isUSBRunning )
			return 0;
	}

	retval =0;
	cmd = (enable == 1) ? USB_ENABLE_ALL_DEVICE : USB_DISABLE_ALL_DEVICE;
	if (ioctl(gUsbDevice, cmd, NULL) < 0)
	{
		printf("Unable to change state of the usb devices\n");
		retval = -1;
	}

	return retval;
}
//Default VNC Server

static enum rfbNewClientAction defaultClient(rfbClientPtr cl)
{
  cl->clientData = (void*)calloc(sizeof(ClientData),1);
  cl->clientGoneHook = clientgone;
  return RFB_CLIENT_ACCEPT;
}

int loadDefaultVNC()
{

  int maxx = 800;
  int maxy = 600;
  rfbScreenInfoPtr rfbScreen = rfbGetScreen(0,NULL,maxx,maxy,8,3,bpp);
  if(!rfbScreen)
    return 0;
  rfbScreen->desktopName = "Default VNC";
  rfbScreen->frameBuffer = (char*)malloc(maxx*maxy*bpp);
  rfbScreen->alwaysShared = TRUE;
//  rfbScreen->ptrAddEvent = doptr;
//  rfbScreen->kbdAddEvent = dokey;
  rfbScreen->newClientHook = defaultClient;
  rfbScreen->httpDir = "../webclients";
  rfbScreen->httpEnableProxyConnect = TRUE;

 initBuffer((unsigned char*)rfbScreen->frameBuffer);
  rfbDrawString(rfbScreen,&radonFont,20,100,"Hello, World!",0xffffff);

  /* This call creates a mask and then a cursor: */
  /* rfbScreen->defaultCursor =
       rfbMakeXCursor(exampleCursorWidth,exampleCursorHeight,exampleCursor,0);
  */

//  MakeRichCursor(rfbScreen);

  /* initialize the server */
  rfbInitServer(rfbScreen);

#ifndef BACKGROUND_LOOP_TEST
#ifdef USE_OWN_LOOP
  {
    int i;
    for(i=0;rfbIsActive(rfbScreen);i++) {
      fprintf(stderr,"%d\r",i);
      rfbProcessEvents(rfbScreen,100000);
    }
  }
#else
  /* this is the blocking event loop, i.e. it never returns */
  /* 40000 are the microseconds to wait on select(), i.e. 0.04 seconds */
  rfbRunEventLoop(rfbScreen,40000,FALSE);
#endif /* OWN LOOP */
#else
#if !defined(LIBVNCSERVER_HAVE_LIBPTHREAD)
#error "I need pthreads for that."
#endif

  /* this is the non-blocking event loop; a background thread is started */
  rfbRunEventLoop(rfbScreen,-1,TRUE);
  fprintf(stderr, "Running background loop...\n");
  /* now we could do some cool things like rendering in idle time */
  while(1) sleep(5); /* render(); */
#endif /* BACKGROUND_LOOP */

  free(rfbScreen->frameBuffer);
  rfbScreenCleanup(rfbScreen);

  return(0);
}

static void initBuffer(unsigned char* buffer)
{
  int i,j;
  for(j=0;j<maxy;++j) {
    for(i=0;i<maxx;++i) {
      buffer[(j*maxx+i)*bpp+0]=(i+j)*128/(maxx+maxy); /* red */
      buffer[(j*maxx+i)*bpp+1]=i*128/maxx; /* green */
      buffer[(j*maxx+i)*bpp+2]=j*256/maxy; /* blue */
    }
    buffer[j*maxx*bpp+0]=0xff;
    buffer[j*maxx*bpp+1]=0xff;
    buffer[j*maxx*bpp+2]=0xff;
    buffer[j*maxx*bpp+3]=0xff;
  }
}


/*frames a rectangle from modified video tiles to reduce video updates*/
int frameRectFromTiles(rectUpdate_t* recUpdate)
{
	int tilecount = 0, count = 0 ,i =0, j=0;
	int x=0,y=0,pos= sizeof( unsigned short );
	int height=0,width=0;
	int posx=0,posy=0;
	int xdiff= 0,hdiff= 0;

	if((tile_info == NULL) || (recUpdate == NULL))
		return tilecount;

	tilecount = *((unsigned short*)tile_info);
	if(tilecount <= 0)
		return tilecount;

	for (i=0;i<tilecount;i++)
	{
		x = (*(tile_info+pos+1));
		y = (*(tile_info+pos));
		posy = y *TILE_HEIGHT;
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
		posx = x *TILE_WIDTH;
		width = posx+TILE_WIDTH;
		height = posy+TILE_HEIGHT;
#elif defined(SOC_AST2400) || defined(SOC_AST2500) ||  ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
		posx = (x-1) *TILE_WIDTH;
		width = TILE_WIDTH;
		height = TILE_HEIGHT;
#endif
		if(0 < count)
		{
#if defined(SOC_AST2400) || defined(SOC_AST2500) ||  ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
			if(count >= maxRectColumn)
			{
				count --;
				break;
			}
#endif
			for (j=0;j<count;j++){
				xdiff = (posx -  recUpdate[j].width);

				//detect next tile also changed?
				if( xdiff <= (TILE_WIDTH*2)){
					hdiff =posy -  recUpdate[j].height;
					//changed tile is not falls with in next two rows
					//so consider to mark it as new rect
					if(hdiff > (TILE_HEIGHT *2)){
						//possible of new rect region
						continue;
					}
					//update already found rect region
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV) || defined(CONFIG_SPX_FEATURE_VIDEO_RVAS)
					recUpdate[j].width = width;
					recUpdate[j].height = height;
#elif defined(SOC_AST2400) || defined(SOC_AST2500) || ( defined(SOC_AST2600) && !defined(CONFIG_SPX_FEATURE_VIDEO_RVAS) )
					recUpdate[j].width = width+TILE_HEIGHT;
					recUpdate[j].height = height+TILE_HEIGHT;
#endif
					break;
				}
			}
		}
		if((0 == count) || (j == count)){
			//New rect region identified
			recUpdate[j].x = posx;
			recUpdate[j].y = posy;
			recUpdate[j].width = width;
			recUpdate[j].height = height;
			count++;
		}

		pos +=2;
	}

	return count;
}

#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
/* Adds password authentication support for nonsecure connections */
void VncPasswordCheck(rfbClientPtr cl)
{
	char *lo = "::ffff:127.0.0.1";
	struct stat statbuff;

	if (cl->host != NULL)
	{
		if (!((strncmp(cl->host, lo, strlen(cl->host)) == 0) ))
		{
			// Not loopback ip. Treat as non-secure connection
			// Add password authendication support
			if (stat(VNC_PASSWORD_FILE, &statbuff) == 0)
			{
				cl->screen->authPasswdData = VNC_PASSWORD_FILE;
				return;
			}
			// VNC_PASSWORD_FILE doesn't exist! clear password (if any)
		}
		// Loopback ip. Treat as secure connection
		// Remove password authendication support
		cl->screen->authPasswdData = NULL;
	}
}
#endif
