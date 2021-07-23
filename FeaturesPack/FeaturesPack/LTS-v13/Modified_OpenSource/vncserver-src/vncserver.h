#ifndef VNC_SERVER_H
#define VNC_SERVER_H
#include <rfb/rfbregion.h>
#include <rfb/rfb.h>


#ifdef __GNUC__
#define PACKED __attribute__ ((packed))
#else
#define PACKED
#pragma pack( 1 )
#endif

#ifdef SOCKET
#undef SOCKET
#endif
#include "hid.h"

/************************Define*******************/
#define PRINT_DLFCN_ERR printf("%s\n",dlerror())
#define FB_FILE 		"/dev/fb0"
#define SET_CMD_IN_PROGRESS "/var/set_kvm_client_in_progress"
//To debug log, change DISABLE to 1
#define DISABLE 0

//vnc HEADER
#define GET_SESSION_MANAGEMENT_STATUS				0x06
#define SESSION_MANAGEMENT_STATUS				0x07
#define SEND_SESSION_INFO					0x08
#define SET_ACTIVE_VNC_SESSION					0x09
#define GET_VNC_CONF						0x01
#define IVTP_KVM_DISCONNECT					(0x0036)
#define RECV_VNC_CONF						0x02
#define GET_KVM_CLIENT_VALUE					0x03
#define GET_POWER_CONS_STATUS					0x04
#define POWER_CONS_STATUS					0x05
#define KVM_CLIENT_STATE					(0xE8)
#define CLEANUP_VNC_SESSION					0x0A
#define DISCONNECT_VNC_SESSION					0x0B
#define SET_ACTIVE_VNC_SESSION_COUNT				0x0C
#define CHECK_USB_STATUS					0x0D
#define USB_STATUS_RESPONSE					0x0E
#define SET_VNC_AS_CLIENT_TYPE					0x0F
#define VNC_TITLE_STRING					"/var/vnc_title_string"
#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
#define VNC_PASSWORD_CMD					0xFF
#define VNC_PASSWORD_FILE					"/conf/vnckey"
#endif

//End of VNC header

#define LIB_VIDEO "/usr/local/lib/libvideo_vnc.so"
#define VNC_CMD_PIPE "/var/vnc_cmd_pipe"
#define VNC_AMI_CMD_PIPE "/var/vnc_ami_cmd_pipe"
#define EMPTY_STRING ""
#define TEXT_LEN 4000
#define VNC_TIMER_TICKLE 5
#define SOCKET int
#define TOKENCFG_MAX_CLIENTIP_LEN			64
#define NEW_LINE 10
#define NON_PRINT_CHAR_RANGE 0x1F
#define BEL 7
#define VNC_SERVER "vncserver"
#define PERMISSION_RWX 0777
#define VNC_SERVER_ID_BIT 8


#define BPP 4
#define VIDEO_ERROR -1
// #define VIDEO_SUCCESS	 0
#define VIDEO_NO_CHANGE  1
#define VIDEO_NO_SIGNAL  2
// #define VIDEO_FULL_SCREEN_CHANGE 3

// x is 2 so that there is some space between left margin of the screen and text.
#define SCREEN_POS_X 2
// y is 10 to have visible line spacing in text.
#define SCREEN_POS_Y 10
#define STOP 1
#define START 0

#define DEFAULT_TITLE_STRING "AMI VNC"
#define VIEW_ONLY "(View Only)"
#define VNC_BIN_PATH "/usr/local/bin/vncserver"
#define ADVISERD "adviserd"


#define VNC_PORT 5900
#define LOCALHOST "localhost"
#define DEFER_VIDEO_UPDATE //video update will be sent to client with delay
// minimum video resolution (on text mode - 640x400)

#define TITLE_STRING_LEN 32/*length*/+1/*null terminate*/
#define VIEW_ONLY_STRING_LEN 12
enum vnc_session_status{UNUSED, ACTIVE, TIMED_OUT, DISCONNECTED, CLEAN_UP};

int g_nonsecureport = 0;
int vncpipe = -1;
int AmiVNCPipe = -1;
u8 g_sync_rec_jviewer = 0;
extern int IsPowerconsumptionmode;

//service

#define SESSION_TYPE_VNC			(14)
#define CONSOLE_TYPE_VNC			(0x10)


#define ENABLED 1

/***************************Struct definations*******************/

struct fb_info_t {
	struct fb_fix_screeninfo fix_screen_info;
	struct fb_var_screeninfo var_screen_info;
	int fd;
	void* fbmem;
};

/* This structure is created so that every client has its own pointer */
typedef struct ClientData {
	rfbBool oldButton;
	int oldx, oldy;
} ClientData;

/***********************Global Variable Declarations***********************/
rfbScreenInfoPtr rfbScreen;

static struct fb_info_t fb_info;
static unsigned short int *fbmmap = MAP_FAILED;
// SERVICE_CONF_STRUCT	vncConf;
typedef unsigned char  INT8U;
typedef unsigned short INT16U;
typedef unsigned int   INT32U;
#ifndef ICC_OS_WINDOWS
typedef char INT8;
#endif

#define MAX_SERVICE_NAME_SIZE       16
#define MAX_SERVICE_IFACE_NAME_SIZE 16
typedef unsigned int		INTU;

#define VNC_CURSOR_PATTERN_SIZE 4096 // Cursor size is 64x64
// Each pixel is represented in 4 values namely (R)ed,(G)reen,(B)lue,(A)lpha
#define VNC_CURSOR_COLOR_SIZE (VNC_CURSOR_PATTERN_SIZE * 4)
#define VNC_SERVICE_NAME            "vnc"
// typedef unsigned int uint32;
// typedef unsigned short TWOBYTES;
// typedef unsigned short UINT16;
// typedef unsigned short WORD;
#if defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2600)
#define VNC_BSOD_CAPTURE  0x0
#define CRASH_SCREEN_FILE_JPEG "/var/bsod/crashscreen.jpeg"
#endif

typedef struct
{
    INT8U   CurrentState;                       /* 1, to enable the service; 0, to disable the service */
    INT32U  NonSecureAccessPort;                        /* Non-secure access port number */
    INT32U  SecureAccessPort;                           /* Secure access port number */                        
    INTU    SessionInactivityTimeout;                   /* Service session inactivity yimeout in seconds*/
    INT8U   MaxAllowSession;                            /* Maximum allowed simultaneous sessions */
    INT32U 	connection_type;
} PACKED vnc_conf_t;


typedef struct
{
        TWOBYTES x_offset;
        TWOBYTES y_offset;
        TWOBYTES width;
        TWOBYTES height;
        BYTE pattern[VNC_CURSOR_PATTERN_SIZE];
        BYTE color[VNC_CURSOR_COLOR_SIZE];
} vnc_cursor_t;

typedef struct rectUpdate_t{
	int x;
	int y;
	int width;
	int height;
}rectUpdate_t ;

typedef struct {
	time_t lastPacketTime;
	int sock;
	int status;
	unsigned int racsession_id;
	char clientIP[INET6_ADDRSTRLEN];
}__attribute__((packed)) VNCSessionInfo;


typedef struct
{
    unsigned short          cmd;
    unsigned long           datalen;
    unsigned short      status;
} __attribute__((packed)) cmd_info_t;

typedef struct
{
	int session_index;
	int reason;
} PACKED vnc_session_index_t;
vnc_session_index_t vnc_session;

typedef struct
{
	cmd_info_t comm_info;
	vnc_session_index_t session;
} PACKED vnc_clean_session_t;


int vnc_max_session = 0;
pthread_mutex_t  *vnc_session_guard;
int seqno;
int m_btn_status;
int g_prv_capture_status = 0;
char *emptyScreen = NULL;
extern int gUsbDevice;
VNCSessionInfo *sessionInfo;
#define ADVISER 1
#define VNC 2

void *pVidHandle = NULL;
// int maxx=800,maxy=600;


static int bpp = 4;
rfbClientPtr pCl = NULL;
pthread_t videoThread = NULL;
pthread_t cursorThread = NULL;
sem_t	mStartCapture;
sem_t	waitForConfUpdate;
sem_t	mStartCursorCapture;
// #define TILE_WIDTH 32

int g_kvm_client_state = -1;
#define HID_INITILIZATION "HID Initilization InProgress.."
#if defined(SOC_PILOT_III) || defined(SOC_PILOT_IV)	
#define ESTABLISHING_VNC_SESSION "Establishing the VNC Session..."
#endif
#if defined(SOC_AST2400) || defined(SOC_AST2500) || defined(SOC_AST2600)
#define VIDEOCAP_JPEG_ENABLE "/proc/ractrends/videocap/jpeg_enable"
#endif

/***********************Function Declarations************************/
#define MASTER_GONE 0
#define MASTER_PRESENT 1
/*video functions*/



void (*set_send_cursor_pattern_flag_sym)(int);

void vncUnloadResource() ;
void initVideo();
int startCapture();
void* updateVideo();//thread function to maintain video update
rfbBool vncrfbProcessEvents(rfbScreenInfoPtr screen,long usec);
void updateHostCursorPattern(vnc_cursor_t *cursor_info);
void *updateCursor();
void showBlankScreen(char *message,int xPos,int yPos);
void getActiveSocketList(SOCKET* active_sock_list);
void adoptResolutionChange(rfbClientPtr cl);
time_t getLastPacketTime (SOCKET fd);
void setActiveSessionCount(int activeSesCnt);
void (*create_vnc_fake_session)();
void *checkTimer();
void vncSignalHandler(int signum);
void* checkIncomingCmd();
void onClientGone(SOCKET* active_sock_list);
void on_vnc_no_session();
int setTimedOut(SOCKET fd, int timedout);
void onStartStopVnc(int mode);
int setLastPacketTime (SOCKET fd, time_t lastPacketTime);
void onStartStopVnc(int mode);
int updateUsbStatus(int enable);
int isCleanUpSession(SOCKET fd);
void select_sleep(time_t sec,suseconds_t usec);
void InitSignals();
int loadDefaultVNC();
int requestKVMClientstate();
int formatString (char* src , char* formattedString);
int send_session_info( int index);
void cleanUpInvalidSession(int sessionIndex, int reason);
int clear_vnc_session( int sessionIndex, int reason);

void getResolution(int *maxx_vnc,int *maxy_vnc);
void closeVncPipe(int fd);
void setResolution(int maxx_vnc, int maxy_vnc);
VNCSessionInfo  *getSessionInfo();
pthread_mutex_t  *getVncSessionGuard();
int init_video_resources(void);
int vnc_capture_cursor(void *cursor_info);
int WriteCmdInAMIVncPipe(int cmd, int status);
static void clientgone(rfbClientPtr cl);
static enum rfbNewClientAction newclient(rfbClientPtr cl);
static void initBuffer(unsigned char* buffer);
#ifdef CONFIG_SPX_FEATURE_VNC_PASSWORD_FOR_NON_SECURE_CONNECTION
void VncPasswordCheck(rfbClientPtr cl);
#endif
#define CREATE_VNC_FAKE_SESSION "create_vnc_fake_session"
#define VNC_CAPTURE_VIDEO "vnc_capture_video"



#endif//AMI_VNC_EXTN
