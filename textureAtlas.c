
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h> 
#include <unistd.h>
#include <libgen.h>


#include "texture.h"


// for debugging
#include "dumpImage.h"








TextureAtlas* TextureAtlas_alloc() {
	TextureAtlas* ta;
	ta = calloc(1, sizeof(*ta));
	
	TextureAtlas_init(ta);
	
	return ta;
}

void TextureAtlas_init(TextureAtlas* ta) {
	VEC_INIT(&ta->sources);
	VEC_INIT(&ta->atlas);
	HT_init(&ta->items, 4);
}





void TextureAtlas_addPNG(TextureAtlas* ta, char* path) {
	BitmapRGBA8 bmp;
	TextureAtlasSource* src;
	int ret;
	char* name = "fixme";
	
	if(verbose > 0) printf("Adding '%s'... ", path);
	bmp.data = NULL;
	ret = readPNG2(path, &bmp);
	if(ret) {
		if(bmp.data) free(bmp.data);
		if(verbose > 0) printf("failed.\n");
		return;
	}
	
	
	src = calloc(1, sizeof(*src));
	
	
	char* b = basename(path);
	char* e = strrchr(b, '.');
	src->name = strndup(b, e - b);
	
	src->size.x = bmp.width;
	src->size.y = bmp.height;
	src->aspectRatio = src->size.x / src->size.y;
	src->data = (void*)bmp.data;
	
	if(verbose > 0) printf("[%fx%f]\n", src->size.x, src->size.y);
	
	VEC_PUSH(&ta->sources, src);
}


// not the most efficient function. could use some optimization later.
void TextureAtlas_addFolder(TextureAtlas* ta, char* prefix, char* dirPath, int recursive) {
	DIR* d;
	struct dirent *dir;
	char* path;
	int prefixlen = strlen(prefix);
	
	d = opendir(dirPath);
	if(!d) return;
	
	while(dir = readdir(d)) {
		if(0 == strncmp(dir->d_name, ".", 1)) continue;
		if(0 == strncmp(dir->d_name, "..", 2)) continue;
		
		path = pathJoin(dirPath, dir->d_name);
		
		if(dir->d_type == DT_REG) { // regular file
			int namelen;
			char* ext = pathExt2(dir->d_name, &namelen);
			char* name = malloc(namelen + prefixlen + 2);
			strcpy(name, prefix);
			strcat(name, "/");
			strncat(name, dir->d_name, namelen);
		
			if(streq(ext, "png")) {
// 				printf("loading '%s' into atlas as '%s'\n", path, name);
				TextureAtlas_addPNG(ta, path);
			}
			
			free(name);
		}
		else if(recursive && dir->d_type == DT_DIR) {
			char* dirpre = pathJoin(prefix, dir->d_name);
			
			TextureAtlas_addFolder(ta, dirpre, path, 1);
			free(dirpre);
		}
		
		free(path);
	}
	
	closedir(d);
}





static void blit32(
	int src_x, int src_y, int dst_x, int dst_y, int w, int h,
	int src_w, int dst_w, uint32_t* src, uint32_t* dst) {
	
	
	int y, x, s, d;
	
	for(y = 0; y < h; y++) {
		for(x = 0; x < w; x++) {
			s = ((y + src_y) * src_w) + src_x + x;
			d = ((y + dst_y) * dst_w) + dst_x + x;
			
			dst[d] = src[s];
		}
	}
}


// sort tallest and widest first
static int source_sort_comp(const void* aa, const void * bb) {
	TextureAtlasSource* a = *((TextureAtlasSource**)aa);
	TextureAtlasSource* b = *((TextureAtlasSource**)bb);
	
	if(a->size.y == b->size.y) {
		return b->size.x - a->size.x;
	}
	else {
		return b->size.y - a->size.y;
	}
}

void TextureAtlas_finalize(TextureAtlas* ta) {
	char buf[256]; // for debugging
	
	if(VEC_LEN(&ta->sources) == 0) {
		if(verbose > 0) printf("Texture atlas finalized without any sources\n");
		return;
	}
	
	VEC_SORT(&ta->sources, source_sort_comp);
	
	if(ta->width == 0) {
		int max = 0;
		VEC_EACH(&ta->sources, i, src) {
			printf("sz %f\n", src->size.x);
			if(src->size.x > max) max = src->size.x;
		}
		printf("sz %d\n", max);
		ta->width = max;
	}
	
	
	if(verbose > 1) printf("Output width: %d\n", ta->width);
	
	
	int width = ta->width;
	int width2 = width * width;
	float fwidth = width;
	
	uint32_t* texData = malloc(sizeof(*texData) * width2);
	memset(texData, 0, sizeof(*texData) * width2);
	
	
	int row = 0;
	int hext = 0;
	int prevhext = VEC_ITEM(&ta->sources, 0)->size.y;
	int rowWidth = 0;
	VEC_EACH(&ta->sources, ind, src) {
		if(verbose > 0) printf("Source: %s [%f]\n", src->name, src->size.x);
		
		if(rowWidth + src->size.x > width) {
			row++;
			rowWidth = 0;
			hext += prevhext;
			prevhext = src->size.y;
			
			
			// next texture
			if(hext + prevhext > width) { // the texture is square; width == height
				VEC_PUSH(&ta->atlas, texData);
				
// 				sprintf(buf, "texatlas-%d.png", VEC_LEN(&ta->atlas));
// 				writePNG(buf, 4, texData, width, width);
				sprintf(buf, ta->pngFileFormat, (int)VEC_LEN(&ta->atlas));
				writePNG(buf, 4, (uint8_t*)texData, width, width);
				
				TextureAtlasDest* dst = calloc(1, sizeof(*dst));
				dst->filename = strdup(buf);
				VEC_PUSH(&ta->dests, dst);
				
				if(verbose > 0) printf("Saved atlas file '%s'\n", buf);
				
// 				
				texData = malloc(sizeof(*texData) * width2);
				memset(texData, 0, sizeof(*texData) * width2);
				
				hext = 0;
			}
		}
		
		// blit the sdf bitmap data
		blit32(
			0, 0, // src x and y offset for the image
			rowWidth, hext, // dst offset
			src->size.x, src->size.y, // width and height
			src->size.x, width, // src and dst row widths
			(void*)src->data, // source
			texData); // destination
		
		
		TextureAtlasItem* it = calloc(1, sizeof(*it));
		*it = (TextureAtlasItem){
			.offsetPx = {rowWidth, hext},
			.offsetNorm = {(float)rowWidth / fwidth, (float)hext / fwidth},
				
			.sizePx = src->size,
			.sizeNorm = {src->size.x / fwidth, src->size.y / fwidth},
				
			.index = VEC_LEN(&ta->atlas)
		};
		
// 		printf("added icon '%s'\n", src->name);
		HT_set(&ta->items, strdup(src->name), it);
		
		
		rowWidth += src->size.x;
		
		
// 		writePNG("sourceour.png", 4, src->data, src->size.x, src->size.y);
		
		free(src->name);
		free(src->data);
		free(src);
		
// 		break;
	}
	
// 	printf("last push %p\n", texData);
	VEC_PUSH(&ta->atlas, texData);
	
	sprintf(buf, ta->pngFileFormat, (int)VEC_LEN(&ta->atlas));
	writePNG(buf, 4, (uint8_t*)texData, width, width);
	
	TextureAtlasDest* dst = calloc(1, sizeof(*dst));
	dst->filename = strdup(buf);
	VEC_PUSH(&ta->dests, dst);
	
	if(verbose >= 0) printf("Saved atlas file '%s'\n", buf);
// 	sprintf(buf, "texatlas-%d.png", VEC_LEN(&ta->atlas));
// 	writePNG(buf, 4, texData, width, width);
}



void TextureAtlas_PrintMetadata(TextureAtlas* ta, FILE* f) {
	fprintf(f, "{\n");
	fprintf(f, "\t\"layers\": [\n");
	VEC_EACH(&ta->dests, i, dst) {
		fprintf(f, "\t\t\"%s\",\n", dst->filename);
	}
	fprintf(f, "\t],\n");
	
	fprintf(f, "\t\"images\": {\n");
	HT_EACH(&ta->items, name, TextureAtlasItem*, it) {
		fprintf(f, "\t\t\"%s\": {\n", name);
		
		fprintf(f, "\t\t\t\"name\": \"%s\",\n", name);
		fprintf(f, "\t\t\t\"layer\": %d,\n", it->index);
		fprintf(f, "\t\t\t\"size\": {\"x\": %d, \"y\": %d},\n", (int)it->sizePx.x, (int)it->sizePx.y);
		fprintf(f, "\t\t\t\"size_norm\": {\"x\": %f, \"y\": %f},\n", it->sizeNorm.x, it->sizeNorm.y);
		fprintf(f, "\t\t\t\"offset\": {\"x\": %d, \"y\": %d},\n", (int)it->offsetPx.x, (int)it->offsetPx.y);
		fprintf(f, "\t\t\t\"offset_norm\": {\"x\": %f, \"y\": %f},\n", it->offsetNorm.x, it->offsetNorm.y);
		
		fprintf(f, "\t\t},\n");
	}
	fprintf(f, "\t},\n");
	
	
	
	
	fprintf(f, "}\n");
	
}


/*
void printCharinfo(FILE* f, char* prefix, struct charInfo* ci) {
	if(ci->code == 0) return;
// 	fprintf(f, "%s%d: {\n", prefix, ci->code);
// 	fprintf(f, "%s\tcode: %d,\n", prefix, ci->code);
// 	fprintf(f, "%s\ttexIndex: %d,\n", prefix, ci->texIndex);
// 	fprintf(f, "%s\ttexelOffset: [%d, %d],\n", prefix, ci->texelOffset.x, ci->texelOffset.y);
// 	fprintf(f, "%s\ttexelSize: [%d, %d],\n", prefix, ci->texelSize.x, ci->texelSize.y);
// 	fprintf(f, "%s\tnormalizedOffset: [%f, %f],\n", prefix, ci->texNormOffset.x, ci->texNormOffset.y);
// 	fprintf(f, "%s\tnormalizedSize: [%f, %f],\n", prefix, ci->texNormSize.x, ci->texNormSize.y);
// 	fprintf(f, "%s\tadvance: %f,\n", prefix, ci->advance);
// 	fprintf(f, "%s\tboxOffset: [%f, %f],\n", prefix, ci->topLeftOffset.x, ci->topLeftOffset.y);
// 	fprintf(f, "%s\tboxSize: [%f, %f]\n", prefix, ci->size.x, ci->size.y);
// 	fprintf(f, "%s},\n", prefix);
	fprintf(f, "%s\"%d\": [ ", prefix, ci->code);
	fprintf(f, "%d, ", ci->code);
	fprintf(f, "%d, ", ci->texIndex);
	fprintf(f, "[%d, %d], ", ci->texelOffset.x, ci->texelOffset.y);
	fprintf(f, "[%d, %d], ", ci->texelSize.x, ci->texelSize.y);
	fprintf(f, "[%f, %f], ", ci->texNormOffset.x, ci->texNormOffset.y);
	fprintf(f, "[%f, %f], ", ci->texNormSize.x, ci->texNormSize.y);
	fprintf(f, "%f, ", ci->advance);
	fprintf(f, "[%f, %f], ", ci->topLeftOffset.x, ci->topLeftOffset.y);
	fprintf(f, "[%f, %f] ", ci->size.x, ci->size.y);
	fprintf(f, "],\n", prefix);
}


void TextureAtlas_saveJSON(TextureAtlas* ta, char* path) {
	FILE* f;

	f = fopen(path, "w");
	if(!f) {
		fprintf(stderr, "Could not save JSON metadata to '%s'\n", path);
		exit(1);
	}
	
	fprintf(f, "{\n");
	
	fprintf(f, "\t\"layers\": [\n");
	VEC_LOOP(&ta->atlas, fi) {
		fprintf(f, "\t\t\"");
		fprintf(f, ta->pngFileFormat, (int)fi);
		fprintf(f, "\",\n");
	}
	fprintf(f, "\t],\n");
	
	fprintf(f, "\t\"infoIndices\": {\n");
	fprintf(f, "\t\t\"code\": 0,\n");
	fprintf(f, "\t\t\"texIndex\": 1,\n");
	fprintf(f, "\t\t\"texelOffset\": 2,\n");
	fprintf(f, "\t\t\"texelSize\": 3,\n");
	fprintf(f, "\t\t\"normalizedOffset\": 4,\n");
	fprintf(f, "\t\t\"normalizedSize\": 5,\n");
	fprintf(f, "\t\t\"advance\": 6,\n");
	fprintf(f, "\t\t\"boxOffset\": 7,\n");
	fprintf(f, "\t\t\"boxSize\": 8\n");
	
	fprintf(f, "\t},\n");
	
	fprintf(f, "\t\"fonts\": {\n");
	VEC_EACH(&ta->fonts, fi, font) {
		fprintf(f, "\t\t\"%s-%d\": {\n", font->name, font->size);
		
		fprintf(f, "\t\t\t\"name\": \"%s\",\n", font->name);
		fprintf(f, "\t\t\t\"size\": %d,\n", font->size);
		
		if(font->hasRegular) {
			fprintf(f, "\t\t\t\"regular\": {\n");
			for(int i = 0; i < font->charsLen; i++) {
				printCharinfo(f, "\t\t\t\t", font->regular + i);
			}
			fprintf(f, "\t\t\t},\n");
		}
		if(font->hasBold) {
			fprintf(f, "\t\t\t\"bold\": {\n");
			for(int i = 0; i < font->charsLen; i++) {
				printCharinfo(f, "\t\t\t\t", &font->bold[i]);
			}
			fprintf(f, "\t\t\t},\n");
		}
		if(font->hasItalic) {
			fprintf(f, "\t\t\t\"italic\": {\n");
			for(int i = 0; i < font->charsLen; i++) {
				printCharinfo(f, "\t\t\t\t", &font->italic[i]);
			}
			fprintf(f, "\t\t\t},\n");
		}
		if(font->hasBoldItalic) {
			fprintf(f, "\t\t\t\"boldItalic\": {\n");
			for(int i = 0; i < font->charsLen; i++) {
				printCharinfo(f, "\t\t\t\t", &font->boldItalic[i]);
			}
			fprintf(f, "\t\t\t},\n");
		}
		
		fprintf(f, "\t\t},\n");
	}
	fprintf(f, "\t}\n");
	
	fprintf(f, "}");
	
	
	fclose(f);
	
	if(verbose >= 0) printf("Saved config file '%s'\n", path);
}

*/


