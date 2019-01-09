
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>

#include "hash.h"
#include "texture.h"

#include <png.h>
#include <setjmp.h>


// something more sophisticated will be needed for tex arrays and view sharing
// a start is a start




BitmapRGBA8* readPNG(char* path) {
	int ret;
	
	BitmapRGBA8* b = (BitmapRGBA8*)calloc(sizeof(BitmapRGBA8), 1);
	if(!b) return NULL;
	
	ret = readPNG2(path, b);
	if(ret) {
		if(b->data) free(b->data);
		free(b);
		return NULL;
	}
	
	return b;
}

int readPNG2(char* path, BitmapRGBA8* bmp) {
	
	FILE* f;
	png_byte sig[8];
	png_bytep* rowPtrs;
	int i;
	png_byte colorType;
	png_byte bitDepth;
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open \"%s\" (readPNG).\n", path);
		return 1;
	}
	
	fread(sig, 1, 8, f);
	
	if(png_sig_cmp(sig, 0, 8)) {
		fprintf(stderr, "\"%s\" is not a valid PNG file.\n", path);
		fclose(f);
		return 1;
	}
	
	png_structp readStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!readStruct) {
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 1.\n", path);
		fclose(f);
		return 1;
	}
	
	png_infop infoStruct = png_create_info_struct(readStruct);
	if (!infoStruct) {
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 2.\n", path);
		png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);
		fclose(f);
		return 1;
	};
	
	
	bmp->path = path;
	
	// exceptions are evil. the alternative with libPNG is a bit worse. ever heard of return codes libPNG devs?
	if (setjmp(png_jmpbuf(readStruct))) {
		
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 3.\n", path);
		png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);
		
		if(bmp->data) free(bmp->data);
		free(bmp);
		fclose(f);
		return 1;
	}
	
	
	png_init_io(readStruct, f);
	png_set_sig_bytes(readStruct, 8);

	png_read_info(readStruct, infoStruct);

	bmp->width = png_get_image_width(readStruct, infoStruct);
	bmp->height = png_get_image_height(readStruct, infoStruct);
	
	
	// coerce the fileinto 8-bit RGBA        
	
	bitDepth = png_get_bit_depth(readStruct, infoStruct);
	colorType = png_get_color_type(readStruct, infoStruct);
	
	if(colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(readStruct);
	}
	if(colorType == PNG_COLOR_TYPE_GRAY) {
		png_set_expand_gray_1_2_4_to_8(readStruct);
		png_set_expand(readStruct);
	}
	
	if(bitDepth == 16) {
		png_set_strip_16(readStruct);
	}
	if(bitDepth < 8) {
		png_set_packing(readStruct);
	}
	if(png_get_valid(readStruct, infoStruct, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(readStruct);
	}
	if(colorType == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(readStruct);
		png_set_expand(readStruct);
	}		
	if(colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(readStruct);
	}

// 	if(colorType > PNG_COLOR_TYPE_PALETTE) {
//  	png_color_16 background = {.red= 255, .green = 255, .blue = 255};
//  	png_set_background(readStruct, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
// 	}
//  	png_set_filler(readStruct, 0x00, PNG_FILLER_AFTER);
	
//	printf("interlace: %d\n", png_set_interlace_handling(readStruct));
	
	png_read_update_info(readStruct, infoStruct);
	
	
	// read the data
	bmp->data = (uint32_t*)malloc(bmp->width * bmp->height * sizeof(uint32_t));
	
	rowPtrs = (png_bytep*)malloc(sizeof(png_bytep) * bmp->height);
	
	for(i = 0; i < bmp->height; i++) {
		rowPtrs[i] = (png_bytep)(bmp->data + (bmp->width * i));
	}

	png_read_image(readStruct, rowPtrs);

	
	free(rowPtrs);
	png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);

	fclose(f);
	
//	printf("Loaded \"%s\".\n", path);
	
	return 0;
}





static uint32_t average(BitmapRGBA8* bmp, Vector2 min, Vector2 max) {
	int w = bmp->width;
	int xmin, xmax, ymin, ymax;
	int x, y;
	
	union {
		uint32_t u;
		uint8_t b[4];
	} u;
	
	xmin = MAX(0, floor(min.x));
	xmax = MIN(bmp->width, ceil(max.x));
	ymin = MAX(0, floor(min.y));
	ymax = MIN(bmp->height, ceil(max.y));
		
	float r = 0.0;
	float g = 0.0;
	float b = 0.0;
	float a = 0.0; 
	
	for(y = ymin; y < ymax; y++) {  
		for(x = xmin; x < xmax; x++) {  
			u.u = bmp->data[(int)(ceil(x) + (w * ceil(y)))];	
			
			r += (float)u.b[0]; // these may not correspond properly to the component
			g += (float)u.b[1]; //   but it doesn't matter as long as everything is
			b += (float)u.b[2]; //   consistent across the function
			a += (float)u.b[3];
		}
	}
	
	float totalWeight = (xmax - xmin) * (ymax - ymin);

	r /= totalWeight;
	g /= totalWeight;
	b /= totalWeight;
	a /= totalWeight;
	
	u.b[0] = iclamp(r, 0, 255);
	u.b[1] = iclamp(g, 0, 255);
	u.b[2] = iclamp(b, 0, 255);
	u.b[3] = iclamp(a, 0, 255);
	
	return u.u;
}



static BitmapRGBA8* resample(BitmapRGBA8* in, Vector2i outSz) {
	int ox, oy;
	float scaleFactor = in->width / outSz.x;
	BitmapRGBA8* out;
	
	out = calloc(1, sizeof(out));
	out->width = outSz.x;
	out->height = outSz.y;
	out->data = malloc(out->width * out->height * sizeof(*out->data));
	
	// really shitty algorithm
	for(oy = 0; oy < outSz.y; oy++) {
		for(ox = 0; ox < outSz.x; ox++) {
			
			out->data[ox + (oy * outSz.x)] = average(
				in, 
				(Vector2){ox * scaleFactor, oy * scaleFactor},
				(Vector2){ox * (scaleFactor + 1), oy * (scaleFactor + 1)}
			);
		}
	}
	
	return out;
}




// nearest resizing

static uint32_t bmpsample(BitmapRGBA8* b, Vector2i p) {
	return b->data[iclamp(p.x, 0, b->width) + (iclamp(p.y, 0, b->height) * b->width)];
}

static uint32_t bmpsamplef(BitmapRGBA8* b, Vector2 p) {
	Vector2i pi = {floor(p.x + .5), floor(p.y + .5)};
	return bmpsample(b, pi);
}

static uint32_t bmpsamplescale(BitmapRGBA8* b, Vector2i p, float scale) {
	Vector2i pi = {floor((p.x * scale) + .5), floor((p.y * scale) + .5)};
	return bmpsample(b, pi);
}


static BitmapRGBA8* nearestRescale(BitmapRGBA8* in, Vector2i outSz) {
	int ox, oy;
	float scaleFactor = (float)outSz.x / (float)in->width;
	BitmapRGBA8* out;
	
	out = calloc(1, sizeof(out));
	out->width = outSz.x;
	out->height = outSz.y;
	out->data = malloc(out->width * out->height * sizeof(*out->data));
	
	for(oy = 0; oy < outSz.y; oy++) {
		for(ox = 0; ox < outSz.x; ox++) {
			out->data[ox + (oy * outSz.x)] = bmpsamplescale(in, (Vector2i){ox, oy}, 1.0f / scaleFactor);
		}
	}
	
	return out;
}



// linear rescaling

static void unpack8(uint32_t v, uint32_t* r, uint32_t* g, uint32_t* b, uint32_t* a) {
	union {
		uint32_t n;
		uint8_t b[4];
	} u;
	
	u.n = v;
	*r = u.b[0];
	*g = u.b[1];
	*b = u.b[2];
	*a = u.b[3];
}
static void unpackadd8(uint32_t v, uint32_t* o) {
	union {
		uint32_t n;
		uint8_t b[4];
	} u;
	
	u.n = v;
	o[0] += u.b[0];
	o[1] += u.b[1];
	o[2] += u.b[2];
	o[3] += u.b[3];
}

static uint32_t repackdiv8(uint32_t* o, float divisor) {
	union {
		uint32_t n;
		uint8_t b[4];
	} u;
	
	u.b[0] = (float)o[0] / divisor;
	u.b[1] = (float)o[1] / divisor;
	u.b[2] = (float)o[2] / divisor;
	u.b[3] = (float)o[3] / divisor;
	
	return u.n;
}



static uint32_t bmplinsamplescale(BitmapRGBA8* b, Vector2i p, int scale) {
	int x, y;
	float invscale = (float)scale;
	Vector2i p0 = {p.x * invscale, p.y * invscale};
	
	uint32_t acc[4] = {0,0,0,0};
	
	for(y = 0; y < scale; y++) {
		for(x = 0; x < scale; x++) {
			unpackadd8(bmpsample(b, (Vector2i){p0.x + x, p0.y + y}), acc);
		}
	}
	
	return repackdiv8(acc, scale * scale);
}


static BitmapRGBA8* linearDownscale(BitmapRGBA8* in, Vector2i outSz) {
	int ox, oy;
	int scaleFactor = (float)in->width / (float)outSz.x;
	BitmapRGBA8* out;
	
	out = calloc(1, sizeof(out));
	out->width = outSz.x;
	out->height = outSz.y;
	out->data = malloc(out->width * out->height * sizeof(*out->data));
	
	for(oy = 0; oy < outSz.y; oy++) {
		for(ox = 0; ox < outSz.x; ox++) {
			out->data[ox + (oy * outSz.x)] = bmplinsamplescale(in, (Vector2i){ox, oy}, scaleFactor);
		}
	}
	
	return out;
}





// actually, the argument is a void**, but the compiler complains. stupid standards...
size_t ptrlen(const void* a) {
	size_t i = 0;
	void** b = (void**)a;
	
	while(*b++) i++;
	return i;
}




static int depth_bytes[] = {
	[TEXDEPTH_8] = 1,
	[TEXDEPTH_16] = 2,
	[TEXDEPTH_32] = 4,
	[TEXDEPTH_FLOAT] = 4,
	[TEXDEPTH_DOUBLE] = 8,
};

TexBitmap* TexBitmap_create(int w, int h, enum TextureDepth d, int channels) {
	int dbytes;
	TexBitmap* bmp;
	
	if(d >= TEXDEPTH_MAXVALUE) return NULL; 
	dbytes = depth_bytes[d];
	
	bmp = calloc(1, sizeof(*bmp));	
	
	bmp->data8 = malloc(w * h * channels * dbytes);
	bmp->width = w;
	bmp->height = h;
	bmp->channels = channels;
	
	return bmp;
}




