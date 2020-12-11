#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/sysinfo.h>
#include <wordexp.h>

#include "texture.h"
#include "sti/sti.h"







char* helpText = "" \
	"Usage:\n" \
	"atlasgen [GLOBAL OPTION]... [-f FONT SIZES [OPTION]...]... [outfile]\n" \
	"\n" \
	"EXAMPLE:\n" \
	"  atlasgen -b -m 8 -o 4 -t 256 -f \"Courier\" 8,12,16 -i -f \"Arial\" 32 myfonts_sdf\n" \
	"     -b - generate bold for all fonts\n" \
	"     -m 8 - SDF magnitude of 8 pixels\n" \
	"     -o 4 - oversample by 4x\n" \
	"     -t 256 - output textures have 256px dimensions\n" \
	"     -f \"Courier\" - first font name \n" \
	"        8,12,16 - generate outputs of size 8, 12, and 16 px for this font \n" \
	"        -i - also generate italic for this font\n" \
	"     -f \"Arial\" - second font name \n" \
	"        32 - generate outputs of size 32px for this font \n" \
	"     myfonts_sdf - output will be myfonts_sdf_[n].png and myfonts_sdf.json\n" \
	"\n" \
	"OPTIONS:\n" \
	"     All Font Options can be included as global options for all fonts.\n" \
	"  -f ...     Add a font to render\n" \
	"  -J         Specify the output JSON file name\n" \
	"  -l N       Limit computation to N threads\n" \
	"  -m         SDF magnitude in pixels. Default 8.\n" \
	"  -o         Amount of pixels to oversample by. Default 16.\n" \
	"  -P FORMAT  Specify the png file name; %d will be expanded to the index.\n" \
	"  -q         Quiet. Suppress all output, even warnings and errors.\n" \
	"  -t [N|fit] Size of output texture in pixels\n" \
	"               'fit' generates a single texture large enough to hold all images\n" \
	"  -v         Verbose.\n" \
	"\n" \
	"MISC OPTIONS:\n" \
	"  --help      Print this message and exit\n" \
	"  --pretend   Verify parameters but do not actually generate output.\n" \
;





int verbose = 0;




int main(int argc, char* argv[]) {
	int an;
	
	#define default_outfile "atlas-output"
	#define default_png_outfile default_outfile "-%d";
	
	char* outfile = NULL;
	char* json_outfile = NULL;
	char* png_outfile = NULL;
	
	int maxCores = get_nprocs();
	char pretend = 0;
	int texSize = 0;
	
	VEC(char*) imagePaths;
	VEC_INIT(&imagePaths);
	
	for(an = 1; an < argc; an++) {
		char* arg = argv[an];
		
		if(arg[0] != '-') {
			wordexp_t p;
			char** w;
			
			wordexp(arg, &p, 0);
			w = p.we_wordv;
			
			for(int i = 0; i < p.we_wordc; i++) {
				printf("Expanding to %s\n", w[i]);
				VEC_PUSH(&imagePaths, strdup(w[i]));
			}
			wordfree(&p);
//			VEC_PUSH(&imagePaths, arg);
		}
		
		
		if(0 == strcmp(arg, "--")) {
			printf("invalid usage of '--'\n");
			exit(1);
		};
		
		if(0 == strcmp(arg, "--help")) {
			puts(helpText);
			exit(0);
		}
		

		if(0 == strcmp(arg, "-J")) {
			an++;
			json_outfile = strdup(argv[an]);
			continue;
		}

		if(0 == strcmp(arg, "-l")) {
			an++;
			int n = strtol(argv[an], NULL, 10);
			if(n > 0) maxCores = n;
			continue;
		}

		// oversample
		if(0 == strcmp(arg, "-o")) {
// 			an++;
// 			int n = strtol(argv[an], NULL, 10);
// 			if(n > 0) oversample = n;
			continue;
		}
		
		if(0 == strcmp(arg, "-P")) {
			an++;
			png_outfile = strdup(argv[an]);
			continue;
		}
		
		if(0 == strcmp(arg, "--pretend")) {
			pretend = 1;
			continue;
		}
		
		if(0 == strcmp(arg, "-q")) {
			verbose = -1;
			continue;
		}
		
		
		
		if(0 == strcmp(arg, "-t")) {
			an++;
			arg = argv[an];
			if(0 == strcmp(arg, "fit")) {
				texSize = 0;
			}
			else {
				int n = strtol(arg, NULL, 10);
				if(n > 0) texSize = n;
			}
			
			continue;
		}
		
		if(0 == strcmp(arg, "-v")) {
			verbose = 1;
			continue;
		}
		if(0 == strcmp(arg, "-vv")) {
			verbose = 2;
			continue;
		}
		
		
		
		// output file
		// TODO: check for -'s and error
		
	}
	
	
	// check and print the options
	if(!outfile) {
		json_outfile = default_outfile;
		png_outfile = default_png_outfile;
	}
	else {
		if(!json_outfile) {
			json_outfile = outfile;
		}
		if(!png_outfile) {
			png_outfile = outfile;
		}
	}
	

	// TODO: check for and append %d if needed
	
	// TODO: check for and append .png and .json 
	json_outfile = strcatdup(json_outfile, ".json");
	png_outfile = strcatdup(png_outfile, ".png");
	
	
	
	
	
	// sanity check
	if(texSize > 0 && texSize < 16 ) {
		if(verbose >= 0) printf("WARNING: Texure Size is suspiciously low: %dpx\n", texSize);
	}
	// check for power of 2
	if(texSize > 0 && (texSize != (texSize & (1 - texSize)))) {
		if(verbose >= 0) printf("WARNING: Texure Size is not a power of two: %dpx\n  GPUs like power of two textures.\n", texSize);
	}
	
	
	
	if(verbose >= 1) {
		printf("JSON output: %s\n", json_outfile);
		printf("PNG output: %s\n", png_outfile);
		printf("Verbosity: %d\n", verbose);
		printf("Max Threads: %d\n", maxCores);
		if(texSize == 0) {
			printf("Tex Size: fit\n");
		}
		else {
			printf("Tex Size: %dx%d\n", texSize, texSize);
		}
		
	}
	
	
	
	
	
	// start rendering
	if(pretend) {
		if(verbose >= 0) printf("Skipping generation (--pretend)\n");
	}
	else {
		
		if(0 == VEC_LEN(&imagePaths)) {
			if(verbose >= 0) printf("Nothing to generate. Check your options (use -v).\n");
			exit(0);
		}
		
		TextureAtlas* tm;
		
		tm = TextureAtlas_alloc();

		tm->pngFileFormat = png_outfile;
		
		
// 		fm->maxThreads = maxCores;
		tm->verbose = verbose;
		tm->maxAtlasSize = texSize;
		
		VEC_EACH(&imagePaths, ii, ip) {
			if(verbose > 0) printf("path: %s\n", ip);
			TextureAtlas_addPNG(tm, ip);
		}
		
		TextureAtlas_finalize(tm);
		
		FILE* jf = fopen(json_outfile, "wb");
		TextureAtlas_PrintMetadata(tm, jf);
		fclose(jf);
// 		FontManager_createAtlas(fm);
		
// 		FontManager_saveJSON(fm, json_outfile);
		
// 		FontManager_saveAtlas(fm, "fonts.atlas");
		
		
	}
		
	return 0;
}





