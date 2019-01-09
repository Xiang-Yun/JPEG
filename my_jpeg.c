#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "my_jpeg.h"

//#define PRINT_CODE_RESULT

#pragma warning( disable : 4996 )  

static uint8 bytenew = 0;          // The byte that will be written in the JPG file
static signed char bytepos = 7;    //bit position in the byte we write (bytenew)

static unsigned short int mask[16] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };

// The Huffman tables we'll use:
static bitstring YDC_HT[12];         
static bitstring CbDC_HT[12];
static bitstring YAC_HT[256];
static bitstring CbAC_HT[256];

// 實際值查表
static uint8 *category_alloc;
static uint8 *category;             

// Huffman 查表
static bitstring *bitcode_alloc;
static bitstring *bitcode;

// JPEG串流
static FILE *fp_jpeg_stream;


// Improved gray-scale(IGS) quantization
void IGS(uint8* IGS_I, uint8* origin_img, uint16 Nr, uint16 Nc)
{
	uint8 sum = 0x00;
	for (int x = 0; x < Nr; x++)
		for (int y = 0; y < Nc; y++)
		{
			if ((sum & 0xf0) == (0xf << 4))      // 最左邊4位元已經是1111時，該像素不加sum
				continue;
			else
				sum += origin_img[y + x*Nr];
			
			IGS_I[y + x*Nr] = sum & 0xf0;     // 輸出左邊位元為量化結果
			sum = sum & 0x0f;                 // 最右邊(不重要)4位元設為sum
		}
} // end IGS()

double *DCT_transform(uint8 *originImage, uint16 Nr, uint16 Nc)
{
	double* F = (double*)calloc(Nr*Nc, sizeof(double));   // Nr*Nc塊double類型的記憶體
	double tmp_shift_data;
	double tmp_sum_data;
	
	for (int i = 0; i < Nr; i += 8)
		for (int j = 0; j < Nc; j += 8)
		{
			// DCT
			for (int u = 0; u < 8; u++)
				for (int v = 0; v < 8; v++)
				{
					tmp_sum_data = 0.0;
					for (int x = 0; x < 8; x++)
						for (int y = 0; y < 8; y++)
						{
							// shift -128
							tmp_shift_data = ((double)originImage[(y + j) + (x + i)*Nc]) - 128.0;      // f(x,y) DCT範圍在-128~127
							tmp_sum_data += (double)(tmp_shift_data*cos(((2.0 * x + 1.0)*u*pi) / 16.0)*cos(((2.0 * y + 1.0)*v*pi) / 16.0));
						} // end (x,y)

					double c_u = 1.0, c_v = 1.0;
					if (u == 0) c_u = 1.0 / sqrt(2);
					if (v == 0) c_v = 1.0 / sqrt(2);

					tmp_sum_data *= 0.25*c_u*c_v;
					F[(v + j) + (u + i) * Nc] = tmp_sum_data;
				} // end (u,v)
		}
	return F;
} // end DCT_transform

double *iDCT_transform(double *dct_img, uint16 Nr, uint16 Nc)
{
	double* f = (double*)calloc(Nr*Nc, sizeof(double));   // Nr*Nc塊double類型的記憶體
	double tmp_sum_data;

	// shift+128
	for (int i = 0; i < Nr; i += 8)
		for (int j = 0; j < Nc; j += 8)
			// iDCT
			for (int x = 0; x < 8; x++)
				for (int y = 0; y < 8; y++)
				{
					tmp_sum_data = 0.0;
					for (int u = 0; u < 8; u++)
						for (int v = 0; v < 8; v++)
						{
							double c_u = 1.0, c_v = 1.0;
							if (u == 0) c_u = 1.0 / sqrt(2);
							if (v == 0) c_v = 1.0 / sqrt(2);

							tmp_sum_data += (double)(c_u*c_v*dct_img[(j + v) + (i + u) * Nc] * cos(((2.0 * x + 1.0)*u*pi) / 16.0)*cos(((2.0 * y + 1.0)*v*pi) / 16.0));

						} // end (u,v)
					tmp_sum_data *= 0.25;
					f[(j + y) + (i + x)*Nc] = tmp_sum_data + 128;
				} // end (x,y)
	return f;
}// end iDCT_transform

uint8* double2uint8(double* double_img, uint32 imgSize)
{
	uint8*uint8_image = (uint8*)calloc(imgSize, sizeof(uint8));
	for (uint32 i = 0; i < imgSize; i++)
		uint8_image[i] = (uint8)round(double_img[i]);	
	return uint8_image;
}// end double2uint8()

double * uint82double(uint8* uint8_img, uint32 imgSize)
{
	double*double_image = (double*)calloc(imgSize, sizeof(double));
	for (uint32 i = 0; i < imgSize; i++)
		double_image[i] = (double)uint8_img[i];
	return double_image;
}// end uint82double()

void Quantization(double* dct_img, uint16 Nr, uint16 Nc, int flag)
{
	for (int u = 0; u < Nr; u += 8)
		for (int v = 0; v < Nc; v += 8)
			for (int i = 0; i < 8; i++)
				for (int j = 0; j < 8; j++)
				{
					if (flag == 0)
						dct_img[(v + j) + (u + i) * Nc] = round(dct_img[(v + j) + (u + i) * Nc] / (double)std_luminance_qt[j + i * 8]);    // 壓縮要取round(誤差)
					else
						dct_img[(v + j) + (u + i) * Nc] = round(dct_img[(v + j) + (u + i) * Nc] / (double)std_chrominance_qt[j + i * 8]);  // 壓縮要取round
				}
} // end Quantization()

void iQuantization(double* dct_qu_img, uint16 Nr, uint16 Nc, int flag)
{
	for (int u = 0; u < Nr; u += 8)
		for (int v = 0; v < Nc; v += 8)
			for (int i = 0; i < 8; i++)
				for (int j = 0; j < 8; j++)
				{
					if (flag == 0)
						dct_qu_img[(v + j) + (u + i) * Nc] *= std_luminance_qt[j + i * 8];
					else
						dct_qu_img[(v + j) + (u + i) * Nc] *= std_chrominance_qt[j + i * 8];
				}
					
} // end iQuantization()

uint8* Downsample(uint8 * originImage, uint16 Nr, uint16 Nc)
{
	uint8* dwns_img = (uint8 *)calloc(Nr*Nc / 4, sizeof(uint8));
	int tmp = 0;
	for (int x = 0; x < Nr; x++)
		for (int y = 0; y < Nc; y++)
			if ((x & 1) && (y & 1))    // 奇數的位置
			{
				dwns_img[tmp] = originImage[y + x*Nc];
				tmp++;
			}
	return dwns_img;
} // end Downsample()

double* Downsample_mean(uint8 * originImage, uint16 Nr, uint16 Nc)
{
	double* dwns_img = (double *)calloc(Nr*Nc / 4, sizeof(double));
	double avr;        // 4點平均值
	for (int x = 0; x < Nr; x += 2)
		for (int y = 0; y < Nc; y += 2)
		{
			avr = 0.0;
			for (int i = 0; i < 2; i++)
				for (int j = 0; j < 2; j++)
					avr += (double)originImage[(y + j) + (x + i)*Nr];
			dwns_img[(y / 2) + (x / 2)*(Nr / 2)] = avr / 4.0;
		}
	return dwns_img;
} // end Downsample_mean()

void Filtering_Mask(double* dct_img, uint16 Nr, uint16 Nc)
{
	for (int u = 0; u < Nr; u += 8)
		for (int v = 0; v < Nc; v += 8)
			for (int i = 0; i < 8; i++)
				for (int j = 0; j < 8; j++)
					dct_img[(j + v) + (i + u)*Nc] *= FilterMask[i][j];
} // end Filtering_Mask()

uint8* subtract_img(uint8* img1, uint8* img2, uint32 size)
{
	uint8* subtraction = (uint8*)calloc(size, sizeof(uint8));
	for (int i = 0; i < size; i++)
		subtraction[i] = abs(img1[i] - img2[i]);
	return subtraction;
} // end subtract_img()

uint8* add_img(uint8* img1, uint8* img2, uint16 Nr, uint16 Nc)
{
	uint8* addition = (uint8*)calloc(Nr*Nc, sizeof(uint8));
	for (int i = 0; i < Nr*Nc; i++)
		addition[i] = (uint8)(img1[i] + img2[i]);
	return addition;
}// end add_img()

double get_image_MSE(uint8* originImage, uint8* IDCT, uint32 size)
{
	double mse;
	double tmp = 0.0;
	for (int i = 0; i < size; i++)
			tmp += pow(originImage[i] - IDCT[i], 2);
	mse = tmp / (size);
	return mse;
} // end get_image_MSE()

double PSNR(double mse)
{
	double psnr = 10 * log10(pow(255, 2) / mse);
	return psnr;
}

double* Normalize(uint8* img, uint32 size)
{
	uint8 img_max = MAX(img, size);
	uint8 img_min = MIN(img, size);
	double* norm_img = calloc(size, sizeof(double));
	for (int i = 0; i < size; i++)
		norm_img[i] = (double)(((img[i] - img_min) * 255.0) / (img_max - img_min));
	return norm_img;
} // end Normalize()

uint8 MAX(uint8* arr, int size)
{
	uint8 max_data = 0;
	for (int i = 0; i < size; i++)
		if (arr[i] > max_data)
			max_data = arr[i];
	return max_data;
}// end MAX()

uint8  MIN(uint8* arr, int size)
{
	uint8 min_data = 255;
	for (int i = 0; i < size; i++)
		if (arr[i] < min_data)
			min_data = arr[i];
	return min_data;
}// end MIN()

 // 因為BMP檔案是BGRBGR...(左下到右上)，所以要YCbCr的資料格式要反轉，在寫入JPEG串流中
void RGB2YCbCr(uint8* originImage, uint8* Y, uint8* Cb, uint8* Cr, uint16 Nr, uint16 Nc)
{
	int tmp;
	uint8 b, g, r;
	for (int row = 0; row < Nr; row++)
		for (int col = 0; col < Nc; col++)
		{
			tmp = col + row*Nc;
			r = originImage[tmp * 3];
			g = originImage[tmp * 3 + 1];
			b = originImage[tmp * 3 + 2];
			Y[tmp] = (uint8)(0.2990*(r - g) + g + 0.1140*(b - g));  // Y = 0.299(R-G)+G+0.114(B-G)
			Cb[tmp] = (uint8)(0.5643*(b - Y[tmp]) + 128.0);	        // Cb = 0.5643(B-Y)+128
			Cr[tmp] = (uint8)(0.7133*(r - Y[tmp])) + 128.0;         // Cr = 0.7133(R-Y)+128
		}
} // end RGB2YCbCr()

void YCbCr2RGB(double* Y, double* Cb, double* Cr, uint8* reconstruct_image, uint16 Nr, uint16 Nc)
{
	int tmp, cb, cr;
	uint8 y;
	for (int row = 0; row < Nr; row++)
		for (int col = 0; col < Nc; col++)
		{
			tmp = col + row*Nc;
			y = Y[tmp];
			cb = Cb[tmp] - 128.0;
			cr = Cr[tmp] - 128.0;
			reconstruct_image[tmp * 3] = (uint8)(1.000*y - 0.0009*cb + 1.4017*cr);       // R
			reconstruct_image[tmp * 3 + 1] = (uint8)(1.000*y - 0.3443*cb - 0.7137*cr);   // G
			reconstruct_image[tmp * 3 + 2] = (uint8)(1.000*y + 1.7753*cb - 0.0015*cr);   // B
		}
} // end YCbCr2RGB()

// 4:1:1 抽樣方式，表示4:1的水平取樣，垂直完全取樣
void YCbCr_downsampling(uint8* mcuCb, uint8* mcuCr, uint8* Cb, uint8* Cr, uint16 Nr, uint16 Nc)
{
	int sum_Cb = 0;
	int sum_Cr = 0;
	int tmp = 0;
	for (int i = 0; i < Nr*Nc; i++)
	{
		if (i % 4 == 0)
		{
			mcuCb[tmp] = sum_Cb / 4;    // 1:
			mcuCr[tmp] = sum_Cr / 4;    // 1
			tmp++;
			sum_Cb = sum_Cr = 0;
		}
		sum_Cb += Cb[i];
		sum_Cr += Cr[i];
	}
} // end YCbCr_sampling()

void YCbCr_upsampling(double* Cb, double* Cr, double* mcuCb, double* mcuCr, uint16 Nr, uint16 Nc)
{
	for (int i = 0; i < Nr*Nc / 4; i++)
		for (int j = 0; j < 4; j++)
		{
			Cb[j + i * 4] = mcuCb[i];
			Cr[j + i * 4] = mcuCr[i];
		}
} // end YCbCr_upsampling()

// BGR(左下到右上)->RGB(左上到右下)
uint8* reverse_bgr(uint8* bmp_m, uint16 Nr, uint16 Nc, uint8 depth)
{
	int t = 0;
	if (depth == 8)
	{
		uint8* DUs = (uint8*)calloc(Nr*Nc, sizeof(uint8));
		for (int x = Nr - 1; x >= 0; x--)
			for (int y = 0; y < Nc; y++)
				DUs[t++] = bmp_m[y + x*Nc];
		return DUs;
	}

	else if (depth == 24)
	{
		uint8* DUs = (uint8*)calloc(Nr*Nc * 3, sizeof(uint8));
		for (int x = Nr - 1; x >= 0; x--)
			for (int y = 0; y < Nc; y++)
			{
				int tmp = y + x*Nr;
				DUs[t++] = bmp_m[tmp * 3 + 2];         // r
				DUs[t++] = bmp_m[tmp * 3 + 1];         // g
				DUs[t++] = bmp_m[tmp * 3];             // b
			}
		return DUs;
	}
} // end reverse_bgr()

uint8* reverse_rgb(uint8* jpg_m, uint16 Nr, uint16 Nc, uint8 depth)
{
	int t = 0;
	if (depth == 8)
	{
		uint8* DUs = (uint8*)calloc(Nr*Nc, sizeof(uint8));
		for (int x = Nr - 1; x >= 0; x--)
			for (int y = 0; y < Nc; y++)
				DUs[y + x*Nc] = jpg_m[t++];
		return DUs;
	}

	else if (depth == 24)
	{
		uint8* DUs = (uint8*)calloc(Nr*Nc * 3, sizeof(uint8));
		for (int x = Nr - 1; x >= 0; x--)
			for (int y = 0; y < Nc; y++)
			{
				int tmp = y + x*Nr;
				DUs[tmp * 3 + 2] = jpg_m[t++];         // r
				DUs[tmp * 3 + 1] = jpg_m[t++];         // g
				DUs[tmp * 3] = jpg_m[t++];             // b
			}
		return DUs;
	}
} // end reverse_rgb()


void set_numbers_category_and_bitcode()
{
	signed long int nr;
	signed long int nrlower, nrupper;
	uint8 cat;

	category_alloc = (uint8 *)malloc(65535 * sizeof(uint8));
	if (category_alloc == NULL)
	{
		printf("Not enough memory.\n");
		exit(EXIT_FAILURE);
	}
		
	category = category_alloc + 32767;   //allow negative subscripts

	bitcode_alloc = (bitstring *)malloc(65535 * sizeof(bitstring));
	if (bitcode_alloc == NULL)
	{
		printf("Not enough memory.\n");
		exit(EXIT_FAILURE);
	}

	bitcode = bitcode_alloc + 32767;
	nrlower = 1; nrupper = 2;
	for (cat = 1; cat <= 15; cat++)
	{
		//Positive numbers
		for (nr = nrlower; nr<nrupper; nr++)
		{
			category[nr] = cat;
			bitcode[nr].length = cat;
			bitcode[nr].value = (uint16)nr;
		}
		//Negative numbers
		for (nr = -(nrupper - 1); nr <= -nrlower; nr++)
		{
			category[nr] = cat;
			bitcode[nr].length = cat;
			bitcode[nr].value = (uint16)(nrupper - 1 + nr);
		}
		nrlower <<= 1;
		nrupper <<= 1;
	}
}

void compute_Huffman_table(uint8 *nrcodes, uint8 *std_table, bitstring *HT)
{
	uint8 k, j;
	uint8 pos_in_table;
	uint16 codevalue;
	codevalue = 0; pos_in_table = 0;
	for (k = 1; k <= 16; k++)
	{
		for (j = 1; j <= nrcodes[k]; j++) {
			HT[std_table[pos_in_table]].value = codevalue;
			HT[std_table[pos_in_table]].length = k;
			pos_in_table++;
			codevalue++;
		}
		codevalue *= 2;
	}
} // end compute_Huffman_table

void init_Huffman_tables()
{
	compute_Huffman_table(std_dc_luminance_nrcodes, std_dc_luminance_values, YDC_HT);
	compute_Huffman_table(std_dc_chrominance_nrcodes, std_dc_chrominance_values, CbDC_HT);
	compute_Huffman_table(std_ac_luminance_nrcodes, std_ac_luminance_values, YAC_HT);
	compute_Huffman_table(std_ac_chrominance_nrcodes, std_ac_chrominance_values, CbAC_HT);
}


void set_DHTinfo()
{
	uint8 i;
	DHTinfo.marker = 0xFFC4;
	DHTinfo.length = 0x01A2;
	DHTinfo.HTYDCinfo = 0;
	for (i = 0; i<16; i++)
		DHTinfo.YDC_nrcodes[i] = std_dc_luminance_nrcodes[i + 1];
	for (i = 0; i <= 11; i++)
		DHTinfo.YDC_values[i] = std_dc_luminance_values[i];

	DHTinfo.HTYACinfo = 0x10;
	for (i = 0; i<16; i++)
		DHTinfo.YAC_nrcodes[i] = std_ac_luminance_nrcodes[i + 1];
	for (i = 0; i <= 161; i++)
		DHTinfo.YAC_values[i] = std_ac_luminance_values[i];

	DHTinfo.HTCbDCinfo = 1;
	for (i = 0; i<16; i++)
		DHTinfo.CbDC_nrcodes[i] = std_dc_chrominance_nrcodes[i + 1];
	for (i = 0; i <= 11; i++)
		DHTinfo.CbDC_values[i] = std_dc_chrominance_values[i];

	DHTinfo.HTCbACinfo = 0x11;
	for (i = 0; i<16; i++)
		DHTinfo.CbAC_nrcodes[i] = std_ac_chrominance_nrcodes[i + 1];
	for (i = 0; i <= 161; i++)
		DHTinfo.CbAC_values[i] = std_ac_chrominance_values[i];
}

void set_DQTinfo()
{
	uint8 scalefactor = 50;// scalefactor controls the visual quality of the image
						   // the smaller is, the better image we'll get, and the smaller
						   // compression we'll achieve
	DQTinfo.marker = 0xFFDB;
	DQTinfo.length = 132;
	DQTinfo.QTYinfo = 0;
	DQTinfo.QTCbinfo = 1;
	set_quant_table(std_luminance_qt, scalefactor, DQTinfo.Ytable);
	set_quant_table(std_chrominance_qt, scalefactor, DQTinfo.Cbtable);
}

void set_quant_table(uint8 *basic_table, uint8 scale_factor, uint8 *newtable)
// Set quantization table and zigzag reorder it
{
	uint8 i;
	long temp;
	for (i = 0; i < 64; i++) {
		temp = ((long)basic_table[i] * scale_factor + 50L) / 100L;
		//limit the values to the valid range
		if (temp <= 0L) temp = 1L;
		if (temp > 255L) temp = 255L; //limit to baseline range if requested
		newtable[zigzag[i]] = (uint8)temp;
	}
}

void write_DHTinfo()
{
	uint8 i;
	writeword(DHTinfo.marker);
	writeword(DHTinfo.length);
	writebyte(DHTinfo.HTYDCinfo);
	for (i = 0; i<16; i++)
		writebyte(DHTinfo.YDC_nrcodes[i]);
	for (i = 0; i <= 11; i++)
		writebyte(DHTinfo.YDC_values[i]);

	writebyte(DHTinfo.HTYACinfo);
	for (i = 0; i<16; i++)
		writebyte(DHTinfo.YAC_nrcodes[i]);
	for (i = 0; i <= 161; i++)
		writebyte(DHTinfo.YAC_values[i]);
	writebyte(DHTinfo.HTCbDCinfo);

	for (i = 0; i<16; i++)
		writebyte(DHTinfo.CbDC_nrcodes[i]);
	for (i = 0; i <= 11; i++)
		writebyte(DHTinfo.CbDC_values[i]);

	writebyte(DHTinfo.HTCbACinfo);
	for (i = 0; i<16; i++)
		writebyte(DHTinfo.CbAC_nrcodes[i]);
	for (i = 0; i <= 161; i++)
		writebyte(DHTinfo.CbAC_values[i]);
}

void write_DQTinfo()
{
	uint8 i;
	writeword(DQTinfo.marker);
	writeword(DQTinfo.length);

	writebyte(DQTinfo.QTYinfo);

	for (i = 0; i < 64; i++)
		writebyte(DQTinfo.Ytable[i]);		

	writebyte(DQTinfo.QTCbinfo);

	for (i = 0; i < 64; i++)
		writebyte(DQTinfo.Cbtable[i]);

}

void write_APP0info()
//Nothing to overwrite for APP0info
{
	writeword(APP0info.marker);
	writeword(APP0info.length);
	writebyte('J'); writebyte('F'); writebyte('I'); writebyte('F'); writebyte(0);
	writebyte(APP0info.versionhi); writebyte(APP0info.versionlo);
	writebyte(APP0info.xyunits);
	writeword(APP0info.xdensity); writeword(APP0info.ydensity);
	writebyte(APP0info.thumbnwidth); writebyte(APP0info.thumbnheight);
}

void write_SOF0info()
// We should overwrite width and height
{
	writeword(SOF0info.marker);
	writeword(SOF0info.length);
	writebyte(SOF0info.precision);
	writeword(SOF0info.height); writeword(SOF0info.width);
	writebyte(SOF0info.nrofcomponents);
	writebyte(SOF0info.IdY); writebyte(SOF0info.HVY); writebyte(SOF0info.QTY);
	writebyte(SOF0info.IdCb); writebyte(SOF0info.HVCb); writebyte(SOF0info.QTCb);
	writebyte(SOF0info.IdCr); writebyte(SOF0info.HVCr); writebyte(SOF0info.QTCr);
}

void write_SOSinfo()
//Nothing to overwrite for SOSinfo
{
	writeword(SOSinfo.marker);
	writeword(SOSinfo.length);
	writebyte(SOSinfo.nrofcomponents);
	writebyte(SOSinfo.IdY); writebyte(SOSinfo.HTY);
	writebyte(SOSinfo.IdCb); writebyte(SOSinfo.HTCb);
	writebyte(SOSinfo.IdCr); writebyte(SOSinfo.HTCr);
	writebyte(SOSinfo.Ss); writebyte(SOSinfo.Se); writebyte(SOSinfo.Bf);
}

void init_all()
{
	set_DQTinfo();
	set_DHTinfo();
	init_Huffman_tables();
	set_numbers_category_and_bitcode();
	/*precalculate_YCbCr_tables();
	prepare_quant_tables();*/
}

//void precalculate_YCbCr_tables()
//{
//	unsigned short int R, G, B;
//	for (R = 0; R <= 255; R++) {
//		YRtab[R] = (signed long int)(65536 * 0.299 + 0.5)*R;
//		CbRtab[R] = (signed long int)(65536 * -0.16874 + 0.5)*R;
//		CrRtab[R] = (signed long int)(32768)*R;
//	}
//	for (G = 0; G <= 255; G++) {
//		YGtab[G] = (signed long int)(65536 * 0.587 + 0.5)*G;
//		CbGtab[G] = (signed long int)(65536 * -0.33126 + 0.5)*G;
//		CrGtab[G] = (signed long int)(65536 * -0.41869 + 0.5)*G;
//	}
//	for (B = 0; B <= 255; B++) {
//		YBtab[B] = (signed long int)(65536 * 0.114 + 0.5)*B;
//		CbBtab[B] = (signed long int)(32768)*B;
//		CrBtab[B] = (signed long int)(65536 * -0.08131 + 0.5)*B;
//	}
//}
//
//
//void prepare_quant_tables()
//{
//	double aanscalefactor[8] = { 1.0, 1.387039845, 1.306562965, 1.175875602,
//		1.0, 0.785694958, 0.541196100, 0.275899379 };
//	uint8 row, col;
//	uint8 i = 0;
//	for (row = 0; row < 8; row++)
//	{
//		for (col = 0; col < 8; col++)
//		{
//			fdtbl_Y[i] = (float)(1.0 / ((double)DQTinfo.Ytable[zigzag[i]] *
//				aanscalefactor[row] * aanscalefactor[col] * 8.0));
//			fdtbl_Cb[i] = (float)(1.0 / ((double)DQTinfo.Cbtable[zigzag[i]] *
//				aanscalefactor[row] * aanscalefactor[col] * 8.0));
//
//			i++;
//		}
//	}
//}


signed char* double2char(double* double_img, uint32 imgSize)
{
	signed char* char_img = (signed char*)calloc(imgSize, sizeof(signed char));
	for (int i = 0; i < imgSize; i++)
		char_img[i] = (signed char)double_img[i];	
	return char_img;
} // end double2char()

void writebits(bitstring bs)
// A portable version; it should be done in assembler
{
	unsigned short int value;
	signed char posval;     //bit position in the bitstring we read, should be<=15 and >=0
	value = bs.value;
	posval = bs.length - 1;

	while (posval >= 0)
	{
		if (value & mask[posval])
			bytenew |= mask[bytepos];
		posval--; 
		bytepos--;

		if (bytepos<0)
		{
#ifdef PRINT_CODE_RESULT
			printf("bytenew = %d\n", bytenew);
#endif
			if (bytenew == 0xFF)
			{
				writebyte(0xFF);
				writebyte(0);
			}
			else
				writebyte(bytenew);
			
			bytepos = 7; bytenew = 0;
		}
	}
} // end writebits()

void processMCU(signed char* DCT_qu, signed short int* DC, bitstring *HTDC, bitstring *HTAC, uint16 startx, uint16 starty, uint16 Nc)
{
	signed short int diff;

	bitstring EOB = HTAC[0x00];
	bitstring M16zeroes = HTAC[0xF0];
	uint8 i;
	uint8 startpos;
	uint8 end0pos;
	uint8 nrzeroes;
	uint8 nrmarker;
	signed short int* MCU = (signed short int*)calloc(8 * 8, sizeof(signed short int));

	// zigzag reorder
	for (int x = 0; x < 8; x++)
		for (int y = 0; y < 8; y++)
			MCU[zigzag[y + x * 8]] = DCT_qu[(starty + y) + (startx + x)*Nc];

	//------------------------------DC係數編碼---------------------------------//

	// DPCM
	diff = MCU[0] - *DC;
	*DC = MCU[0];

	if (diff == 0)
		writebits(HTDC[0]);

	// DC Huffman
	else
	{
#ifdef PRINT_CODE_RESULT
		printf("\nDiffDC value = %d\n", diff);
		printf("DiffDC Huffman code length = %d\n", HTDC[category[diff]].length);  // 4       Huffman: code word length
		printf("DiffDC Huffman code word = %d\n", HTDC[category[diff]].value);  // 14       Huffman: code word
		printf("DiffDC bit length = %d\n", bitcode[diff].length);  //6                    DiffDC bit length
		printf("DiffDC value = %d\n", bitcode[diff].value);   //20                   Diff value(負數反轉過後的)
#endif
		writebits(HTDC[category[diff]]);     // write Huffman code word length and code word
		writebits(bitcode[diff]);            // write DiffDC bit length and value
	}

	//------------------------------AC係數編碼---------------------------------//

	// RLE(Run size位置)
	for (end0pos = 63; (end0pos>0) && (MCU[end0pos] == 0); end0pos--);
	// end0pos = first element in reverse order !=0
	if (end0pos == 0) { writebits(EOB); return; }

	// RLE + AC Huffman編碼
	i = 1;
	while (i <= end0pos)
	{
		startpos = i;
		for (; (MCU[i] == 0) && (i <= end0pos); i++);
		nrzeroes = i - startpos;
		if (nrzeroes >= 16)
		{
			for (nrmarker = 1; nrmarker <= nrzeroes / 16; nrmarker++)
				writebits(M16zeroes);
			nrzeroes = nrzeroes % 16;
		}
#ifdef PRINT_CODE_RESULT
		printf("\nz[%d] = %d\n", i, MCU[i]);            // MCU[2]=249
		printf("AC Huffman code length = %d\n", HTAC[nrzeroes * 16 + category[MCU[i]]].length);   // 10
		printf("AC Huffman code word = %d\n", HTAC[nrzeroes * 16 + category[MCU[i]]].value);    // 1014
		printf("zigzag bit length = %d\n", bitcode[MCU[i]].length);           // 8
		printf("zigzag value = %d\n", bitcode[MCU[i]].value);            // 249
#endif
		writebits(HTAC[nrzeroes * 16 + category[MCU[i]]]);      // write Huffman code word length and code word
		writebits(bitcode[MCU[i]]);                             // write zig-zag value bit length and value

		i++;
	}
	if (end0pos != 63)
		writebits(EOB);
} // end processDU()

void entropy_code(signed char* DCT_qu_Y, signed char* DCT_qu_Cb, signed char* DCT_qu_Cr, uint16 Nr, uint16 Nc)
{
	signed short int DCY = 0, DCCb = 0, DCCr = 0;  //DC coefficients used for differential encoding
	uint16 x, y;
	for (x = 0; x < Nr; x += 8)
		for (y = 0; y < Nc; y += 8)
		{
			processMCU(DCT_qu_Y, &DCY, YDC_HT, YAC_HT, x, y, Nc);
			processMCU(DCT_qu_Cb, &DCCb, CbDC_HT, CbAC_HT, x, y, Nc);
			processMCU(DCT_qu_Cr, &DCCr, CbDC_HT, CbAC_HT, x, y, Nc);
		}
} // end entropy_code()

void SaveJpgFile(char* fileName, signed char* DCT_qu_Y, signed char* DCT_qu_Cb, signed char* DCT_qu_Cr, uint16 Nr, uint16 Nc)
{
	bitstring fillbits;     //filling bitstring for the bit alignment of the EOI marker

	fp_jpeg_stream = fopen(fileName, "wb");
	init_all();

	SOF0info.width = Nc;
	SOF0info.height = Nr;

	writeword(0xFFD8);    //SOI
	write_APP0info();
	write_DQTinfo();
	write_SOF0info();
	write_DHTinfo();
	write_SOSinfo();

	bytenew = 0; bytepos = 7;
	entropy_code(DCT_qu_Y, DCT_qu_Cb, DCT_qu_Cr, Nr, Nc);

	//Do the bit alignment of the EOI marker
	if (bytepos >= 0)
	{
		fillbits.length = bytepos + 1;
		fillbits.value = (1 << (bytepos + 1)) - 1;
		writebits(fillbits);
	}

	writeword(0xFFD9);   //EOI

	free(category_alloc);
	free(bitcode_alloc);
	fclose(fp_jpeg_stream);
} // end SaveJpgFile()
