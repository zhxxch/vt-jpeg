All: stat-jpeg-text.exe stat-jpeg-pixel.exe
CFLAGS=/D_CRT_SECURE_NO_WARNINGS=1 /O2 /nologo /GL /W4 /MD /DEBUG /Zi /arch:AVX2

STATHEADERS=DHTs.h DQTs.h DRIs.h markers.h MCUs.h SOSs.h SOFs.h

stat-jpeg-text.exe: stat-jpeg-text.c $(STATHEADERS)
	cl stat-jpeg-text.c $(CFLAGS)
	
stat-jpeg-pixel.exe: stat-jpeg-pixel.c
	cl stat-jpeg-pixel.c $(CFLAGS)

clean:
	del *.obj