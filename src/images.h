#ifndef _IMAGES_H
#define _IMAGES_H

typedef struct ImageFormat {
	unsigned long n;
	unsigned long x;
	unsigned long y;
	unsigned char **img;
	int *dur;	// in 0,01 seconds
} ImgF;

ImgF *ReadJPG(char *path);

ImgF *ReadPNG(char *path);

ImgF *ReadGIF(char *path);

ImgF *ReadBMP(char *path);

ImgF *ReadImage(char *path);

void DestroyImgF(ImgF *Image);

ImgF *ResizeImage(ImgF *img, double zoom);

ImgF *ResizeImageChunk(ImgF *img, double zoom, unsigned long xOut, unsigned long yOut, unsigned long xspos, unsigned long yspos);

ImgF *FitImage(ImgF *img, unsigned long xfit, unsigned long yfit, unsigned char fitTo, unsigned char filteropt);

ImgF *FitImageNN(ImgF *img, unsigned long xfit, unsigned long yfit);

ImgF *FitImageBiLin(ImgF *img, unsigned long xfit, unsigned long yfit);

ImgF *BlurImage(ImgF *img, float stdev);

ImgF *FitImageX(ImgF *img, unsigned long xfit, unsigned long yfit, int filter);

int ScaleBmNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long x2, unsigned long y2, double zoom);

int ScaleBmBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, double zoom);

int ScaleBmChunkNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom);

int ScaleBmChunkBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom);

#endif