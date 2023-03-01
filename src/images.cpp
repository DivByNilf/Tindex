#include "images.hpp"

#include <cstdint>
#include <string>
#include <memory>

#include <filesystem>
namespace std {
	namespace fs = std::filesystem;
}

#include <stdio.h>
#include <setjmp.h>
#include <math.h>

#include "jpeglib.h"
#include <png.h>
#include <gif_lib.h>

extern "C" {
	#include "openclfunc.h"
	#include "portables.h"
}

#include "portables.hpp"
#include "breakpath.hpp"

#include "errorf.hpp"
#define errorf(str) g_errorfStdStr(str)

typedef struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
} *my_error_ptr;

static void my_error_exit(j_common_ptr cinfo) { // address of my_error_mgr struct is address of the first element (pub), cinfo.err receives the same address, by treating it as a pointer to my_error_mgr struct the second element can be accessed because the element is the same
	char buf[200];
	my_error_ptr myerr = (my_error_ptr)cinfo->err;
//	(*cinfo->err->output_message) (cinfo);	// function invocation
	(*cinfo->err->format_message)(cinfo, buf);
	g_errorfStream << "jpeg exit: " << buf << std::flush;

	longjmp(myerr->setjmp_buffer, 1);
}

std::shared_ptr<ImgF> ReadJPG(std::fs::path path) {		// test with monochrome jpgs
	uint8_t *bmbuf = 0;
	FILE *infile;
	std::shared_ptr<ImgF> image = 0;

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	int row_stride;
	unsigned char *rowp[1];

	if ((infile = MBfopen(path.generic_string().c_str(), "rb")) == NULL) {
		errorf("fopen failed (ReadJPG)");
		return 0;
	}
	
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		if (bmbuf)
			free(bmbuf);
		errorf("jumped error");
		return 0;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void)jpeg_read_header(&cinfo, TRUE);
	
	if (cinfo.num_components == 2) {
		errorf("2 Components");
	} if (cinfo.num_components > 4) {
		errorf("More than 4 components");
	}
	
	cinfo.out_color_space = JCS_EXT_BGRA;
	
	(void)jpeg_start_decompress(&cinfo);
	if ((bmbuf = (uint8_t *) malloc(cinfo.output_width*cinfo.output_height*4)) == 0) {
		g_errorfStream << "Malloc failed: width " << cinfo.image_width << ", height" << cinfo.image_height << std::flush;
	}
	image = std::make_shared<ImgF>();
	image->n = 1;
	image->x = cinfo.image_width;
	image->y = cinfo.image_height;
	image->dur = 0;
	image->img = (uint8_t **) malloc(sizeof(uint8_t *));
	*image->img = bmbuf;
	
	row_stride = cinfo.output_width * cinfo.output_components;
	while (cinfo.output_scanline < cinfo.output_height) {
		rowp[0] = bmbuf + cinfo.output_scanline * row_stride;
		(void)jpeg_read_scanlines(&cinfo, rowp, 1);
	}
	
	(void)jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	fclose(infile);
	
	return image;	
}

std::shared_ptr<ImgF> ReadPNG(std::fs::path path) {
	uint8_t *bmbuf = 0;
	FILE *infile;
	std::shared_ptr<ImgF> image = 0;
	int i, npasses;

	uint32_t width, height;
	int bit_depth, color_type;
	png_color_16 my_background = {0, 255, 255, 255, 0};
	png_color_16p image_background;
	int rowbytes;
	unsigned char *rowpointer;
	
	if ((infile = MBfopen(path.generic_string().c_str(), "rb")) == NULL) {
		g_errorfStream << "fopen failed (ReadPNG) \"" << path << "\"" << std::flush;
		return 0;
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		errorf("png_create_read_struct failed");
		
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		errorf("png_create_info_struct failed for info_ptr");
	}
	
	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		errorf("png_create_info_struct failed for end_info");
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		fclose(infile);
		if (bmbuf)
			free(bmbuf);
		errorf("jumped error");
		return 0;
	}
		
	png_init_io(png_ptr, infile);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);
	
	if (!(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA || PNG_COLOR_TYPE_PALETTE)) {
		g_errorfStream << "Not RGB, RGBA, grayscale or palette: \"" << path << "\"" << std::flush;
	}
	if (bit_depth != 8) {
		if (bit_depth == 16) {
			png_set_strip_16(png_ptr);
			if (!(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA)) {
				g_errorfStream << "non-rgb bit depth 16: " << path << std::flush;
			}
		} else {
			g_errorfStream << "Bit depth not 8, bit_depth: " << bit_depth << ", " << path << std::flush;
		}
	}
	if (png_get_bKGD(png_ptr, info_ptr, &image_background))
		png_set_background(png_ptr, image_background, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
	else
		png_set_background(png_ptr, &my_background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	png_set_add_alpha(png_ptr, 255, PNG_FILLER_AFTER);
	png_set_bgr(png_ptr);	// blue green red
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png_ptr);
	}
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	npasses = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	bmbuf = (uint8_t *) malloc(width*height*4);
	if (bmbuf == 0) {
		g_errorfStream << "Malloc failed: width " << width << ", height " << height << std::flush;
	}
	
	image = std::make_shared<ImgF>();
	image->n = 1;
	image->x = width;
	image->y = height;
	image->dur = 0;
	image->img = (uint8_t **) malloc(sizeof(char *));
	*image->img = bmbuf;
	
	rowbytes = width*4;
	rowpointer = bmbuf;
	while (npasses-- > 0) {
		for (i = 0; i < height; i++) {
			rowpointer = bmbuf + i * rowbytes;
			png_read_row(png_ptr, rowpointer, NULL);
		}
	}
	
	png_read_end(png_ptr, 0);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
				
	fclose(infile);
	
	return image;
}

std::shared_ptr<ImgF> ReadGIF(std::fs::path path) {		// test with special characters in path
	uint8_t *bmbuf;
	BITMAPINFO bminfo = {0};
	std::shared_ptr<ImgF> image = 0;

	int width, height, error, framew, frameh, framex, framey, i, j, k, *tcolors;
	int framec, *delay;
	long long int res, l;
	GifFileType *gifFile;
	unsigned char col, *disposal;
	GraphicsControlBlock GCB;
	
	if ((gifFile = DGifOpenFileName(path.generic_string().c_str(), &error)) == NULL) {
		g_errorfStream << "DGifOpenFileName -- Error code1: " << error << std::flush;
		return 0;
	}

	if (DGifSlurp(gifFile) == 0) {
		errorf("DGifSlurp failed");
		return 0;
	}
	
	width = gifFile->SWidth;
	height = gifFile->SHeight;
	res = width*height;
	framec = gifFile->ImageCount;

/*	if (gifFile->SColorMap == NULL)
		errorf("No global colormap. ImageCount: %d Bits per pixel(1): %d Width: %d Height: %d Interlaced: %d Colorcount: %d Sortflag: %d" , framec, gifFile->SavedImages[0].ImageDesc.ColorMap->BitsPerPixel, width, height, gifFile->SavedImages[0].ImageDesc.Interlace, gifFile->SavedImages[0].ImageDesc.ColorMap->ColorCount, gifFile->SavedImages[0].ImageDesc.ColorMap->SortFlag);
	else
		errorf("Yes global colormap. ImageCount: %d Bits per pixel: %d Width: %d Height: %d Interlaced: %d Colorcount: %d Sortflag: %d", framec, gifFile->SColorMap->BitsPerPixel, width, height, gifFile->SavedImages[1].ImageDesc.Interlace, gifFile->SColorMap->ColorCount, gifFile->SColorMap->SortFlag);
*/
	tcolors = (int *) calloc(framec, sizeof(int));
	delay = (int *) calloc(framec, sizeof(int));
	disposal = (unsigned char *) calloc(framec, sizeof(unsigned char));
	
	for (i = 0; i < framec; i++) {
		for (j = 0; j < gifFile->SavedImages[i].ExtensionBlockCount; j++) {
			if (gifFile->SavedImages[i].ExtensionBlocks[j].Function == 0xf9) {
				DGifExtensionToGCB(gifFile->SavedImages[i].ExtensionBlocks[j].ByteCount, gifFile->SavedImages[i].ExtensionBlocks[j].Bytes, &GCB);
				tcolors[i] = GCB.TransparentColor;
				delay[i] = GCB.DelayTime;	// in 0,01 second units - integer
				if (delay[i] == 0)
					delay[i] = 10;		// 0 defaults to 0,1 seconds
				disposal[i] = GCB.DisposalMode;
					if (disposal[i] == DISPOSE_BACKGROUND && gifFile->SBackGroundColor != tcolors[0]) 	// haven't tested with non-transparent bg -- only transparent color of the first frame matters?
						errorf("Dispose BG");													
					if (disposal[i] == DISPOSE_PREVIOUS)	// never seen; no support yet, would require keeping previous frame in buffer (which might be feasible here)
						errorf("Dispose previous");
			}
		}
		if (j == gifFile->SavedImages[i].ExtensionBlockCount-1 && gifFile->SavedImages[i].ExtensionBlocks[j-1].Function != 0xf9) {		//? is the j loop meant to break when it finds the GCB or does the GCB only come at a certain position? is the -1 meant to be there for the first operand of &&? 
			g_errorfStream << "Didn't get GCB frame %d" << std::flush;
		}
	}
	
	image = std::make_shared<ImgF>();
	image->n = framec;
	image->x = width;
	image->y = height;
	image->dur = delay;
	image->img = (uint8_t **) malloc(framec*sizeof(uint8_t *));
	bmbuf = (uint8_t *) malloc(res*4);
	
	for (i = 0; i < framec; i++) {
		if (!(image->img[i] = (uint8_t *) malloc(res*4))) {
			g_errorfStream << "malloc failed for gif frame " << i+1 << " / " << framec << std::flush;
			while (i > 0) {
				i--;
				free(image->img[i]);
			} free(tcolors), free(delay), free(disposal), free(bmbuf);
		}
	}

	if (gifFile->SBackGroundColor == tcolors[0]) {
		for (k = 0; k < res; k++) {
			bmbuf[4*k] = 255;				// default background color can be changed later
			bmbuf[4*k+1] = 255; 
			bmbuf[4*k+2] = 255;
			
		}
	} else if (gifFile->SColorMap) {
		for (k = 0; k < res; k++) {
			bmbuf[4*k] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Blue;
			bmbuf[4*k+1] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Green; 
			bmbuf[4*k+2] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Red;
		}
	} else if (gifFile->SavedImages[0].ImageDesc.ColorMap) {
			for (k = 0; k < res; k++) {
			bmbuf[4*k] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Blue;
			bmbuf[4*k+1] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Green; 
			bmbuf[4*k+2] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Red;
		}
	}
	
	//maybe could be optimized with more variables
	
	for (i = 0; i < framec; i++) {
		framew = gifFile->SavedImages[i].ImageDesc.Width;
		frameh = gifFile->SavedImages[i].ImageDesc.Height;
		framex = gifFile->SavedImages[i].ImageDesc.Left;
		framey = gifFile->SavedImages[i].ImageDesc.Top;
		
		if (gifFile->SavedImages[i].ImageDesc.ColorMap) {
			for (j = 0; j < frameh; j++) {
				for (k = 0; k < framew; k++) {
					col = gifFile->SavedImages[i].RasterBits[j*framew+k];
					if (col == tcolors[i])
						continue;
					bmbuf[4*((framey+j)*width+framex+k)] = gifFile->SavedImages[i].ImageDesc.ColorMap->Colors[col].Blue;
					bmbuf[4*((framey+j)*width+framex+k)+1] = gifFile->SavedImages[i].ImageDesc.ColorMap->Colors[col].Green;			// last byte of bmbuf is [3*width*height-1] = [3*((framey+j)*width+framex+k)+2)] = [3*(j*width+k)+2] = [3*((height-1)*width+width-1)+2]
					bmbuf[4*((framey+j)*width+framex+k)+2] = gifFile->SavedImages[i].ImageDesc.ColorMap->Colors[col].Red;			//  = [3*(width*height-width+width-1)+2] = [3*(width*height-1)+2] = [3*(width*height)-3+2] = [3*width*height-1]
				}
			}
			
		} else if (gifFile->SColorMap) {
			for (j = 0; j < frameh; j++) {
				for (k = 0; k < framew; k++) {
					col = gifFile->SavedImages[i].RasterBits[j*framew+k];
					if (col == tcolors[i]) {
						continue;
					} 
						
					bmbuf[4*((framey+j)*width+framex+k)] = gifFile->SColorMap->Colors[col].Blue;
					bmbuf[4*((framey+j)*width+framex+k)+1] = gifFile->SColorMap->Colors[col].Green;
					bmbuf[4*((framey+j)*width+framex+k)+2] = gifFile->SColorMap->Colors[col].Red;
				}
			}
			
		} else {
			g_errorfStream << "No global colormap or local colormap for " << (i+1) << std::flush;
		}
		for (j = 0; j < res*4; image->img[i][j] = bmbuf[j], j++);
		
		if (disposal[i] == DISPOSE_BACKGROUND) {
			if (gifFile->SBackGroundColor == tcolors[0]) {			//? not sure which frame's transparent color is used - only the first one's?
				for (k = 0; k < res; k++) {
					bmbuf[4*k] = 255;
					bmbuf[4*k+1] = 255; 
					bmbuf[4*k+2] = 255;
				}
			} else if (gifFile->SavedImages[0].ImageDesc.ColorMap) {
				for (k = 0; k < res; k++) {
					bmbuf[4*k] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Blue;			//? not sure which frame's colormap is used -- could be the first one's?
					bmbuf[4*k+1] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Green;
					bmbuf[4*k+2] = gifFile->SavedImages[0].ImageDesc.ColorMap->Colors[gifFile->SBackGroundColor].Red;
				}
			} else if (gifFile->SColorMap) {
				for (k = 0; k < res; k++) {
					bmbuf[4*k] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Blue;
					bmbuf[4*k+1] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Green;
					bmbuf[4*k+2] = gifFile->SColorMap->Colors[gifFile->SBackGroundColor].Red;
				}
			}
		}
	}
	
	free(bmbuf);
	free(tcolors);
	free(disposal);
	DGifCloseFile(gifFile, &error);

	return image;
}

std::shared_ptr<ImgF> ReadBMP(std::fs::path path) { //! unfinished
	FILE *infile;
	char buf[50];
	int i, j, k, bpp;
	unsigned long long start = 0;
	long long width, height;
	
	return 0;
	
	infile = MBfopen(path.generic_string().c_str(), "rb");
	buf[0] = getc(infile), buf[1] = getc(infile), buf[2] = '\0';
	
	if (strcmp(buf, "BM") && strcmp(buf, "BA") && strcmp(buf, "CI") && strcmp(buf, "CP") && strcmp(buf, "IC") && strcmp(buf, "PT")) {
		fclose(infile);
		g_errorfStream << "bitmap init chars, " << buf << std::flush;
		return 0;
	}
	fseek(infile, 8, SEEK_CUR);
	for (i = 0, j = 1; i < 4; i++, j *=256) {
		start += getc(infile)*j;
	}
	for (i = k = 0, j = 1; i < 4; i++, j *= 256) {
		k += getc(infile)*j;
	}
	
	if (k == 12) {
		width = getc(infile), width += getc(infile)*256;
		height = getc(infile), height += getc(infile)*256;
		getc(infile);
		if (getc(infile) != 1 || getc(infile) != 0) {
			errorf("color planes different from 1");
			fclose(infile);
			return 0;
		} bpp = getc(infile), bpp += getc(infile)*256;
		if (bpp != 24 && bpp != 8 && bpp != 4 && bpp != 1) {
			errorf("bpp not 1, 4, 8 or 24");
			fclose(infile);
			return 0;
		}
	} else if (k == 40) {
		for (i = 0, j = 1; i < 4; i++, j *=256) {
			width += getc(infile)*j;
		}
		for (i = 0, j = 1; i < 4; i++, j *=256) {
			height += getc(infile)*j;
		}
		
		
		
	} else {
		g_errorfStream << "unsupported bmp header: size - " << j << std::flush;
		fclose(infile);
		return 0;
	}
	
}

std::shared_ptr<ImgF> ReadImage(std::fs::path path) {
	std::string extension = ToLowerCase(path.extension().string());
	
	std::shared_ptr<ImgF> image;
	if (extension == "jpg" || extension == "jpeg") {
		image = ReadJPG(path);
	} else if (extension == "png") {
		image = ReadPNG(path);
	} else if (extension == "gif") {
		image = ReadGIF(path);
	} else if (extension == "bmp") {
		image = ReadBMP(path);
	} else {
		g_errorfStream << "no matched file extension: " << path << std::flush;
	}
	if (image == nullptr) {
		g_errorfStream << ("no picture: %s", path);
	}
	return image;
}

ImageFormat::~ImageFormat() {
	for (int32_t i = 0; i < this->n; i++) {
		if (this->img[i] != 0) {
			free(this->img[i]);
		}
	}
	if (this->dur != 0) {
		free(this->dur);
	}
}

std::shared_ptr<ImgF> ResizeImage(std::shared_ptr<ImgF> img, double zoom) {
	if (img == 0 || img->img == 0) {
		errorf("tried to resize 0");
		return 0;
	}
	std::shared_ptr<ImgF> dest;
	dest = std::make_shared<ImgF>();
	unsigned long pos;
	long long i, j, y, x;	// make x and y into floating points later and use them for interpolation
	
	dest->n = img->n;
	dest->x = img->x*zoom;
	dest->y = img->y*zoom;
	if (dest->x <= 0 || dest->y <= 0) {
		return 0;
	}
	if (dest->n > 1) {
		dest->dur = (int32_t *) malloc(dest->n*sizeof(int));
	} else {
		dest->dur = 0;
	}
	
	dest->img = (uint8_t **) malloc(dest->n*sizeof(unsigned char*));
	for (pos = 0; pos < dest->n; pos++) {
		dest->img[pos] = (uint8_t *) malloc(dest->x*dest->y*4);
		if (dest->n > 1) {
			dest->dur[pos] = img->dur[pos];
		}
	}
	
	if (0) {
		ScaleBmNN(img->img, dest->img, img->n, img->x, dest->x, dest->y, zoom);
	} else {
		ScaleBmBiLin(img->img, dest->img, img->n, img->x, img->y, dest->x, dest->y, zoom);
	}
	
	return dest;
}

std::shared_ptr<ImgF> ResizeImageChunk(std::shared_ptr<ImgF> img, double zoom, unsigned long xOut, unsigned long yOut, unsigned long xspos, unsigned long yspos, const char *prgDir) {
	if (img == 0 || img->img == 0) {
		errorf("tried to resize 0");
		return 0;
	} if (xspos < 0 || yspos < 0) {
		errorf("resizeimagechunk startpos less than 0");
	}
	std::shared_ptr<ImgF> dest;
	dest = std::make_shared<ImgF>();
	unsigned long pos;
	long long i, j, y, x, full_x, full_y;
	
	dest->n = img->n;
	dest->x = xOut;
	dest->y = yOut;
	full_x = img->x*zoom;
	full_y = img->y*zoom;
	if (xspos+xOut > full_x || yspos+yOut > full_y) {
		errorf("imagechunk reaches out of bounds");
		g_errorfStream
			<< "vals: zoom: " << zoom
			<< ", full_x: " << full_x
			<< ", full_y: " << full_y
			<<", xspos: " << xspos
			<< ", xOut: " << xOut
			<< ", yspos: " << yspos
			<< ", yOut: " << yOut 
			<< std::flush;
	}
		
	if (dest->x <= 0 || dest->y <= 0) {
		return 0;
	}
	if (dest->n > 1) {
		dest->dur = (int32_t *) malloc(dest->n*sizeof(int));
	} else {
		dest->dur = 0;
	}
	
	dest->img = (uint8_t **) malloc(dest->n*sizeof(unsigned char*));
	for (pos = 0; pos < dest->n; pos++) {
		dest->img[pos] = (uint8_t *) malloc(dest->x*dest->y*4);
		if (dest->n > 1) {
			dest->dur[pos] = img->dur[pos];
		}
	}
	errorf("ResizeImageChunk spot 3");
//	ScaleBmChunkNN(img->img, dest->img, img->n, img->x, img->y, dest->x, dest->y, xspos, yspos, zoom);
//	ScaleBmChunkBiLin(img->img, dest->img, img->n, img->x, img->y, dest->x, dest->y, xspos, yspos, zoom);
	ScaleBmChunkBiLinCL(img->img, dest->img, img->n, img->x, img->y, dest->x, dest->y, xspos, yspos, zoom, prgDir);
	return dest;
}

std::shared_ptr<ImgF> FitImage(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit, unsigned char fitTo, unsigned char filteropt) {
	if (img == 0 || img->img == 0) {
		errorf("tried to resize 0");
		return 0;
	}
	std::shared_ptr<ImgF> dest;
	unsigned long pos;
	long long i, j, y, x;
	double zoom1, zoom2;
	
	i = 0;
	while (1) {
		zoom1 = (double) xfit / (double) img->x;
		zoom2 = (double) yfit / (double) img->y;
		
		if (zoom1 <= zoom2) {
/*			if (zoom1 == zoom2) i= 1; else i = 0;
			zoom1 = nextafter(zoom1, INFINITY); //rounded up to guarantee (xfit-0.5)/zoom1 is less than img->x
			dest->x = xfit;
			if (i)
				dest->y = yfit;
			else
				dest->y = img->y*zoom1;
*/		} else {
			zoom1 = zoom2;
//			zoom1 = nextafter(zoom1, INFINITY);
//			dest->x = img->x*zoom1;
//			dest->y = yfit;
		}
		if (zoom1 >= 0.5) {
			dest = FitImageBiLin(img, xfit, yfit);
			break;
		} else {
			dest = FitImageBiLin(img, img->x/2, img->y/2);
			if (i == 1) {
				img = nullptr;
			} else {
				i = 1;
			}
			img = dest;
			
		}
	}
	
	/*
	dest->n = img->n;
	
	if (dest->x <= 0 || dest->y <= 0) {
		free(dest);
		return 0;
	}
	if (dest->n > 1) {
		dest->dur = malloc(dest->n*sizeof(int));
	} else {
		dest->dur = 0;
	}
	
	dest->img = (uint8_t **) malloc(dest->n*sizeof(unsigned char*));
	for (pos = 0; pos < dest->n; pos++) {
		dest->img[pos] = malloc(dest->x*dest->y*4);
	}
	for (pos = 0; pos < dest->n; pos++) {
		if (dest->n > 1) {
			dest->dur[pos] = img->dur[pos];
		}
	}
	
	DestroyImgF(img);
	*/
	
	return dest;
}

std::shared_ptr<ImgF> FitImageX(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit, int filter) {
	if (img == 0 || img->img == 0) {
		errorf("tried to resize 0");
		return 0;
	}
	std::shared_ptr<ImgF> dest;
	dest = std::make_shared<ImgF>();
	unsigned long pos;
	long long i, j;
	double zoom1, zoom2;
	
	dest->n = img->n;
	
	zoom1 = (double) xfit / (double) img->x;
	zoom2 = (double) yfit / (double) img->y;
	
	if (zoom1 <= zoom2) {
		if (zoom1 == zoom2) i= 1; else i = 0;
		zoom1 = nextafter(zoom1, INFINITY); //rounded up to guarantee (xfit-0.5)/zoom1 is less than img->x
		dest->x = xfit;
		if (i)
			dest->y = yfit;
		else
			dest->y = img->y*zoom1;
	} else {
		zoom1 = zoom2;
		zoom1 = nextafter(zoom1, INFINITY);
		dest->x = img->x*zoom1;
		dest->y = yfit;
	}
	if (dest->x <= 0 || dest->y <= 0) {
		return 0;
	}
	if (dest->n > 1) {
		dest->dur = (int32_t *) malloc(dest->n*sizeof(int));
	} else {
		dest->dur = 0;
	}
	
	dest->img = (uint8_t **) malloc(dest->n*sizeof(unsigned char*));
	for (pos = 0; pos < dest->n; pos++) {
		dest->img[pos] = (uint8_t *) malloc(dest->x*dest->y*4);
		if (dest->n > 1) {
			dest->dur[pos] = img->dur[pos];
		}
	}
	
	if (filter == 0) {
		ScaleBmNN(img->img, dest->img, img->n, img->x, dest->x, dest->y, zoom1);
	} else {
		ScaleBmBiLin(img->img, dest->img, img->n, img->x, img->y, dest->x, dest->y, zoom1);
	}
	
	return dest;
	
}

std::shared_ptr<ImgF> FitImageNN(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit) {
	return FitImageX(img, xfit, yfit, 0);
}

std::shared_ptr<ImgF> FitImageBiLin(std::shared_ptr<ImgF> img, unsigned long xfit, unsigned long yfit) {
	return FitImageX(img, xfit, yfit, 1);
}

int ScaleBmChunkNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom) {
	unsigned long pos;
	long long i, j, y, x, full_x, full_y;
	
	full_x = x1*zoom;
	full_y = y1*zoom;
	
	for (pos = 0; pos < n; pos++) {
		if (from[pos]) {
			for (i = 0; i < y2; i++) {
				if (yspos+i >= full_y) {	// out of bounds of the original image
					for (j = 0; j < x2; j++) {
						to[pos][4*(i*x2+j)] = 0;
						to[pos][4*(i*x2+j)+1] = 0;
						to[pos][4*(i*x2+j)+2] = 0;
					}
					continue;
				}
				y = (yspos+i+0.5)/zoom;
				for (j = 0; j < x2; j++) {
					if (xspos+j >= full_x) {
						to[pos][4*(i*x2+j)] = 0;
						to[pos][4*(i*x2+j)+1] = 0;
						to[pos][4*(i*x2+j)+2] = 0;
						continue;
					}
					x = (xspos+j+0.5)/zoom;
					to[pos][4*(i*x2+j)] = from[pos][4*(y*x1+x)];
					to[pos][4*(i*x2+j)+1] = from[pos][4*(y*x1+x)+1];
					to[pos][4*(i*x2+j)+2] = from[pos][4*(y*x1+x)+2];
				}
			}
		} else {
			for (i = 0; i < y2; i++) {
				for (j = 0; j < x2; j++) {
					to[pos][4*(i*x2+j)] = 0;
					to[pos][4*(i*x2+j)+1] = 0;
					to[pos][4*(i*x2+j)+2] = 0;
				}
			}
		}
	}
	
	return 0;
}

int ScaleBmChunkBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long xspos, unsigned long yspos, double zoom) {
	unsigned long pos;
	long long i, j, ypx1, ypx2, xpx1, xpx2, full_x, full_y;
	double y, x;
	float fmody1, fmody2, fmodx1, fmodx2;
	
	full_x = x1*zoom;
	full_y = y1*zoom;
	
	// errorf("resizing with: n: %d, x1: %d, y1: %d, x2: %d, y2: %d, xspos: %d, yspos: %d, zoom: %f", n, x1, y1, x2, y2, xspos, yspos, zoom);
	
	// first_y: ((yspos + 0.5) / zoom) - 0.5
	// last_y : ((yspos + y2 - 1 + 0.5) / zoom) - 0.5
	
	for (pos = 0; pos < n; pos++) {
		if (from[pos]) {
			for (i = 0; i < y2; i++) {
				if (yspos+i >= full_y) {	// out of bounds of the original image
					for (j = 0; j < x2; j++) {
						to[pos][4*(i*x2+j)] = 0;
						to[pos][4*(i*x2+j)+1] = 0;
						to[pos][4*(i*x2+j)+2] = 0;
					}
					continue;
				}
				y = (yspos+i+0.5)/zoom;
				fmody2 = fmod(y+0.5, 1.0);
				fmody1 = 1-fmody2;
				ypx1 = y-0.5;
				ypx2 = ypx1+1;
				if (ypx1 < 0) { // might not be necessary if it's truncated toward 0
					ypx1 = 0;
					errorf("ypx1 under");
				}
				if (ypx1 >= y1) {
					ypx1 = y1 - 1;
					errorf("ypx1 over");
				}
				if (ypx2 < 0) { //! this check shouldn't be necessary
					ypx2 = 0;
					errorf("ypx2 under");
				}
				if (ypx2 >= y1) {
					ypx2 = y1 - 1;
					//errorf("ypx2 over");
				}
				for (j = 0; j < x2; j++) {
					if (xspos+j >= full_x) {
						to[pos][4*(i*x2+j)] = 0;
						to[pos][4*(i*x2+j)+1] = 0;
						to[pos][4*(i*x2+j)+2] = 0;
						continue;
					}
					x = (xspos+j+0.5)/zoom;
					fmodx2 = fmod(x+0.5, 1.0);
					fmodx1 = 1-fmodx2;
					xpx1 = x-0.5;
					xpx2 = xpx1+1;
					if (xpx1 < 0) { // might not be necessary if it's truncated toward 0
						xpx1 = 0;
						errorf("xpx1 under");
					}
					if (xpx1 >= x1) {
						xpx1 = x1 - 1;
						errorf("xpx1 over");
					}
					if (xpx2 < 0) { //! this check shouldn't be necessary
						xpx2 = 0;
						errorf("xpx2 under");
					}
					if (xpx2 >= x1) {
						xpx2 = x1 - 1;
						//errorf("xpx2 over");
					}
						
					to[pos][4*(i*x2+j)] = (from[pos][4*(ypx1*x1+xpx1)]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)]*fmodx2)*fmody2 + 0.5;
					
					to[pos][4*(i*x2+j)+1] = (from[pos][4*(ypx1*x1+xpx1)+1]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)+1]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)+1]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)+1]*fmodx2)*fmody2 + 0.5;
					
					to[pos][4*(i*x2+j)+2] = (from[pos][4*(ypx1*x1+xpx1)+2]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)+2]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)+2]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)+2]*fmodx2)*fmody2 + 0.5;
				}
			}
		} else {
			for (i = 0; i < y2; i++) {
				for (j = 0; j < x2; j++) {
					to[pos][4*(i*x2+j)] = 0;
					to[pos][4*(i*x2+j)+1] = 0;
					to[pos][4*(i*x2+j)+2] = 0;
				}
			}
		}
	}
	
	return 0;
}

int ScaleBmNN(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long x2, unsigned long y2, double zoom) {
	unsigned long pos;
	long long i, j, y, x;
	
	for (pos = 0; pos < n; pos++) {
		if (from[pos]) {
			for (i = 0; i < y2; i++) {
				y = (i+0.5)/zoom;
				for (j = 0; j < x2; j++) {
					x = (j+0.5)/zoom;
					to[pos][4*(i*x2+j)] = from[pos][4*(y*x1+x)];
					to[pos][4*(i*x2+j)+1] = from[pos][4*(y*x1+x)+1];
					to[pos][4*(i*x2+j)+2] = from[pos][4*(y*x1+x)+2];
				}
			}
		} else {
			for (i = 0; i < y2; i++) {
				y = (i+0.5)/zoom;
				for (j = 0; j < x2; j++) {
					x = (j+0.5)/zoom;
					to[pos][4*(i*x2+j)] = 0;
					to[pos][4*(i*x2+j)+1] = 0;
					to[pos][4*(i*x2+j)+2] = 0;
				}
			}
		}
	}
	
	return 0;
}

int ScaleBmBiLin(unsigned char **from, unsigned char **to, unsigned long n, unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, double zoom) {
	unsigned long pos;
	long long i, j, ypx1, ypx2, xpx1, xpx2;
	double y, x;
	float fmody1, fmody2, fmodx1, fmodx2;
	
	ScaleBmChunkBiLin(from, to, n, x1, y1, x2, y2, 0, 0, zoom);
	
	return 0;
	
	//!
	
	for (pos = 0; pos < n; pos++) {
		if (from[pos]) {
			for (i = 0; i < y2; i++) {
				y = (i+0.5)/zoom;
				fmody2 = fmod(y+0.5, 1.0);
				fmody1 = 1-fmody2;
				ypx1 = y-0.5;
				ypx2 = ypx1+1;
//errorf("y: %lf, fmody1: %lf, fmody2: %lf, ypx1: %lld, ypx2: %lld", y, fmody1, fmody2, ypx1, ypx2);

				//! no checks should be necessary
				if (ypx1 < 0) { // might not be necessary if it's truncated toward 0
					ypx1 = 0;
					errorf("ypx1 under");
				}
				if (ypx1 >= y1) {
					ypx1 = y1 - 1;
					errorf("ypx1 over");
				}
				if (ypx2 < 0) { //! this check shouldn't be necessary
					ypx2 = 0;
					errorf("ypx2 under");
				}
				if (ypx2 >= y1) {
					ypx2 = y1 - 1;
					//errorf("ypx2 over");
				}
				for (j = 0; j < x2; j++) {
					x = (j+0.5)/zoom;
					fmodx2 = fmod(x+0.5, 1.0); // from math.h
					fmodx1 = 1-fmodx2;
					xpx1 = x-0.5;
					xpx2 = xpx1+1;
					if (xpx1 < 0) {
						xpx1 = 0;
					}
					if (xpx1 >= x1) {
						xpx1 = x1 - 1;
					}
					
					to[pos][4*(i*x2+j)] = (from[pos][4*(ypx1*x1+xpx1)]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)]*fmodx2)*fmody2 + 0.5;
					
					to[pos][4*(i*x2+j)+1] = (from[pos][4*(ypx1*x1+xpx1)+1]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)+1]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)+1]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)+1]*fmodx2)*fmody2 + 0.5;
					
					to[pos][4*(i*x2+j)+2] = (from[pos][4*(ypx1*x1+xpx1)+2]*fmodx1 + from[pos][4*(ypx1*x1+xpx2)+2]*fmodx2)*fmody1 + (from[pos][4*(ypx2*x1+xpx1)+2]*fmodx1 + from[pos][4*(ypx2*x1+xpx2)+2]*fmodx2)*fmody2 + 0.5;
				}
			}
		} else {
			for (i = 0; i < y2; i++) {
				y = (i+0.5)/zoom;
				for (j = 0; j < x2; j++) {
					x = (j+0.5)/zoom;
					to[pos][4*(i*x2+j)] = 0;
					to[pos][4*(i*x2+j)+1] = 0;
					to[pos][4*(i*x2+j)+2] = 0;
				}
			}
		}
	}
	
	return 0;
	
}

std::shared_ptr<ImgF> BlurImage(std::shared_ptr<ImgF> img, float stdev) {
	if (img == 0 || img->img == 0) {
		errorf("no source image to blur");
		return 0;
	}
	
errorf("blur1");
	std::shared_ptr<ImgF> dest;
	dest = std::make_shared<ImgF>();
	unsigned long pos;
	long long i, j, k, abs, var4;
	uint8_t **bm;
	
	dest->n = img->n;
	dest->x = img->x;
	dest->y = img->y;
	
	if (dest->x <= 0 || dest->y <= 0) {
		return 0;
	}
	
	if (dest->n > 1) {
		dest->dur = (int32_t *) malloc(dest->n*sizeof(int));
	} else {
		dest->dur = 0;
	}
	
	dest->img = (uint8_t **) malloc(dest->n*sizeof(uint8_t *));
	bm = (uint8_t **) malloc(dest->n*sizeof(uint8_t *));
	for (pos = 0; pos < dest->n; pos++) {
		dest->img[pos] = (uint8_t *) malloc(dest->x*dest->y*4);
		bm[pos] = (uint8_t *) malloc(dest->x*dest->y*4);
	}
	
//	float stdev = 5;
	int range = stdev*3;
	float *mods = (float *) malloc((range+1)*sizeof(float));
	float out = 1;
	
//errorf("range: %i", range);
	
	float gauss1 = (float) 1 / sqrt(2*M_PI*stdev*stdev);
//errorf("gauss1: %f", gauss1);
	float gauss2 = (float) 2*stdev*stdev;
//errorf("gauss2: %f", gauss2);
	for (i = 0; i <= range; i++) {
		mods[i] = gauss1 * exp(-i*i/(2*stdev*stdev));
		out -= mods[i];
		if (i != 0)
			out -= mods[i];
		if (1 - out < 0.005) {
			range = i;
		}
//errorf("mods[%lld]: %f", i, mods[i]);
	}
g_errorfStream << "out: " << out << std::flush;
	
//errorf("range after: %i", range);

	
	float cumR, cumG, cumB, cumA, cumOut;
errorf("blur2");
	
	for (pos = 0; pos < dest->n; pos++) {
		if (dest->n > 1) {
			dest->dur[pos] = img->dur[pos];
		}
		if (img->img[pos]) {
			for (i = 0; i < dest->y; i++) {
				for (j = 0; j < dest->x; j++) {
					cumR = cumG = cumB = cumA = cumOut = out;
					for (k = -range; k <= range; k++) {
						var4 = j+k;
						if (k >= 0)
							abs = k;
						else
							abs = -k;
//errorf("k: %lld, i: %lld, j: %lld", k, i, j);
//errorf("var4: %lld", var4);
						if (var4 >= 0 && var4 < img->x) {
//errorf("cumR before: %f", cumR);
//errorf("value: %d, mods: %f,  adding: %f", img->img[pos][4*(i*img->x+var4)], mods[abs], img->img[pos][4*(i*img->x+var4)] * mods[abs]);
							cumR += img->img[pos][4*(i*img->x+var4)] * mods[abs];
							cumG += img->img[pos][4*(i*img->x+var4)+1] * mods[abs];
							cumB += img->img[pos][4*(i*img->x+var4)+2] * mods[abs];
//errorf("cumR after: %f", cumR);
						} else {
							cumOut += mods[abs];
						}
					}
//errorf("cumR: %f, cumG: %f, cumB: %f, cumOut: %f, cumR value after: %f", cumR, cumG, cumB, cumOut, cumR/(1-cumOut)+0.5);
					bm[pos][4*(i*dest->x+j)] = cumR/(1-cumOut)+0.5;	// theoretically max 255 -- practically cumR rounded down and divisor rounded up so always 255 at most
					bm[pos][4*(i*dest->x+j)+1] = cumG/(1-cumOut)+0.5;
					bm[pos][4*(i*dest->x+j)+2] = cumB/(1-cumOut)+0.5;
				}
			}
		} else {
			for (i = 0; i < dest->y; i++) {
				for (j = 0; j < dest->x; j++) {
					bm[pos][4*(i*dest->x+j)] = 0;
					bm[pos][4*(i*dest->x+j)+1] = 0;
					bm[pos][4*(i*dest->x+j)+2] = 0;
				}
			}
		}
	}
	
errorf("made it out");
	for (pos = 0; pos < dest->n; pos++) {
		for (i = 0; i < dest->y; i++) {
			for (j = 0; j < dest->x; j++) {
				cumR = cumG = cumB = cumA = cumOut = out;
				for (k = -range; k <= range; k++) {
					var4 = i+range;
					if (k >= 0)
						abs = k;
					else
						abs = -k;
					if (var4 >= 0 && var4 < img->y) { 
						cumR += bm[pos][4*(var4*img->x+j)] * mods[abs];
						cumG += bm[pos][4*(var4*img->x+j)+1] * mods[abs];
						cumB += bm[pos][4*(var4*img->x+j)+2] * mods[abs];
					} else {
						cumOut += mods[abs];
					}
				}
				dest->img[pos][4*(i*dest->x+j)] = cumR/(1-cumOut)+0.5;
				dest->img[pos][4*(i*dest->x+j)+1] = cumG/(1-cumOut)+0.5;
				dest->img[pos][4*(i*dest->x+j)+2] = cumB/(1-cumOut)+0.5;
			}
		}
		free(bm[pos]);
	}
errorf("pre freebm");
	free(bm);
errorf("after freebm");
	
	return dest;
}
