#pragma once

#include <cstdint>
#include <memory>
#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

typedef struct ImageFormat {
	int32_t n;
	int32_t x;
	int32_t y;
	uint8_t **img;
	int32_t *dur;	// in 0,01 seconds

	~ImageFormat();
} ImgF;

std::shared_ptr<ImgF> eadJPG(const std::fs::path path);

std::shared_ptr<ImgF> ReadPNG(const std::fs::path path);

std::shared_ptr<ImgF> ReadGIF(const std::fs::path path);

std::shared_ptr<ImgF> ReadBMP(const std::fs::path path);

std::shared_ptr<ImgF> ReadImage(const std::fs::path path);

std::shared_ptr<ImgF> ResizeImage(std::shared_ptr<ImgF> img, double zoom);

std::shared_ptr<ImgF> ResizeImageChunk(std::shared_ptr<ImgF> img, double zoom, unsigned long xOut, unsigned long yOut, unsigned long xspos, unsigned long yspos, const char *prgDir);

std::shared_ptr<ImgF> FitImage(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit, unsigned char fitTo, unsigned char filteropt);

std::shared_ptr<ImgF> FitImageNN(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit);

std::shared_ptr<ImgF> FitImageBiLin(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit);

std::shared_ptr<ImgF> BlurImage(std::shared_ptr<ImgF> img, float stdev);

std::shared_ptr<ImgF> FitImageX(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit, int filter);

int ScaleBmNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long x2, unsigned long y2, double zoom);

int ScaleBmBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, double zoom);

int ScaleBmChunkNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom);

int ScaleBmChunkBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom);