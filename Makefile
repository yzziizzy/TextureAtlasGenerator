INCLUDES=
CC=gcc
CFLAGS=$(INCLUDES) -std=gnu11 -g -DLINUX \
	-DSTI_C3DLAS_NO_CONFLICT \
	-fno-math-errno \
	-fexcess-precision=fast \
	-fno-signed-zeros -fno-trapping-math -fassociative-math \
	-ffinite-math-only -fno-rounding-math \
	-fno-signaling-nans \
	-Wall \
	-Wno-unused-result \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-Wno-unused-function \
	-Wno-unused-label \
	-Wno-pointer-sign \
	-Wno-missing-braces \
	-Wno-char-subscripts \
	-Wno-int-conversion \
	-Wno-int-to-pointer-cast \
	-Wno-unknown-pragmas \
	-Wno-sequence-point \
	-Wno-switch \
	-Wno-parentheses \
	-Wno-comment \
	-Werror=implicit-function-declaration \
	-Werror=uninitialized \
	-Werror=incompatible-pointer-types \
	-Werror=return-type 


SOURCES=main.c \
	c3dlas.c \
	sti/sti.c \
	dumpImage.c \
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
