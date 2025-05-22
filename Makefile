test_drm: main.c
	gcc main.c -I/usr/include/libdrm -ldrm -o $@
