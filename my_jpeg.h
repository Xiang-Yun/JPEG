#ifndef MY_JPEG_H
#define MY_JPEG_H
#define _CRT_SECURE_NO_WARNINGS

typedef unsigned char uint8;        // 1byte
typedef unsigned short int uint16;      // 2bytes
typedef unsigned int uint32;		// 4bytes
typedef unsigned long long uint64;  // 8bytes
typedef unsigned int bool;



#define false 0
#define true 1
#define pi (3.14159265358979323846264338327950288)

#define writebyte(b) fputc((b),fp_jpeg_stream)
#define writeword(w) writebyte((w)/256);writebyte((w)%256);


typedef struct
{
	uint8 length;
	uint16 value;
} bitstring;



// frame geader
static struct  SOF0infotype {
	uint16 marker; // = 0xFFC0
	uint16 length; // = 17 for a truecolor YCbCr JPG
	uint8 precision;// Should be 8: 8 bits/sample
	uint16 height;
	uint16 width;
	uint8 nrofcomponents;//Should be 3: We encode a truecolor JPG(彩色)
	uint8 IdY;  // = 1
	uint8 HVY; // sampling factors for Y (bit 0-3 vert., 4-7 hor.)
	uint8 QTY;  // Quantization Table number for Y = 0
	uint8 IdCb; // = 2
	uint8 HVCb;       // 全部0x11表示沒有downsampling
	uint8 QTCb; // 1
	uint8 IdCr; // = 3
	uint8 HVCr;
	uint8 QTCr; // Normally equal to QTCb = 1
} SOF0info = { 0xFFC0,17,8,0,0,3,1,0x11,0,2,0x11,1,3,0x11,1 };
// Default sampling factors are 1,1 for every image component: No downsampling


// Scan Header
static struct SOSinfotype {
	uint16 marker;  // = 0xFFDA
	uint16 length; // = 12
	uint8 nrofcomponents; // Should be 3: truecolor JPG
	uint8 IdY; //1
	uint8 HTY; //0 // bits 0..3: AC table (0..3)
			  // bits 4..7: DC table (0..3)
	uint8 IdCb; //2
	uint8 HTCb; //0x11
	uint8 IdCr; //3
	uint8 HTCr; //0x11
	uint8 Ss, Se, Bf; // not interesting, they should be 0,63,0
} SOSinfo = { 0xFFDA,12,3,1,0,2,0x11,3,0x11,0,0x3F,0 };


static struct DQTinfotype {
	uint16 marker;  // = 0xFFDB
	uint16 length;  // = 132
	uint8 QTYinfo;// = 0:  bit 0..3: number of QT = 0 (table for Y)
				  //       bit 4..7: precision of QT, 0 = 8 bit
	uint8 Ytable[64];
	uint8 QTCbinfo; // = 1 (quantization table for Cb,Cr}
	uint8 Cbtable[64];
} DQTinfo;
// Ytable from DQTinfo should be equal to a scaled and zizag reordered version
// of the table which can be found in "tables.h": std_luminance_qt
// Cbtable , similar = std_chrominance_qt
// We'll init them in the program using set_DQTinfo function


static struct DHTinfotype {
	uint16 marker;  // = 0xFFC4
	uint16 length;  //0x01A2
	uint8 HTYDCinfo; // bit 0..3: number of HT (0..3), for Y =0
					 //bit 4  :type of HT, 0 = DC table,1 = AC table
					 //bit 5..7: not used, must be 0
	uint8 YDC_nrcodes[16]; //at index i = nr of codes with length i
	uint8 YDC_values[12];
	uint8 HTYACinfo; // = 0x10
	uint8 YAC_nrcodes[16];
	uint8 YAC_values[162];//we'll use the standard Huffman tables
	uint8 HTCbDCinfo; // = 1
	uint8 CbDC_nrcodes[16];
	uint8 CbDC_values[12];
	uint8 HTCbACinfo; //  = 0x11
	uint8 CbAC_nrcodes[16];
	uint8 CbAC_values[162];
} DHTinfo;

static struct APP0infotype {
	uint16 marker;// = 0xFFE0
	uint16 length; // = 16 for usual JPEG, no thumbnail
	uint8 JFIFsignature[5]; // = "JFIF",'\0'
	uint8 versionhi; // 1
	uint8 versionlo; // 1
	uint8 xyunits;   // 0 = no units, normal density
	uint16 xdensity;  // 1
	uint16 ydensity;  // 1
	uint8 thumbnwidth; // 0
	uint8 thumbnheight; // 0
} APP0info = { 0xFFE0,16,'J','F','I','F',0,1,1,0,1,1,0,0 };


// 亮度量化表
static uint8 std_luminance_qt[64] =
{
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  56,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99
};

// 色度量化表
static uint8 std_chrominance_qt[64] =
{
	17,  18,  24,  47,  99,  99,  99,  99,
	18,  21,  26,  66,  99,  99,  99,  99,
	24,  26,  56,  99,  99,  99,  99,  99,
	47,  66,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99
};

// ZigZag編碼
static uint8 zigzag[64] =
{
	0,   1,   5,  6,   14,  15,  27,  28,
	2,   4,   7,  13,  16,  26,  29,  42,
	3,   8,  12,  17,  25,  30,  41,  43,
	9,   11, 18,  24,  31,  40,  44,  53,
	10,  19, 23,  32,  39,  45,  52,  54,
	20,  22, 33,  38,  46,  51,  55,  60,
	21,  34, 37,  47,  50,  56,  59,  61,
	35,  36, 48,  49,  57,  58,  62,  63
};

static uint8 FilterMask[8][8] =
{
	{ 1,1,1,0,0,0,0,0 },
	{ 1,1,0,0,0,0,0,0 },
	{ 1,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0 },
};

static uint8 std_dc_luminance_nrcodes[17] = { 0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static uint8 std_dc_luminance_values[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static uint8 std_dc_chrominance_nrcodes[17] = { 0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static uint8 std_dc_chrominance_values[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static uint8 std_ac_luminance_nrcodes[17] = { 0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static uint8 std_ac_luminance_values[162] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

static uint8 std_ac_chrominance_nrcodes[17] = { 0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
static uint8 std_ac_chrominance_values[162] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };



void IGS(uint8* IGS_I, uint8* origin_img, uint16 Nr, uint16 Nc);

double *DCT_transform(uint8 *originImage, uint16 Nr, uint16 Nc);
double *iDCT_transform(double *dct_img, uint16 Nr, uint16 Nc);
uint8 * double2uint8(double* double_img, uint32 imgSize);
double * uint82double(uint8* uint8_img, uint32 imgSize);
void Quantization(double* dct_img, uint16 Nr, uint16 Nc, int flag);
void iQuantization(double* dct_qu_img, uint16 Nr, uint16 Nc, int flag);


uint8* Downsample(uint8 * originImage, uint16 Nr, uint16 Nc);
double* Downsample_mean(uint8 * originImage, uint16 Nr, uint16 Nc);
void Filtering_Mask(double* dct_img, uint16 Nr, uint16 Nc);

uint8* subtract_img(uint8* img1, uint8* img2, uint32 size);
uint8* add_img(uint8* img1, uint8* img2, uint16 Nr, uint16 Nc);
double get_image_MSE(uint8* originImage, uint8* IDCT, uint32 size);
double PSNR(double mse);

double* Normalize(uint8* img, uint32 size);
uint8 MAX(uint8* arr, int size);
uint8 MIN(uint8* arr, int size);

void RGB2YCbCr(uint8* originImage, uint8* Y, uint8* Cb, uint8* Cr, uint16 Nr, uint16 Nc);
void YCbCr2RGB(double* Y, double* Cb, double* Cr, uint8* reconstruct_image, uint16 Nr, uint16 Nc);
void YCbCr_downsampling(uint8* mcuCb, uint8* mcuCr, uint8* Cb, uint8* Cr, uint16 Nr, uint16 Nc);
void YCbCr_upsampling(double* Cb, double* Cr, double* mcuCb, double* mcuCr, uint16 Nr, uint16 Nc);
uint8* reverse_bgr(uint8* bmp_m, uint16 Nr, uint16 Nc, uint8 depth);
uint8* reverse_rgb(uint8* jpg_m, uint16 Nr, uint16 Nc, uint8 depth);

void set_numbers_category_and_bitcode();
void compute_Huffman_table(uint8 *nrcodes, uint8 *std_table, bitstring *HT);
void init_Huffman_tables();

// 寫入JPEG檔案
void set_DHTinfo();
void set_DQTinfo();
void write_DHTinfo();
void write_DQTinfo();
void write_APP0info();
void write_SOF0info();
void write_SOSinfo();

//void prepare_quant_tables();
//void precalculate_YCbCr_tables();
void set_quant_table(uint8 *basic_table, uint8 scale_factor, uint8 *newtable);
void init_all();

signed char* double2char(double* double_img, uint32 imgSize);
void writebits(bitstring bs);
void processMCU(signed char* DCT_qu, signed short int* DC, bitstring *HTDC, bitstring *HTAC, uint16 startx, uint16 starty, uint16 Nc);
void entropy_code(signed char* DCT_qu_Y, signed char* DCT_qu_Cb, signed char* DCT_qu_Cr, uint16 Nr, uint16 Nc);
void SaveJpgFile(char* fileName, signed char* DCT_qu_Y, signed char* DCT_qu_Cb, signed char* DCT_qu_Cr, uint16 Nr, uint16 Nc);

#endif