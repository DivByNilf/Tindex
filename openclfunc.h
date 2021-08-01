#define CL_TARGET_OPENCL_VERSION 220
#include "CL/cl.h"

int ScaleBmChunkBiLinCL(unsigned char **from, unsigned char **to, const unsigned long n, const unsigned long x1, const unsigned long y1, const unsigned long x2, const unsigned long y2, const unsigned long xspos, const unsigned long yspos, const double zoom);