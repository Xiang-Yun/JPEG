#include "my_bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define _CRT_SECURE_NO_WARNINGS

uint8 *Load_bmp(char *fileName, uint16 *Nr, uint16 *Nc, uint8 *depth)
{
	int padding;
	int i = 0;
	char temp;
	struct bmpheader header;

	FILE* f_r = fopen(fileName, "rb");
	if (f_r == NULL) return 1;

	fseek(f_r, 2, SEEK_SET);    // id
	fread((char*)&header, sizeof(uint8), sizeof(header), f_r);

	*Nc = header.width;
	*Nr = header.height;
	*depth = header.bits;

	// move offset to rgb_raw_data_offset to get RGB raw data
	fseek(f_r, header.BitmapDataOffset, SEEK_SET);

	//calculate padding bytes in one row (COL數目要是4的倍數，因為bgrbgr....有缺的話都要補0)
	padding = ((header.bits*header.width + 31) / 32) * 4 - ((header.bits / 8)*header.width);

	uint8* img_data = (uint8*)malloc(header.FileSize * sizeof(uint8*));

	// image data(RGB)
	if (*depth == 24)
		for (int row = 0; row < header.height; row++)
		{
			for (int col = 0; col < header.width; col++)
				for (int c = 0; c < 3; c++)
				{
					fread((char*)&temp, sizeof(temp), sizeof(temp), f_r);
					img_data[i] = temp;
					i++;
				}
			fseek(f_r, padding, SEEK_CUR);
		}
	// image data(gray)
	else
		fread(img_data, sizeof(uint8), header.FileSize, f_r);

	fclose(f_r);
	Print_BMP_Header(header);
	return img_data;
} // end Load_bmp()

void Save_bmp_24bit(char* fileName, uint8* m, uint16 Nr, uint16 Nc)
{
	char i = 'B', d = 'M';
	int row, col, padding;
	int j = 0, tmp = 0;
	unsigned char b, g, r;
	struct bmpheader header;
	int byte_per_pixel = 3;

	FILE* f_w = fopen(fileName, "wb");
	if (f_w == 0) printf("Error open!\n");

	//raw data offset
	header.BitmapDataOffset = 54;   // 1074
	header.BitmapHeaderSize = 40;
	header.height = Nr;
	header.width = Nc;
	header.planes = 1;
	header.bits = 24;
	header.Compression = 0;
	header.BitmapDataSize = Nr*Nc * byte_per_pixel;
	header.H_Rosolution = 0;
	header.U_Rosolution = 0;
	header.UsedColorsSize = 0;
	header.ImportantColors = 0;

	// file size
	header.FileSize = Nr * Nc * byte_per_pixel + header.BitmapDataOffset;
	header.Reserve = 0;

	// BM id header
	fwrite((char*)&i, sizeof(i), sizeof(i), f_w);
	fwrite((char*)&d, sizeof(d), sizeof(d), f_w);

	// rest of header
	fwrite((char *)&header, sizeof(uint8), sizeof(header), f_w);

	//calculate padding bytes in one row
	padding = ((header.bits*header.width + 31) / 32) * 4 - ((header.bits / 8)*header.width);

	// data stored bottom up, left to right
	for (row = 0; row < header.height; row++)
	{
		for (col = 0; col<header.width; col++)
		{
			tmp = row*Nc + col;
			b = m[tmp * 3];
			g = m[tmp * 3 + 1];
			r = m[tmp * 3 + 2];
			fwrite((char*)&b, sizeof(b), sizeof(b), f_w);
			fwrite((char*)&g, sizeof(g), sizeof(g), f_w);
			fwrite((char*)&r, sizeof(r), sizeof(r), f_w);
		}
		//write padding bytes
		for (int k = 0; k < padding; ++k)
			fputs('\0', f_w);
	}
	fclose(f_w);
	//printf("Save %s succeed!\n", fileName);
} // end Save_bmp_24bit()

void Save_bmp_8bit(char* fileName, uint8* m, uint16 Nr, uint16 Nc)
{
	char i = 'B', d = 'M', temp;
	int row, col, padding;
	int j = 0;
	struct bmpheader header;
	int byte_per_pixel = 1;
	unsigned char Palette[256 * 4];
	int nbpal;

	FILE* f_w = fopen(fileName, "wb");
	if (f_w == 0) printf("Error open!\n");

	//raw data offset
	header.BitmapDataOffset = 1074;   // 1074
	header.BitmapHeaderSize = 40;
	header.height = Nr;
	header.width = Nc;
	header.planes = 1;
	header.bits = 8;
	header.Compression = 0;
	header.BitmapDataSize = Nr*Nc;
	header.H_Rosolution = 0;
	header.U_Rosolution = 0;
	header.UsedColorsSize = 0;
	header.ImportantColors = 0;

	// file size
	header.FileSize = Nr * Nc * byte_per_pixel + header.BitmapDataOffset;
	header.Reserve = 0;

	// BM id header
	fwrite((char*)&i, sizeof(i), sizeof(i), f_w);
	fwrite((char*)&d, sizeof(d), sizeof(d), f_w);

	// rest of header
	fwrite((char *)&header, sizeof(uint8), sizeof(header), f_w);

	//Palette
	for (nbpal = 0; nbpal<256; nbpal++)
	{
		Palette[4 * nbpal + 0] = nbpal;
		Palette[4 * nbpal + 1] = nbpal;
		Palette[4 * nbpal + 2] = nbpal;
		Palette[4 * nbpal + 3] = 0;
	}

	fwrite((char*)&Palette, sizeof(uint8), sizeof(Palette), f_w);

	//calculate padding bytes in one row
	padding = ((header.bits*header.width + 31) / 32) * 4 - ((header.bits / 8)*header.width);

	// data stored bottom up, left to right
	for (row = 0; row < header.height; row++)
	{
		for (col = 0; col<header.width; col++)
		{
			temp = m[j];
			fwrite((char*)&temp, sizeof(temp), sizeof(temp), f_w);
			j++;

		}
		//write padding bytes
		for (int k = 0; k < padding; ++k)
			fputs('\0', f_w);
	}
	fclose(f_w);
	//printf("Save %s succeed!\n", fileName);
}// end Save_bmp_8bit

void Print_BMP_Header(struct bmpheader h)
{
	printf("File Size          = %d\n", h.FileSize);
	printf("Width              = %d\n", h.width);
	printf("Height             = %d\n", h.height);
	printf("Bits per Pixel     = %d\n", h.bits);
	printf("Compression        = %d\n", h.Compression);
	printf("BitmapDataOffset   = %d\n", h.BitmapDataOffset);
	printf("BitmapDataSize     = %d\n", h.BitmapDataSize);
	printf("------------------------------------\n\n");
} // end Print_BMP_Header

void Get_gray_level_histogram(uint8* img_data, uint32* graylevel, uint16 Nr, uint16 Nc)
{
	for (int i = 0; i < 256; i++)
		*(graylevel + i) = 0;               //初始化全為0
	for (int row = 0; row < Nr; row++)
		for (int col = 0; col < Nc; col++)
			(*(graylevel + *(img_data + col + row*Nr)))++;       // 像素值對應graylevel記憶體空間的位置
} // end Get_gray_level_histogram()

void Save_gray_level_histogram(char* fileName, uint32* graylevel)
{
	FILE* f_w = fopen(fileName, "wb");
	if (f_w == 0) printf("Error open!\n");

	for (int i = 0; i < 256; i++)
		fprintf(f_w, "%d,\r\n", graylevel[i]);


	printf("build %s succeed!\n", fileName);
	fclose(f_w);
}

uint8* Power_law(uint8* gray, double C, double gamma, uint16 Nr, uint16 Nc)
{
	uint8* new_gray = (uint8*)calloc(Nr*Nc, sizeof(uint8));
	for (int i = 0; i < Nr*Nc; i++)
		new_gray[i] = (uint8)C*pow(gray[i] / 255.0, gamma)*255.0;
	return new_gray;
} // end Power_law()
