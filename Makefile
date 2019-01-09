INCLUDES=
CC=gcc
CFLAGS=$(INCLUDES) -std=gnu11 -g -DLINUX \
	-Werror=implicit-function-declaration \
	-Werror=incompatible-pointer-types


SOURCES=main.c \
	c3dlas.c \
	ds.c \
	dumpImage.c \
	hash.c \
	MurmurHash3.c \
	texture.c \
	textureAtlas.c



LIBS=-lpng -lm -pthread


OBJS = $(SOURCES:.c=.o)

all: atlasgen


.c.o:
	$(CC) $(CFLAGS) -o $@ -c $< 

atlasgen: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^




clean:
	$(RM) *.o atlasgen
	
	
# .PHONY: clean

# clean:
# 	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
