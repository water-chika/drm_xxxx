all: test_drm switch_vt

test_drm: main.c
	gcc main.c -I/usr/include/libdrm -ldrm -o $@

switch_vt: switch_vt.c
	gcc switch_vt.c -o $@
