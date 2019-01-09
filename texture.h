#ifndef __EACSMB_texture_h__
#define __EACSMB_texture_h__


#include "c3dlas.h"
#include "hash.h"
#include "ds.h"


// defined in main.c. dropped in here for visibility. was originally in a utilities file.
char* pathJoin(const char* a, const char* b);
const char* pathExt(const char* path);
const char* pathExt2(const char* path, int* end);
#define streq(a, b) (0 == strcmp(a, b))
#define strcaseeq(a, b) (0 == strcasecmp(a, b))




// lots of cruft in this file from the parent project



typedef struct {
	char* path;
	short width, height;
	uint32_t* data;
} BitmapRGBA8;


enum TextureDepth {
	TEXDEPTH_8,
	TEXDEPTH_16,
	TEXDEPTH_32,
	TEXDEPTH_FLOAT,
	TEXDEPTH_DOUBLE,
	TEXDEPTH_MAXVALUE
};


typedef struct TexBitmap {
	short channels;
	enum TextureDepth depth;
	unsigned int width, height;
	
	union {
		uint8_t* data8;
		uint16_t* data16;
		uint32_t* data32;
		float* fdata;
		double* ddata;
	};
	
} TexBitmap;







typedef struct FloatBitmap {
	unsigned int w, h;
	float* data;
} FloatBitmap;

typedef struct FloatTex {
	char channels;
	unsigned int w, h;
	FloatBitmap* bmps[4];
} FloatTex;







BitmapRGBA8* readPNG(char* path);
int readPNG2(char* path, BitmapRGBA8* bmp);


TexBitmap* TexBitmap_create(int w, int h, enum TextureDepth d, int channels); 




	
	

int TexBitmap_pixelStride(TexBitmap* bmp);
int TexBitmap_componentSize(TexBitmap* bmp); 
void* TexBitmap_pixelPointer(TexBitmap* bmp, int x, int y);

void TexBitmap_sampleFloat(TexBitmap* bmp, int x, int y, float* out);

BitmapRGBA8* TexGen_Generate(char* source, Vector2i size); 

FloatBitmap* FloatBitmap_alloc(int width, int height);
FloatTex* FloatTex_alloc(int width, int height, int channels);
void FloatTex_free(FloatTex* ft);
FloatTex* FloatTex_similar(FloatTex* orig);
FloatTex* FloatTex_copy(FloatTex* orig);
float FloatTex_sample(FloatTex* ft, float xf, float yf, int channel);
float FloatTex_texelFetch(FloatTex* ft, int x, int y, int channel); 

BitmapRGBA8* FloatTex_ToRGBA8(FloatTex* ft);






// found in textureAtlas.c

typedef struct TextureAtlasItem {
	Vector2 offsetPx;
	Vector2 offsetNorm;
	
	Vector2 sizePx;
	Vector2 sizeNorm;
	
	int index;
	
} TextureAtlasItem;


// for building the atlas
typedef struct TextureAtlasSource {
	char* name;
	
	float aspectRatio;
	Vector2 size;
	uint8_t* data;
} TextureAtlasSource;


typedef struct TextureAtlas {
	HashTable(TextureAtlasItem*) items;
	
	int width;
	VEC(uint32_t*) atlas;
	
	char* pngFileFormat;
	
	VEC(TextureAtlasSource*) sources;
	
	int maxAtlasSize;
	int verbose;
	
} TextureAtlas;


TextureAtlas* TextureAtlas_alloc();
void TextureAtlas_init(TextureAtlas* ta); 

void TextureAtlas_addPNG(TextureAtlas* ta, char* path);
void TextureAtlas_addFolder(TextureAtlas* ta, char* prefix, char* dirPath, int recursive);
void TextureAtlas_finalize(TextureAtlas* ta);

#endif // __EACSMB_texture_h__
