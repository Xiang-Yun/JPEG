#ifndef MY_BMP_H
#define MY_BMP_H
#define _CRT_SECURE_NO_WARNINGS


typedef unsigned char uint8;        // 1bytes
typedef unsigned short uint16;      // 2bytes
typedef unsigned int uint32;		// 4bytes
typedef unsigned long long uint64;  // 8bytes
typedef unsigned int bool;
#define false 0
#define true 1

struct bmpheader
{
	//char c1;
	//char c2;
	uint32 FileSize;         // file size in bytes, dword (4 bytes)
	uint32 Reserve;          // reserved for later use
	uint32 BitmapDataOffset; // Bitmap Data Offset, beginning of data
	uint32 BitmapHeaderSize; // Bitmap Header Size, size of bmp info
	uint32 width;            // horizontal width in pixels
	uint32 height;           // vertical height in pixels
	uint16 planes;              // number of planes in bitmap
	uint16 bits;                // bits per pixel
	uint32 Compression;      // compression mode
	uint32 BitmapDataSize;   // Bitmap Data Size, size of data
	uint32 H_Rosolution;     // horizontal resolution, pixels per meter
	uint32 U_Rosolution;     // vertical resolution
	uint32 UsedColorsSize;   // number of colors used
	uint32 ImportantColors;  // important colors
};

uint8 *Load_bmp(char* fileName, uint16 *Nr, uint16 *Nc, uint8 *depth);
void Save_bmp_24bit(char* fileName, uint8* m, uint16 Nr, uint16 Nc);
void Save_bmp_8bit(char* fileName, uint8* m, uint16 Nr, uint16 Nc);
void Print_BMP_Header(struct bmpheader h);

void Get_gray_level_histogram(uint8* img_data, uint32* graylevel, uint16 Nr, uint16 Nc);
uint8* Power_law(uint8* gray, double C, double gamma, uint16 Nr, uint16 Nc);
void Save_gray_level_histogram(char* fileName, uint32* graylevel);


#endif