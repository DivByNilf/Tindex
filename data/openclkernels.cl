__kernel void ScaleBmChunkBiLinKer(__global unsigned char *from, __global unsigned char *to, const uint x1, const uint y1, const uint x2, const uint y2, const uint xspos, const uint yspos, const double zoom) {
	uint pos;
	long i, j, ypx1, ypx2, xpx1, xpx2, full_x, full_y;
	double y, x;
	float fmody1, fmody2, fmodx1, fmodx2;
	
	i = get_global_id(0);
	j = get_global_id(1);

	if (i >= y2 || j >= x2)
		return;
	
	full_x = x1*zoom;
	full_y = y1*zoom;
	
	if (yspos+i >= full_y) {	// out of bounds of the original image
		to[4*(i*x2+j)] = 0;
		to[4*(i*x2+j)+1] = 0;
		to[4*(i*x2+j)+2] = 0;
		return;
	}
	if (xspos+j >= full_x) {
		to[4*(i*x2+j)] = 0;
		to[4*(i*x2+j)+1] = 0;
		to[4*(i*x2+j)+2] = 0;
		return;
	}
	
	y = (yspos+i+0.5)/zoom;
	fmody2 = fmod((float)(y+0.5), (float)1.0);
	fmody1 = 1-fmody2;
	ypx1 = y-0.5;
	ypx2 = ypx1+1;
	if (ypx1 < 0) { // might not be necessary if it's truncated toward 0
		ypx1 = 0;
	}
	if (ypx1 >= y1) { //! should not be necessary to check
		ypx1 = y1 - 1;
	}
	if (ypx2 >= y1) {
		ypx2 = y1 - 1;
	}
	
	x = (xspos+j+0.5)/zoom;
	fmodx2 = fmod((float)(x+0.5), (float)1.0);
	fmodx1 = 1-fmodx2;
	xpx1 = x-0.5;
	xpx2 = xpx1+1;
	if (xpx1 < 0) {
		xpx1 = 0;
	}
	if (xpx1 >= x1) {
		xpx1 = x1 - 1;
	}
	if (xpx2 >= x1) {
		xpx2 = x1 - 1;
	}
	
	to[4*(i*x2+j)] = (from[4*(ypx1*x1+xpx1)]*fmodx1 + from[4*(ypx1*x1+xpx2)]*fmodx2)*fmody1 + (from[4*(ypx2*x1+xpx1)]*fmodx1 + from[4*(ypx2*x1+xpx2)]*fmodx2)*fmody2 + 0.5;
	
	to[4*(i*x2+j)+1] = (from[4*(ypx1*x1+xpx1)+1]*fmodx1 + from[4*(ypx1*x1+xpx2)+1]*fmodx2)*fmody1 + (from[4*(ypx2*x1+xpx1)+1]*fmodx1 + from[4*(ypx2*x1+xpx2)+1]*fmodx2)*fmody2 + 0.5;
	
	to[4*(i*x2+j)+2] = (from[4*(ypx1*x1+xpx1)+2]*fmodx1 + from[4*(ypx1*x1+xpx2)+2]*fmodx2)*fmody1 + (from[4*(ypx2*x1+xpx1)+2]*fmodx1 + from[4*(ypx2*x1+xpx2)+2]*fmodx2)*fmody2 + 0.5;
	return;
}