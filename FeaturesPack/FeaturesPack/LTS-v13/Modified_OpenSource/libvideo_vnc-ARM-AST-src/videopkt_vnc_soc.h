
#ifndef _VIDEOPKT_SOC_H_
#define _VIDEOPKT_SOC_H_

#include <stdint.h>

#define IVTP_HW_CURSOR			(0x1002)
#define IVTP_GET_VIDEO_CONFIG	(0x1003)
#define IVTP_SET_VIDEO_CONFIG	(0x1004)
#define SOC_ID			0x02//AST

typedef struct {
	uint16_t  iEngVersion;
	uint16_t  wHeaderLen;

	// SourceModeInfo
	uint16_t  SourceMode_X;
	uint16_t  SourceMode_Y;
	uint16_t  SourceMode_ColorDepth;
	uint16_t  SourceMode_RefreshRate;
	uint8_t    SourceMode_ModeIndex;

	// DestinationModeInfo
	uint16_t  DestinationMode_X;
	uint16_t  DestinationMode_Y;
	uint16_t  DestinationMode_ColorDepth;
	uint16_t  DestinationMode_RefreshRate;
	uint8_t    DestinationMode_ModeIndex;

	// FRAME_HEADER
	uint32_t   FrameHdr_StartCode;
	uint32_t   FrameHdr_FrameNumber;
	uint16_t  FrameHdr_HSize;
	uint16_t  FrameHdr_VSize;
	uint32_t   FrameHdr_Reserved[2];
	uint8_t    FrameHdr_CompressionMode;
	uint8_t    FrameHdr_JPEGScaleFactor;
	uint8_t    FrameHdr_JPEGTableSelector;
	uint8_t    FrameHdr_JPEGYUVTableMapping;
	uint8_t    FrameHdr_SharpModeSelection;
	uint8_t    FrameHdr_AdvanceTableSelector;
	uint8_t    FrameHdr_AdvanceScaleFactor;
	uint32_t   FrameHdr_NumberOfMB;
	uint8_t    FrameHdr_RC4Enable;
	uint8_t    FrameHdr_RC4Reset;
	uint8_t    FrameHdr_Mode420;

	// INF_DATA
	uint8_t    InfData_DownScalingMethod;
	uint8_t    InfData_DifferentialSetting;
	uint16_t  InfData_AnalogDifferentialThreshold;
	uint16_t  InfData_DigitalDifferentialThreshold;
	uint8_t    InfData_ExternalSignalEnable;
	uint8_t    InfData_AutoMode;
	uint8_t    InfData_VQMode;

	// COMPRESS_DATA
	uint32_t   CompressData_SourceFrameSize;
	uint32_t   CompressData_CompressSize;
	uint32_t   CompressData_HDebug;
	uint32_t   CompressData_VDebug;

	uint8_t    InputSignal;
	uint16_t	Cursor_XPos;
	uint16_t	Cursor_YPos;
} __attribute__ ((packed)) frame_hdr_t;


/* Character device IOCTL's */
#define ASTCAP_MAGIC  		'a'
#define ASTCAP_IOCCMD		_IOWR(ASTCAP_MAGIC, 0, ASTCap_Ioctl)

#define  ASTCAP_IOCTL_RESET_VIDEOENGINE         _IOW('a', 0, int)
#define  ASTCAP_IOCTL_START_CAPTURE             _IOW('a', 1, int)
#define  ASTCAP_IOCTL_STOP_CAPTURE              _IOW('a', 2, int) 
#define  ASTCAP_IOCTL_GET_VIDEO                 _IOR('a', 3, int)
#define  ASTCAP_IOCTL_GET_CURSOR                _IOR('a', 4, int)
#define  ASTCAP_IOCTL_CLEAR_BUFFERS             _IOW('a', 5, int)
#define  ASTCAP_IOCTL_SET_VIDEOENGINE_CONFIGS   _IOW('a', 6, int)
#define  ASTCAP_IOCTL_GET_VIDEOENGINE_CONFIGS   _IOR('a', 7, int)
#define  ASTCAP_IOCTL_SET_SCALAR_CONFIGS        _IOW('a', 8, int)
#define  ASTCAP_IOCTL_ENABLE_VIDEO_DAC          _IOW('a', 9, int)

#define ASTCAP_IOCTL_SUCCESS            _IOR('a', 10, int)
#define ASTCAP_IOCTL_ERROR              _IOR('a', 11, int)
#define ASTCAP_IOCTL_NO_VIDEO_CHANGE    _IOR('a', 12, int)
#define ASTCAP_IOCTL_BLANK_SCREEN       _IOR('a', 13, int)

//video 
#define VIDEO_HDR_BUF_SZ (16 * 1024)
#define VIDEO_DATA_BUF_SZ (4 * 1024 * 1024)
#define VIDEO_BUF_SZ (VIDEO_HDR_BUF_SZ + VIDEO_DATA_BUF_SZ)
#define VIDEO_HDR_VIDEO_SZ (4 * 1024)
#define PERMISSION_RWX 0777

typedef struct {
	int OpCode;
	int ErrCode;
	unsigned long Size;
	void *vPtr;
	unsigned char Reserved [2];
} ASTCap_Ioctl;

void decode_tile_coordinates(unsigned char **tile_info, frame_hdr_t *video_frame_hdr, void *video_buffer);

#endif /* _VIDEO_PKT_SOC_H_ */
