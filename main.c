#include <stdio.h>
#include <stdlib.h>
#include <direct.h> 
#include "my_bmp.h"
#include "my_jpeg.h"

#define _CRT_SECURE_NO_WARNINGS

#define FILENAME "lena.bmp"

/*
JPEG壓縮流程：

1. 色系變換
2. 正向離散餘弦轉換
3. 量化
4. ZigZag 編碼
5. RLE 編碼
6. Huffman 編碼

*/

#define RUN_ENCODER
#define RUN_DECODER
#define RUN_IGS_ENCODER



int main()
{
	_mkdir("result");
	_mkdir("result/DCT_iDCT");
	_mkdir("result/Qu_iQu");
	_mkdir("result/subtract");

	//_mkdir("result/down_sample");
	_mkdir("result/power_law");

	uint16 nr, nc;
	uint8 depth;
	uint8 * origin;
	origin = Load_bmp(FILENAME, &nr, &nc, &depth);
	origin = reverse_bgr(origin, nr, nc, depth);

#ifdef RUN_ENCODER

	/* 一、色彩轉換 */  
	uint8* Y = (uint8*)calloc(nr*nc, sizeof(uint8));      // 0~255
	uint8* Cb = (uint8*)calloc(nr*nc, sizeof(uint8));     // [-128, 127]+128
	uint8* Cr = (uint8*)calloc(nr*nc, sizeof(uint8));     // [-128, 127]+128
	uint32 Y_graylevel[256];

	RGB2YCbCr(origin, Y, Cb, Cr, nr, nc);                      // RGB->YCbCr
	Get_gray_level_histogram(Y, Y_graylevel, nr, nc);       // histogram
	Save_gray_level_histogram("result/power_law/Y_graylevel.txt", Y_graylevel);
	Save_bmp_8bit("result/power_law/Y.bmp", reverse_rgb(Y, nr, nc, 8), nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/Y.bmp", reverse_rgb(Y, nr, nc, 8), nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/Cb.bmp", reverse_rgb(Cb, nr, nc, 8), nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/Cr.bmp", reverse_rgb(Cr, nr, nc, 8), nr, nc);
	 
	// power-Law Tranformation
	// C=1.0, gamma=0.5
	double C = 1.0;
	double gamma = 0.5;
	uint32 Y_05_graylevel[256];
	uint8* Y_05_gray = (uint8*)calloc(nr*nc, sizeof(uint8));

	Y_05_gray = Power_law(Y, C, gamma, nr, nc);                       // power law
	Get_gray_level_histogram(Y_05_gray, Y_05_graylevel, nr, nc);         // histogram
	Save_gray_level_histogram("result/power_law/Y_05_graylevel.txt", Y_05_graylevel);
	Save_bmp_8bit("result/power_law/Y_05.bmp", reverse_rgb(Y_05_gray, nr, nc, 8), nr, nc);


	// C=1.0, gamma=2.0
	gamma = 2.0;
	uint32 Y_20_graylevel[256];
	uint8* Y_20_gray = (uint8*)calloc(nr*nc, sizeof(uint8));

	Y_20_gray = Power_law(Y, C, gamma, nr, nc);                       // power law
	Get_gray_level_histogram(Y_20_gray, Y_20_graylevel, nr, nc);         // histogram
	Save_gray_level_histogram("result/power_law/Y_20_graylevel.txt", Y_20_graylevel);
	Save_bmp_8bit("result/power_law/Y_20.bmp", reverse_rgb(Y_20_gray, nr, nc, 8), nr, nc);


	/* 二、4:4:4抽樣模式 */   
	
	/* 三、shift-128 + Discrete Cosine Transform */
	// Y
	double * DCT_Y = (double*)calloc(nr*nc, sizeof(double));
	DCT_Y = DCT_transform(Y, nr, nc);

	// Cb
	double * DCT_Cb = (double*)calloc(nr*nc, sizeof(double));
	DCT_Cb = DCT_transform(Cb, nr, nc);

	// Cr
	double * DCT_Cr = (double*)calloc(nr*nc, sizeof(double));
	DCT_Cr = DCT_transform(Cr, nr, nc);


	/* 四、量化 */
	// 小於0的值變為0(黑)
	// Y
	Quantization(DCT_Y, nr, nc, 0);             // DCT->Qu
	Save_bmp_8bit("result/Qu_iQu/DCT_QuY.bmp", reverse_rgb(double2uint8(DCT_Y, nr*nc), nr, nc, 8), nr, nc);

	// Cb
	Quantization(DCT_Cb, nr, nc, 1);           // DCT->Qu
	Save_bmp_8bit("result/Qu_iQu/DCT_QuCb.bmp", reverse_rgb(double2uint8(DCT_Cb, nr*nc), nr, nc, 8), nr, nc);
	
	// Cr
	Quantization(DCT_Cr, nr, nc, 1);           // DCT->Qu
	Save_bmp_8bit("result/Qu_iQu/DCT_QuCr.bmp", reverse_rgb(double2uint8(DCT_Cr, nr*nc), nr, nc, 8), nr, nc);


	/* 五、編碼 */
	// DCT經過量化後->產生的範圍在-128~128 
	signed char* DCT_YDUs = (signed char*)calloc(nr*nc, sizeof(signed char));
	signed char* DCT_CbDUs = (signed char*)calloc(nr*nc, sizeof(signed char));
	signed char* DCT_CrDUs = (signed char*)calloc(nr*nc, sizeof(signed char));

	DCT_YDUs = double2char(DCT_Y, nr*nc);          
	DCT_CbDUs = double2char(DCT_Cb, nr*nc);
	DCT_CrDUs = double2char(DCT_Cr, nr*nc);


	// DC_霍夫曼編碼 + AC_RLE編碼 + AC_霍夫曼編碼
	SaveJpgFile("result/result.jpg", DCT_YDUs, DCT_CbDUs, DCT_CrDUs, nr, nc);

#endif


#ifdef RUN_IGS_ENCODER

	/*  Improved gray-scale(IGS) quantization */
	uint8* IGS_Y = (uint8*)calloc(nr*nc, sizeof(uint8));
	uint32 IGS_Y_graylevel[256];
	IGS(IGS_Y, Y, nr, nc);      //   IGS
	Get_gray_level_histogram(IGS_Y, IGS_Y_graylevel, nr, nc);         // histogram
	Save_gray_level_histogram("result/power_law/IGS_Y_graylevel.txt", IGS_Y_graylevel);
	Save_bmp_8bit("result/DCT_iDCT/IGS_Y.bmp", reverse_rgb(IGS_Y, nr, nc, 8), nr, nc);
	Save_bmp_8bit("result/power_law/IGS_Y.bmp", reverse_rgb(IGS_Y, nr, nc, 8), nr, nc);


	double * DCT_IGS = (double*)calloc(nr*nc, sizeof(double));    // DCT
	signed char* DCT_IGS_DUs = (signed char*)calloc(nr*nc, sizeof(signed char));

	DCT_IGS = DCT_transform(IGS_Y, nr, nc);
	Quantization(DCT_IGS, nr, nc, 0);     // 量化
	DCT_IGS_DUs = double2char(DCT_IGS, nr*nc);
	SaveJpgFile("result/test_IGS.jpg", DCT_IGS_DUs, DCT_CbDUs, DCT_CrDUs, nr, nc);

#endif

/*------------------------------------------------------------------------------------*/

#ifdef RUN_DECODER

	/* 一、解碼 */

	/* 二、反量化 */ 
	
	// Y
	iQuantization(DCT_Y, nr, nc, 0);

	// Cb
	iQuantization(DCT_Cb, nr, nc, 1);

	// Cr
	iQuantization(DCT_Cr, nr, nc, 1);


	/* 三、Inverse Discrete Cosine Transform + shift+128 */
	double* iDCT_Y = (double*)calloc(nr*nc, sizeof(double));
	double* iDCT_Cb = (double*)calloc(nr*nc, sizeof(double));   
	double* iDCT_Cr = (double*)calloc(nr*nc, sizeof(double));

	iDCT_Y = iDCT_transform(DCT_Y, nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/iDCT_Y.bmp", reverse_rgb(double2uint8(iDCT_Y, nr*nc), nr, nc, 8), nr, nc);
	iDCT_Cb = iDCT_transform(DCT_Cb, nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/iDCT_Cb.bmp", reverse_rgb(double2uint8(iDCT_Cb, nr*nc), nr, nc, 8), nr, nc);
	iDCT_Cr = iDCT_transform(DCT_Cr, nr, nc);
	Save_bmp_8bit("result/DCT_iDCT/iDCT_Cr.bmp", reverse_rgb(double2uint8(iDCT_Cr, nr*nc), nr, nc, 8), nr, nc);


	/* 五、色彩轉換 */
	uint8* reconstruct_image = (uint8*)calloc(nr*nc * 3, sizeof(uint8));
	YCbCr2RGB(iDCT_Y, iDCT_Cb, iDCT_Cr, reconstruct_image, nr, nc);
	Save_bmp_24bit("result/reconstruct_image.bmp", reverse_rgb(reconstruct_image, nr, nc, 24), nr, nc);

#endif


	/* 六、評估準則 */
	// 相減圖片計算PSNR
	// Y
	uint8* subtract_Y = (uint8*)calloc(nr*nc, sizeof(uint8));
	subtract_Y = subtract_img(Y, double2uint8(iDCT_Y, nr*nc), nr*nc);
	Save_bmp_8bit("result/subtract/subtract_Y.bmp", reverse_rgb(subtract_Y, nr, nc, 8), nr, nc);

	double psnr_Y = PSNR(get_image_MSE(Y, double2uint8(iDCT_Y, nr*nc), nr*nc));
	printf("\nPSNR_Y = %f\n", psnr_Y);

	// Cb
	uint8* subtract_Cb = (uint8*)calloc(nr*nc, sizeof(uint8));
	subtract_Cb = subtract_img(Cb, double2uint8(iDCT_Cb, nr*nc), nr*nc);
	Save_bmp_8bit("result/subtract/subtract_Cb.bmp", reverse_rgb(subtract_Cb, nr, nc, 8), nr, nc);

	double psnr_Cb = PSNR(get_image_MSE(Cb, double2uint8(iDCT_Cb, nr*nc), nr*nc));
	printf("PSNR_Cb = %f\n", psnr_Cb);


	// Cr
	uint8* subtract_Cr = (uint8*)calloc(nr*nc, sizeof(uint8));
	subtract_Cr = subtract_img(Cr, double2uint8(iDCT_Cr, nr*nc), nr*nc);
	Save_bmp_8bit("result/subtract/subtract_Cr.bmp", reverse_rgb(subtract_Cr, nr, nc, 8), nr, nc);

	double psnr_Cr = PSNR(get_image_MSE(Cr, double2uint8(iDCT_Cr, nr*nc), nr*nc));
	printf("PSNR_Cr = %f\n", psnr_Cr);


	// RGB
	uint8* subtract_BGR = (uint8*)calloc(nr*nc * 3, sizeof(uint8));
	subtract_BGR = subtract_img(origin, reconstruct_image, nr*nc*3);
	Save_bmp_24bit("result/subtract/subtract_BGR.bmp", reverse_rgb(subtract_BGR, nr, nc, 24), nr, nc);

	double psnr = PSNR(get_image_MSE(origin, reconstruct_image, nr*nc*3));
	printf("PSNR = %f\n", psnr);


	// 相減圖片做正規化
	// Y
	double* norm_subtract_Y = calloc(nr*nc, sizeof(double));
	norm_subtract_Y = Normalize(subtract_Y, nr*nc);
	Save_bmp_8bit("result/subtract/norm_subtract_Y.bmp", reverse_rgb(double2uint8(norm_subtract_Y, nr*nc), nr, nc, 8), nr, nc);
	
	// Cb
	double* norm_subtract_Cb = calloc(nr*nc, sizeof(double));
	norm_subtract_Cb = Normalize(subtract_Cb, nr*nc);
	Save_bmp_8bit("result/subtract/norm_subtract_Cb.bmp", reverse_rgb(double2uint8(norm_subtract_Cb, nr*nc), nr, nc, 8), nr, nc);

	// Cr
	double* norm_subtract_Cr = calloc(nr*nc, sizeof(double));
	norm_subtract_Cr = Normalize(subtract_Cr, nr*nc);
	Save_bmp_8bit("result/subtract/norm_subtract_Cr.bmp", reverse_rgb(double2uint8(norm_subtract_Cr, nr*nc), nr, nc, 8), nr, nc);


	system("pause");
	return 0;
}