

#ifndef VQ_H
#define VQ_H

#define MAXCODE 1024
#define MAXRUN 255
#define MASK 0XFF
#define BMPHEADERSIZE 0x36
//#define WIDTH 1024
//#define HEIGHT 768
#define HBLOCK 16
#define VBLOCK 16

#define VQCODESIZE 64
#define RBIT 2
#define GBIT 2
#define BBIT 2
#define GRAYBIT 3

#define EASYENCODE 1
#define AVERAGEVQ 1
#define GRAY 0
#define SNAKE 0
#define BIT8ONLY 0
#define YUVMODE 1

#define YLEVEL 5
#define YBIT 3
#define UVLEVEL 32
#define UVBIT 5
#define INNERUV 10
#define OUTERUV 21
#define UV_separate 0

/* Gamma=2.0
#define GAMMACODE7 245
#define GAMMACODE6 225
#define GAMMACODE5 203
#define GAMMACODE4 180
#define GAMMACODE3 152
#define GAMMACODE2 117
#define GAMMACODE1 67
#define GAMMACODEA 232
#define GAMMACODEB 180
#define GAMMACODEC 103
*/

/* Gamma=1.0 */
#define GAMMACODE7 236
#define GAMMACODE6 200
#define GAMMACODE5 163
#define GAMMACODE4 127
#define GAMMACODE3 91
#define GAMMACODE2 54
#define GAMMACODE1 18
#define GAMMACODEA 212
#define GAMMACODEB 127
#define GAMMACODEC 42


#define GAMMA 1.0

#define COMPRESS_HEADER_SIZE 16


/* for decoding */
#define NO_LEAF 0x0
#define LEFT_LEAF 0x1
#define RIGHT_LEAF 0x2

#define VQ_BLOCK_START_CODE   0x0
#define JPEG_BLOCK_START_CODE 0x1
#define VQ_BLOCK_SKIP_CODE    0x2
#define JPEG_BLOCK_SKIP_CODE  0x3
#define BLOCK_START_LENGTH    0x2
#define BLOCK_START_MASK      0x3

#define BLOCK_HEADER_S_MASK           0x01
#define BLOCK_HEADER_MASK             0x0F
#define VQ_HEADER_MASK                0x01
#define VQ_NO_UPDATE_HEADER           0x00
#define VQ_UPDATE_HEADER              0x01
#define VQ_NO_UPDATE_LENGTH           0x03
#define VQ_UPDATE_LENGTH              0x1B
#define VQ_INDEX_MASK                 0x03
#define VQ_COLOR_MASK                 0xFFFFFF
#define JPEG_NO_SKIP_CODE             0x00
#define LOW_JPEG_NO_SKIP_CODE         0x04
#define LOW_JPEG_SKIP_CODE            0x0C
#define JPEG_SKIP_CODE                0x08
#define FRAME_END_CODE                0x09
#define JPEG_NO_SKIP_PASS2_CODE		  0x02
#define JPEG_SKIP_PASS2_CODE		  0x0A
#define VQ_NO_SKIP_1_COLOR_CODE       0x05
#define VQ_NO_SKIP_2_COLOR_CODE       0x06
#define VQ_NO_SKIP_4_COLOR_CODE       0x07
#define VQ_SKIP_1_COLOR_CODE          0x0D
#define VQ_SKIP_2_COLOR_CODE          0x0E
#define VQ_SKIP_4_COLOR_CODE          0x0F
#define BLOCK_AST2100_START_LENGTH    0x04
#define BLOCK_AST2100_SKIP_LENGTH     20    // S:1 H:3 X:8 Y:8

struct RGB1 {
	unsigned char B;
	unsigned char G;
	unsigned char R;
};

struct YUV {
	unsigned char U;
	unsigned char Y;
	unsigned char V;
};

struct COMPRESSHEADER {
        int codesize;
        int width;
        int height;
        int modes;
        int bwidth;
        int bheight;
        int nd0;
        int nd1;
        int cmptype;
        int qfactor;
        int qfactoruv;
        int nd11;
        int nd12;
        int nd13;
        int nd14;
        int nd15;
};
#endif //VQ_H
