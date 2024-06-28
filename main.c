#include <xf86drm.h>
#include <stddef.h>
#include <assert.h>
#include <xf86drmMode.h>
#include <stdio.h>
#include <inttypes.h>
#include <fcntl.h>

static int modeset_find_crtc(int fd, drmModeRes* res, drmModeConnector* conn) {
    drmModeEncoder* enc;
    int i, j;
    for (i = 0; i < conn->count_encoders; ++i) {
    }
}

static void modeset_resources_print(drmModeRes* res) {
    printf("fbs count: %d\n", res->count_fbs);
    printf("crtcs count: %d\n", res->count_crtcs);
    printf("connectors count: %d\n", res->count_connectors);
    printf("encoders count: %d\n", res->count_encoders);
    printf("min res: %dx%d\n", res->min_width, res->min_height);
    printf("max res: %dx%d\n", res->max_width, res->max_height);
}

const char* modeset_connection_to_str(drmModeConnection connection) {
    switch (connection) {
        case DRM_MODE_CONNECTED: return "connected";
        case DRM_MODE_DISCONNECTED: return "disconnected";
        case DRM_MODE_UNKNOWNCONNECTION: return "unknownconnection";
        default: return "unkown value";
    }
}
const char* modeset_sub_pixel_to_str(drmModeSubPixel sub_pixel) {
    switch (sub_pixel) {
        case DRM_MODE_SUBPIXEL_UNKNOWN: return "unknown";
        case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB: return "horizontal RGB";
        case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR: return "horizontal BGR";
        case DRM_MODE_SUBPIXEL_VERTICAL_RGB: return "vertical RGB";
        case DRM_MODE_SUBPIXEL_VERTICAL_BGR: return "vertical BGR";
        case DRM_MODE_SUBPIXEL_NONE: return "none";
    }
}

static void modeset_connector_print(drmModeConnector* connector) {
    printf("connector id: %d\n", connector->connector_id);
    printf("encoder id: %d\n", connector->encoder_id);
    printf("connection: %s\n", modeset_connection_to_str(connector->connection));
    printf("width x height: %"PRIu32"mm x %"PRIu32" mm\n", connector->mmWidth, connector->mmHeight);
    printf("sub pixel: %s\n", modeset_sub_pixel_to_str(connector->subpixel));
}

static void modeset_encoder_print(drmModeEncoder* encoder) {
    printf("encoder id: %d\n", encoder->encoder_id);
    printf("crtc id: %d\n", encoder->crtc_id);
    printf("possible crtcs: %x\n", encoder->possible_crtcs);
    printf("possible clones: %x\n", encoder->possible_clones);
}

static void modeset_mode_info_print(drmModeModeInfo* mode) {
    printf("clock: %d\n", mode->clock);
    printf("hdisplay,hsync_start,hsync_end,htotal,hskew: %"PRIu16", %"PRIu16", %"PRIu16", %"PRIu16", %"PRIu16"\n",
            mode->hdisplay, mode->hsync_start, mode->hsync_end, mode->htotal, mode->hskew);
    printf("vdisplay,vsync_start,vsync_end,vtotal,vscan: %"PRIu16", %"PRIu16", %"PRIu16", %"PRIu16", %"PRIu16"\n",
            mode->vdisplay, mode->vsync_start, mode->vsync_end, mode->vtotal, mode->vscan);
    printf("vrefresh: %"PRIu32"\n", mode->vrefresh);
    printf("flags: %"PRIx32"\n", mode->flags);
    printf("type: %"PRIu32"\n", mode->type);
    printf("name: %s\n", mode->name);
}

static void modeset_crtc_print(drmModeCrtc* crtc) {
    printf("crtc id: %d\n", crtc->crtc_id);
    printf("buffer id: %d\n", crtc->buffer_id);
    printf("(x,y): (%"PRIu32",%"PRIu32")\n", crtc->x, crtc->y);
    printf("width x height: %"PRIu32"x%"PRIu32"\n", crtc->width, crtc->height);
    printf("mode valid: %d\n", crtc->mode_valid);
    modeset_mode_info_print(&crtc->mode);
    printf("gamma size: %d\n", crtc->gamma_size);
}

int main(void){
    //int fd = drmOpen(NULL, "pci-0000:13:00.0");
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "drmOpen failed\n");
        return -1;
    }
    assert(fd >= 0);
    int res= 0;
    drmModeRes* resources = drmModeGetResources(fd);
    assert(resources != NULL);

    modeset_resources_print(resources);

    for (int i = 0; i < resources->count_connectors; i++) {
        uint32_t connector_id = resources->connectors[i];
        drmModeConnector* connector = drmModeGetConnector(fd, connector_id);
        printf("\nconnector info:\n");
        modeset_connector_print(connector);
    }

    for (int i = 0; i < resources->count_encoders; i++) {
        uint32_t encoder_id = resources->encoders[i];
        drmModeEncoder* encoder = drmModeGetEncoder(fd, encoder_id);
        printf("\nencoder info\n");
        modeset_encoder_print(encoder);
    }

    for (int i = 0; i < resources->count_crtcs; i++) {
        uint32_t crtc_id = resources->crtcs[i];
        drmModeCrtc* crtc = drmModeGetCrtc(fd, crtc_id);
        printf("\n-----crtc info-----\n");
        modeset_crtc_print(crtc);
    }

    res = drmSetMaster(fd);
    if (res != 0) {
        fprintf(stderr, "drmSetMaster failed\n");
        goto close_fd;
    }
    
close_fd:
    drmClose(fd);
    return 0;
}
