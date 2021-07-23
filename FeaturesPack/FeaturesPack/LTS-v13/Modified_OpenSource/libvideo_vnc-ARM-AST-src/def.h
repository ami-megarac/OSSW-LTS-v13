

#ifndef __DEF_H__
#define __DEF_H__

#define    PCI                      1
#define    PCIE                     2
#define    AGP                      3
#define    ACTIVE                   4
#define    PASS                     0

#define    FAIL                     1
#define    FIX                      0
#define    STEP                     1
#define    SEQ_ADDRESS_REGISTER     0x3C4
#define    SEQ_DATA_REGISTER        0x3C5
#define    CRTC_ADDRESS_REGISTER    0x3D4
#define    CRTC_DATA_REGISTER       0x3D5
#define    DAC_INDEX_REGISTER       0x3C8
#define    DAC_DATA_REGISTER        0x3C9
//PCI information define
#define    HOST_BRIDGE_MIN                             0x80000000
#define    HOST_BRIDGE_MAX                             0x80060000
#define    PCI_SLOT_STEP                               0x800
#define    AGP_SLOT_MIN                                0x80010000
#define    AGP_SLOT_MAX                                0x8001F800
#define    PCI_ADDR_PORT                               0x0CF8
#define    PCI_DATA_PORT                               0x0CFC
#define    REG_VENENDER_ID_DEVICE_ID                   0
#define    REG_PCI_STATUS_COMMAND                      0x04
#define    REG_PCI_CLASS_REVISION_ID                   0x08
#define    REG_CAPABILITY_POINTER                      0x34
#define    PhysicalBase_Offset                         0x10
#define    MMIO_Offset                                 0x14
#define    RelocateIO_Offset                           0x18

//  Registers
#define    VIDEOBASE_OFFSET                            0x10000
#define    KEY_CONTROL                                 0x00 + VIDEOBASE_OFFSET
#define    VIDEOENGINE_SEQUENCE_CONTROL                0x04 + VIDEOBASE_OFFSET
#define    VIDEOENGINE_PASS1_CONTROL                   0x08 + VIDEOBASE_OFFSET
#define    VIDEOENGINE_MODEDETECTIONSETTING_H          0x0C + VIDEOBASE_OFFSET
#define    VIDEOENGINE_MODEDETECTIONSETTING_V          0x10 + VIDEOBASE_OFFSET
#define    SCALE_FACTOR_REGISTER                       0x14 + VIDEOBASE_OFFSET
#define    SCALING_FILTER_PARAMETERS_1                 0x18 + VIDEOBASE_OFFSET
#define    SCALING_FILTER_PARAMETERS_2                 0x1C + VIDEOBASE_OFFSET
#define    SCALING_FILTER_PARAMETERS_3                 0x20 + VIDEOBASE_OFFSET
#define    SCALING_FILTER_PARAMETERS_4                 0x24 + VIDEOBASE_OFFSET
#define    MODEDETECTION_STATUS_READBACK               0x28 + VIDEOBASE_OFFSET
#define    VIDEOPROCESSING_CONTROL                     0x2C + VIDEOBASE_OFFSET
#define    VIDEO_PROCESS_WINDOW_SETTING                0x30 + VIDEOBASE_OFFSET
#define    VIDEO_COMPRESS_WINDOW_SETTING               0x34 + VIDEOBASE_OFFSET
#define    VIDEO_IN_BUFFER_BASEADDRESS                 0x38 + VIDEOBASE_OFFSET
#define    VIDEO_IN_BUFFER_OFFSET                      0x3C + VIDEOBASE_OFFSET
#define    VIDEOPROCESS_BUFFER_BASEADDRESS             0x40 + VIDEOBASE_OFFSET 
#define    VIDEOCOMPRESS_SOURCE_BUFFER_BASEADDRESS     0x44 + VIDEOBASE_OFFSET
#define    VIDEOPROCESS_OFFSET                         0x48 + VIDEOBASE_OFFSET
#define    VIDEOPROCESS_REFERENCE_BUFFER_BASEADDRESS   0x4C + VIDEOBASE_OFFSET
#define    FLAG_BUFFER_BASEADDRESS                     0x50 + VIDEOBASE_OFFSET
#define    VIDEO_COMPRESS_DESTINATION_BASEADDRESS      0x54 + VIDEOBASE_OFFSET
#define    DOWN_SCALE_LINES_REGISTER                   0x58 + VIDEOBASE_OFFSET
#define    VIDEO_CAPTURE_BOUND_REGISTER                0x5C + VIDEOBASE_OFFSET
#define    VIDEO_COMPRESS_CONTROL                      0x60 + VIDEOBASE_OFFSET
#define    BLOCK_SHARPNESS_DETECTION_CONTROL           0x64 + VIDEOBASE_OFFSET
#define    POST_WRITE_BUFFER_DRAM_THRESHOLD            0x68 + VIDEOBASE_OFFSET
#define    VIDEO_INTERRUPT_CONTROL                     0x6C + VIDEOBASE_OFFSET
#define    COMPRESS_DATA_COUNT_REGISTER                0x70 + VIDEOBASE_OFFSET
#define    COMPRESS_BLOCK_COUNT_REGISTER               0x74 + VIDEOBASE_OFFSET
#define    VQ_Y_VECTOR_CODE_REGISTER_START             0x80 + VIDEOBASE_OFFSET
#define    VQ_U_VECTOR_CODE_REGISTER_START             0x90 + VIDEOBASE_OFFSET
#define    VQ_V_VECTOR_CODE_REGISTER_START             0xB0 + VIDEOBASE_OFFSET
#define    U_SUM_HISTOGRAM                             0x500 + VIDEOBASE_OFFSET
#define    U_COUNT_HISTOGRAM                           0x480 + VIDEOBASE_OFFSET
#define    V_SUM_HISTOGRAM                             0x400 + VIDEOBASE_OFFSET
#define    V_COUNT_HISTOGRAM                           0x480 + VIDEOBASE_OFFSET
#define    Y_SUM_HISTOGRAM                             0x580 + VIDEOBASE_OFFSET
#define    Y_COUNT_HISTOGRAM                           0x5C0 + VIDEOBASE_OFFSET
#define    RC4KEYS_REGISTER                            0x600 + VIDEOBASE_OFFSET
#define    VQHUFFMAN_TABLE_REGISTER                    0x300 + VIDEOBASE_OFFSET


//  Parameters
/*
#define    SAMPLE_RATE                                 12000000.0
#define    MODEDETECTION_VERTICAL_STABLE_MAXIMUM       0x4
#define    MODEDETECTION_HORIZONTAL_STABLE_MAXIMUM     0x4
#define    MODEDETECTION_VERTICAL_STABLE_THRESHOLD     0x4
#define    MODEDETECTION_HORIZONTAL_STABLE_THRESHOLD   0x8
*/
#define    HORIZONTAL_SCALING_FILTER_PARAMETERS_LOW    0xFFFFFFFF
#define    HORIZONTAL_SCALING_FILTER_PARAMETERS_HIGH   0xFFFFFFFF
#define    VIDEO_WRITE_BACK_BUFFER_THRESHOLD_LOW       0x08
#define    VIDEO_WRITE_BACK_BUFFER_THRESHOLD_HIGH      0x04
#define    VQ_Y_LEVELS                                 0x10
#define    VQ_UV_LEVELS                                0x05
#define    EXTERNAL_VIDEO_HSYNC_POLARITY               0x01
#define    EXTERNAL_VIDEO_VSYNC_POLARITY               0x01
#define    VIDEO_SOURCE_FROM                           0x01
#define    EXTERNAL_ANALOG_SOURCE                      0x01
#define    USE_INTERNAL_TIMING_GENERATOR               0x01
#define    WRITE_DATA_FORMAT                           0x00
#define    SET_BCD_TO_WHOLE_FRAME                      0x01
#define    ENABLE_VERTICAL_DOWN_SCALING                0x01
#define    BCD_TOLERENCE                               0xFF
#define    BCD_START_BLOCK_XY                          0x0
#define    BCD_END_BLOCK_XY                            0x3FFF
#define    COLOR_DEPTH                                 16
#define    BLOCK_SHARPNESS_DETECTION_HIGH_THRESHOLD    0xFF
#define    BLOCK_SHARPNESS_DETECTION_LOE_THRESHOLD     0xFF
#define    BLOCK_SHARPNESS_DETECTION_HIGH_COUNTS_THRESHOLD   0x3F
#define    BLOCK_SHARPNESS_DETECTION_LOW_COUNTS_THRESHOLD    0x1F
#define    VQTABLE_AUTO_GENERATE_BY_HARDWARE           0x0
#define    VQTABLE_SELECTION                           0x0
#define    JPEG_COMPRESS_ONLY                          0x0
#define    DUAL_MODE_COMPRESS                          0x1
#define    BSD_H_AND_V                                 0x0
#define    ENABLE_RC4_ENCRYPTION                       0x1
#define    BSD_ENABLE_HIGH_THRESHOLD_CHECK             0x0
#define    VIDEO_PROCESS_AUTO_TRIGGER                  0x0
#define    VIDEO_COMPRESS_AUTO_TRIGGER                 0x0
#define    VIDEO_IN_BUFFER_ADDRESS                     0x800000
#define    VIDEO_PROCESS_BUFFER_ADDRESS                0x200000
#define    VIDEO_REFERENCE_BUFFER_ADDRESS              0xC00000
#define    VIDEO_COMPRESS_SOURCE_BUFFER_ADDRESS        0x200000
#define    VIDEO_COMPRESS_DESTINATION_BUFFER_ADDRESS   0x3C00000
#define    VIDEO_FLAG_BUFFER_ADDRESS                   0xF00000


#define    ReadMemoryBYTE(baseaddress,offset)        *(volatile BYTE *)((unsigned long)(baseaddress)+(unsigned long)(offset))
#define    ReadMemoryLong(baseaddress,offset)        *(volatile unsigned long *)((unsigned long)(baseaddress)+(unsigned long)(offset))
#define    ReadMemoryShort(baseaddress,offset)       *(volatile unsigned short *)((unsigned long)(baseaddress)+(unsigned long)(offset))
#define    WriteMemoryBYTE(baseaddress,offset,data)  *(volatile BYTE *)((unsigned long)(baseaddress)+(unsigned long)(offset)) = (BYTE)(data)
#define    WriteMemoryLong(baseaddress,offset,data)  *(volatile unsigned long *)((unsigned long)(baseaddress)+(unsigned long)(offset))=(unsigned long)(data)
#define    WriteMemoryShort(baseaddress,offset,data) *(volatile unsigned short *)((unsigned long)(baseaddress)+(unsigned long)(offset))=(unsigned short)(data)
#define    WriteMemoryLongWithANDData(baseaddress, offset, anddata, data)  *(volatile unsigned long *)((unsigned long)(baseaddress)+(unsigned long)(offset)) = ((*(volatile unsigned long *)((unsigned long)(baseaddress)+(unsigned long)(offset))) & (unsigned long)(anddata)) | (unsigned long)(data)

#define    intfunc      int386

/*
#ifndef  Linux
#define    outdwport         outpd
#define    indwport          inpd
#define    outport           outp
#define    inport            inp
#endif
*/

/*
#ifdef   Linux
#include <sys/io.h>
#include <linux/pci_ids.h>
#define    u8                unsigned char
#define    u16               unsigned short
#define    u32               unsigned int

#define    outdwport(p,v)    outl((u32)(v),(u16)(p))
#define    indwport(p)       inl((u16)(p))
#define    outport(p,v)      outb((u8)(v),(u16)(p))
#define    inport(p)         inb((u16)(p))
#endif
*/

#ifndef  _STRUCTURE_INFO
#define  _STRUCTURE_INFO

//Buffer structure
typedef struct _XMS_BUFFER
{
    unsigned long     handle;
    unsigned long     size;
    unsigned long     physical_address;
    unsigned long     virtual_address;
} XMS_BUFFER, *PXMS_BUFFER;

//PCI info structure
typedef struct
{
    unsigned short    usVendorID;
    unsigned short    usDeviceID;
    unsigned long     ulPCIConfigurationBaseAddress;
    BYTE      jAGPStatusPort;
    BYTE      jAGPCommandPort;
    BYTE      jAGPVersion;
} PCI_INFO;

typedef struct _DEVICE_PCI_INFO
{
    unsigned short    usVendorID;
    unsigned short    usDeviceID;
    unsigned long     ulPCIConfigurationBaseAddress;
    unsigned long     ulPhysicalBaseAddress;
    unsigned long     ulMMIOBaseAddress;
    unsigned short    usRelocateIO;
} DEVICE_PCI_INFO, *PDEVICE_PCI_INFO;

typedef struct _VIDEO_MODE_INFO
{
    unsigned short    X;
    unsigned short    Y;
    unsigned short    ColorDepth;
    unsigned short    RefreshRate;
    BYTE      ModeIndex;
} VIDEO_MODE_INFO, *PVIDEO_MODE_INFO;

typedef struct _VQ_INFO {
    BYTE    Y[16];
    BYTE    U[32];
    BYTE    V[32];
    BYTE    NumberOfY;
    BYTE    NumberOfUV;
    BYTE    NumberOfInner;
    BYTE    NumberOfOuter;
} VQ_INFO, *PVQ_INFO;

typedef struct _HUFFMAN_TABLE {
    unsigned long  HuffmanCode[32];
} HUFFMAN_TABLE, *PHUFFMAN_TABLE;

typedef struct _FRAME_HEADER {
    unsigned long     StartCode;
    unsigned long     FrameNumber;
    unsigned short    HSize;
    unsigned short    VSize;
    unsigned long     Reserved[2];
    BYTE      CompressionMode;
    BYTE      JPEGScaleFactor;
    BYTE      JPEGTableSelector;
    BYTE      JPEGYUVTableMapping;
    BYTE      SharpModeSelection;
    BYTE      AdvanceTableSelector;
    BYTE      AdvanceScaleFactor;
    unsigned long     NumberOfMB;
    //BYTE      VQ_YLevel;
    //BYTE      VQ_UVLevel;
    //VQ_INFO   VQVectors;
    BYTE      RC4Enable;
    BYTE      RC4Reset;
    BYTE      Mode420;
} FRAME_HEADER, *PFRAME_HEADER;

typedef struct _INF_DATA {
    BYTE    DownScalingMethod;
    BYTE    DifferentialSetting;
    unsigned short  AnalogDifferentialThreshold;
    unsigned short  DigitalDifferentialThreshold;
    BYTE    ExternalSignalEnable;
    BYTE    AutoMode;
    BYTE    DisableHWCursor;
    BYTE    VQMode;
} INF_DATA, *PINF_DATA;

typedef struct _TIME_DATA {
    unsigned long    UnitTimeLow;
    unsigned long    UnitTimeHigh;
    unsigned long    StartTimeLow;
    unsigned long    StartTimeHigh;
    unsigned long    TimeUsed;
} TIME_DATA, *PTIME_DATA;

typedef struct _COMPRESS_DATA {
    unsigned long   SourceFrameSize;
    unsigned long   CompressSize;
    unsigned long   HDebug;
    unsigned long   VDebug;
} COMPRESS_DATA, *PCOMPRESS_DATA;

//VIDEO Engine Info
typedef struct _VIDEO_ENGINE_INFO {
    unsigned short             iEngVersion;
    XMS_BUFFER         VideoCompressBuffer;
    XMS_BUFFER         VideoOutputBuffer;
    VIDEO_MODE_INFO    SourceModeInfo;
    VIDEO_MODE_INFO    DestinationModeInfo;
    FRAME_HEADER       FrameHeader;
    INF_DATA           INFData;
    COMPRESS_DATA      CompressData;
} VIDEO_ENGINE_INFO, *PVIDEO_ENGINE_INFO;

/*typedef struct {
    unsigned short    HorizontalTotal;
    unsigned short    VerticalTotal;
    unsigned short    HorizontalActive;
    unsigned short    VerticalActive;
    BYTE      RefreshRate;
    double    HorizontalFrequency;
    unsigned short    HSyncTime;
    unsigned short    HBackPorch;
    unsigned short    VSyncTime;
    unsigned short    VBackPorch;
    unsigned short    HLeftBorder;
    unsigned short    HRightBorder;
    unsigned short    VBottomBorder;
    unsigned short    VTopBorder;
} VESA_MODE;*/

typedef struct {
    unsigned short    HorizontalActive;
    unsigned short    VerticalActive;
    unsigned short    RefreshRate;
    BYTE      ADCIndex1;
    BYTE      ADCIndex2;
    BYTE      ADCIndex3;
    BYTE      ADCIndex5;
    BYTE      ADCIndex6;
    BYTE      ADCIndex7;
    BYTE      ADCIndex8;
    BYTE      ADCIndex9;
    BYTE      ADCIndexA;
    BYTE      ADCIndexF;
    BYTE      ADCIndex15;
    int       HorizontalShift;
    int       VerticalShift;
} ADC_MODE;


struct COLOR_CACHE {
	unsigned long Color[4];
	unsigned char Index[4];
	unsigned char BitMapBits;
};

struct RGB {
	unsigned char B;
	unsigned char G;
	unsigned char R;
	unsigned char Reserved;
};

struct YUV444 {
	unsigned char U;
	unsigned char Y;
	unsigned char V;
};

struct YUV422 {
	unsigned char Y0;
	unsigned char U;
	unsigned char Y1;
	unsigned char V;
};

//  RC4 structure
struct rc4_state
{
    int x;
    int y;
    int m[256];
};


// Function prototypes - Parts
void  GetINFData (PVIDEO_ENGINE_INFO VideoEngineInfo);
unsigned long TestVideo (PVIDEO_ENGINE_INFO VideoEngineInfo);
void createweighting(int oldDim, int newDim, int taps, int method, int WeightMat[16][8],double g_enlargeFactor,double g_shrinkFactor);
void  Outdwm(unsigned long    ulLinearBaseAddress, unsigned long    ulOffset, unsigned long    ulLength, unsigned long    ulData);
unsigned long  IndwmBankMode(unsigned long    ulLinearBaseAddress, unsigned long    ulOffset);
void  OutdwmBankMode(unsigned long    ulLinearBaseAddress, unsigned long    ulOffset, unsigned long    ulLength, unsigned long    ulData);
unsigned long  Indwm(unsigned long    ulLinearBaseAddress, unsigned long    ulOffset);

#ifdef __cplusplus
	extern "C"{
#endif
void init_jpg_table ();
unsigned long decode(PVIDEO_ENGINE_INFO VideoEngineInfo);
#ifdef __cplusplus
    }
#endif

void init_jpg_table ();

#define VIDEOD_PORT                 (7578)

#pragma pack(1)
typedef struct
{
    unsigned short  iEngVersion;
    unsigned short  wHeaderLen;

    //  SourceModeInfo
    unsigned short  SourceMode_X;
    unsigned short  SourceMode_Y;
    unsigned short  SourceMode_ColorDepth;
    unsigned short  SourceMode_RefreshRate;
    BYTE    SourceMode_ModeIndex;

    //  DestinationModeInfo
    unsigned short  DestinationMode_X;
    unsigned short  DestinationMode_Y;
    unsigned short  DestinationMode_ColorDepth;
    unsigned short  DestinationMode_RefreshRate;
    BYTE    DestinationMode_ModeIndex;

    //  FRAME_HEADER
    unsigned long   FrameHdr_StartCode;
    unsigned long   FrameHdr_FrameNumber;
    unsigned short  FrameHdr_HSize;
    unsigned short  FrameHdr_VSize;
    unsigned long   FrameHdr_Reserved[2];
    BYTE    FrameHdr_CompressionMode;
    BYTE    FrameHdr_JPEGScaleFactor;
    BYTE    FrameHdr_JPEGTableSelector;
    BYTE    FrameHdr_JPEGYUVTableMapping;
    BYTE    FrameHdr_SharpModeSelection;
    BYTE    FrameHdr_AdvanceTableSelector;
    BYTE    FrameHdr_AdvanceScaleFactor;
    unsigned long   FrameHdr_NumberOfMB;
    BYTE    FrameHdr_RC4Enable;
    BYTE    FrameHdr_RC4Reset;
    BYTE    FrameHdr_Mode420;

    //  INF_DATA
    BYTE    InfData_DownScalingMethod;
    BYTE    InfData_DifferentialSetting;
    unsigned short  InfData_AnalogDifferentialThreshold;
    unsigned short  InfData_DigitalDifferentialThreshold;
    BYTE    InfData_ExternalSignalEnable;
    BYTE    InfData_AutoMode;
    BYTE    InfData_VQMode;

    //  COMPRESS_DATA
    unsigned long   CompressData_SourceFrameSize;
    unsigned long   CompressData_CompressSize;
    unsigned long   CompressData_HDebug;
    unsigned long   CompressData_VDebug;

    BYTE    InputSignal;
	unsigned short	Cursor_XPos;
	unsigned short	Cursor_YPos;

} ASP2000_IMG_HEADER;
#define ASP2000_IMG_HEADER_SIZE     (sizeof (ASP2000_IMG_HEADER))

typedef struct
{
    //BYTE    b1Reserved[28];
    BYTE    b1Reserved[19];     /* Parts Hack   */
    BYTE    bSignature [8];		/**< "ASP-2000" signature. */
    unsigned short  wHeaderLen;			/**< header length. */
    unsigned long   dwDataLen;			/**< data length. */
    unsigned short  wCommand;			/**< command number. */
    unsigned short  wStatus;			/**< should be VIDEOD_ERROR. */
    BYTE    bReserved[2];

} VISERV_HEADER;


//////////////////////////////////////////////////////////

#define VISERV_HEADER_SIZE			(sizeof(VISERV_HEADER))
#pragma pack()


#define CLIENT_BUFFER_ALLOC_SIZE   ((1600*1200*3) + VISERV_HEADER_SIZE + ASP2000_IMG_HEADER_SIZE)
/*--------------------------------------------------------------
	List of Commands used between the
	Client & the Video Server
----------------------------------------------------------------*/
#define VISERV_GET_CHALLENGE        (0x01)
#define VISERV_LOGIN                (0x02)
#define VISERV_START_REDIRECTION    (0x03)
#define VISERV_TAKE_SCREEN          (0x04)
#define VISERV_STOP_REDIRECTION     (0x05)
#define VISERV_GET_DEVCAPS          (0x06)
#define VISERV_GET_PORTCAPS         (0x07)
#define VISERV_GET_NUM_OF_ACTIVE_CLIENTS    (0x08)
#define VISERV_DISCONNECT           (0x09)
#define VIDCMD_PALETTE_INFO         (0x0a)
#define VISERV_HEART_BEAT           (0x0b)
#define VISERV_ENGINE_CFG           (0x0c)
#define VISERV_HOST_SCREEN_RES      (0x0d)
#define VISERV_GET_SCREEN           (0x0e)
#define VISERV_MAX_BYTES_PER_SEC    (0x0f)
#define VISERV_PING                 (0x10)
#define VISERV_ALOVICA_PARAM        (0x11)
#define VISERV_FRAME_RATE           (0x12)

#endif
#endif  /*  __DEF_H__   */
