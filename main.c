#include <xf86drm.h>
#include <stddef.h>
#include <assert.h>
#include <xf86drmMode.h>
#include <stdio.h>
#include <inttypes.h>
#include <fcntl.h>
#include <drm_fourcc.h>
#include <sys/mman.h>
#include <errno.h>
#include <threads.h>
#include <string.h>
#include <unistd.h>

static int modeset_find_crtc(int fd, drmModeRes* res, drmModeConnector* conn) {
    drmModeEncoder* enc;
    int i, j;
    for (i = 0; i < conn->count_encoders; ++i) {
    }
    return -1;
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

static const char* modeset_property_flag_to_str(uint32_t flag) {
    switch (flag) {
        case DRM_MODE_PROP_PENDING:
            return "pending";
        case DRM_MODE_PROP_RANGE:
            return "range";
        case DRM_MODE_PROP_IMMUTABLE:
            return "immutable";
        case DRM_MODE_PROP_ENUM:
            return "enum";
        case DRM_MODE_PROP_BLOB:
            return "blob";
        case DRM_MODE_PROP_BITMASK:
            return "bitmask";
        case DRM_MODE_PROP_OBJECT:
            return "object";
        case DRM_MODE_PROP_SIGNED_RANGE:
            return "signed_range";
        case DRM_MODE_PROP_ATOMIC:
            return "atomic";
        default:
            return "unknown";
    }
}

static void modeset_property_print(int fd, uint32_t property_id, uint32_t property_value) {
    drmModePropertyPtr property = drmModeGetProperty(fd, property_id);

    printf("flags: ");
    for (uint32_t i = 0; i < 32; i++) {
        if ((1u<<i) & property->flags) {
            printf("%s | ", modeset_property_flag_to_str(1u<<i));
        }
    }
    printf("\n");

    printf("name: %s\n", property->name);

    if (property->count_values > 0) {
        printf("values: ");
        for (int i = 0; i < property->count_values; i++) {
            printf("%"PRIu64",", property->values[i]);
        }
        printf("\n");
    }

    if (property->count_enums > 0) {
        printf("enums: ");
        for (int i = 0; i < property->count_enums; i++) {
            printf("%s=%"PRIu64",", property->enums[i].name,property->enums[i].value);
        }
        printf("\n");
    }

    if (property->count_blobs > 0) {
        printf("count blobs: %d\n", property->count_blobs);
    }

    if (DRM_MODE_PROP_BLOB & property->flags && property_value != 0) {
        uint32_t blob_id = property_value;
        drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(fd, blob_id);
        printf("blob (length=%"PRIu32"): ", blob->length);
        uint8_t* data = blob->data;
        for (uint32_t i = 0; i < blob->length; i++) {
            printf("%d ", data[i]);
        }
        printf("\n");
        drmModeFreePropertyBlob(blob);
    }

    drmModeFreeProperty(property);
}

static void modeset_connector_print(int fd, drmModeConnector* connector) {
    printf("connector id: %d\n", connector->connector_id);
    printf("encoder id: %d\n", connector->encoder_id);
    printf("connection: %s\n", modeset_connection_to_str(connector->connection));
    printf("width x height: %"PRIu32"mm x %"PRIu32" mm\n", connector->mmWidth, connector->mmHeight);
    printf("sub pixel: %s\n", modeset_sub_pixel_to_str(connector->subpixel));

    printf("count of properties: %d\n", connector->count_props);
    for (int i = 0; i < connector->count_props; i++) {
        printf("\n");
        printf("property id: %"PRIu32 "\n", connector->props[i]);
        printf("property value: %"PRIu64 "\n", connector->prop_values[i]);
        modeset_property_print(fd, connector->props[i], connector->prop_values[i]);
        printf("\n");
    }
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

static void modeset_fb_print(drmModeFB* fb) {
    printf("fb id: %"PRIu32"\n", fb->fb_id);
    printf("width x height: %"PRIu32"x%"PRIu32"\n", fb->width, fb->height);
    printf("pitch: %"PRIu32"\n", fb->pitch);
    printf("bpp: %"PRIu32"\n", fb->bpp);
    printf("depth: %"PRIu32"\n", fb->depth);
    printf("handle: %"PRIu32"\n", fb->handle);
}

int main(int argc, const char* argv[]){
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

    uint32_t connected_connector_id;
    uint32_t connected_encoder_id;
    uint32_t connected_crtc_id;
    drmModeModeInfo prefered_mode;
    uint32_t previous_fb_id;

    for (int i = 0; i < resources->count_fbs; i++) {
        uint32_t fb_id = resources->fbs[i];
        drmModeFB* fb = drmModeGetFB(fd, fb_id);
        printf("-----fb info-----");
        modeset_fb_print(fb);
    }

    for (int i = 0; i < resources->count_connectors; i++) {
        uint32_t connector_id = resources->connectors[i];
        drmModeConnector* connector = drmModeGetConnector(fd, connector_id);
        printf("\nconnector info:\n");
        modeset_connector_print(fd, connector);
        if (connector->connection == DRM_MODE_CONNECTED){
            connected_connector_id = connector_id;
            connected_encoder_id = connector->encoder_id;
            prefered_mode = connector->modes[0];
            assert(connector->count_modes > 0);
        }
    }

    for (int i = 0; i < resources->count_encoders; i++) {
        uint32_t encoder_id = resources->encoders[i];
        drmModeEncoder* encoder = drmModeGetEncoder(fd, encoder_id);
        printf("\nencoder info\n");
        modeset_encoder_print(encoder);
        if (encoder_id == connected_encoder_id) {
            connected_crtc_id = encoder->crtc_id;
        }
    }

    for (int i = 0; i < resources->count_crtcs; i++) {
        uint32_t crtc_id = resources->crtcs[i];
        drmModeCrtc* crtc = drmModeGetCrtc(fd, crtc_id);
        printf("\n-----crtc info-----\n");
        modeset_crtc_print(crtc);
        if (crtc_id == connected_crtc_id) {
            previous_fb_id = crtc->buffer_id;
        }
    }

    res = drmSetMaster(fd);
    if (res != 0) {
        fprintf(stderr, "drmSetMaster failed\n");
        goto close_fd;
    }

    drmModeModeInfo mode = prefered_mode;
    mode.flags = 0;
    uint32_t fb_handles[2];
    uint32_t fb_ids[2];
    uint32_t* data_ptrs[2];
    uint32_t pitch;
    uint64_t fb_size;
    uint32_t width=mode.hdisplay, height=mode.vdisplay;
    for (int i =0; i < 2; i++) {
        res = drmModeCreateDumbBuffer(fd, width, height, 32, 0, &fb_handles[i], &pitch, &fb_size);
        if (res != 0) {
            fprintf(stderr, "drmModeCreateDumbBuffer failed\n");
            goto close_fd;
        }

        uint64_t data_offset;
        res = drmModeMapDumbBuffer(fd, fb_handles[i], &data_offset);
        if (res != 0) {
            fprintf(stderr, "drmModeMapDumbBuffer failed\n");
            goto free_dumb_buffer;
        }
        void* ptr = mmap(NULL, pitch*height, PROT_WRITE | PROT_READ, MAP_SHARED,
                fd, data_offset);
        if (ptr == MAP_FAILED) {
            fprintf(stderr, "mmap failed:%d\n", errno);
            goto free_dumb_buffer;
        }
        data_ptrs[i] = (uint32_t*)((char*)ptr);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                data_ptrs[i][y*(pitch/4)+x] = i % 2 == 0 ? 0x00ff0000 : 0x0000ff00;
            }
        }


        uint32_t fb_id=0;
        uint32_t offset=0;
        res = drmModeAddFB(fd, width, height, 24, 32, pitch, fb_handles[i], &fb_ids[i]);
        if (res != 0) {
            fprintf(stderr, "drmModeAddFB failed\n");
            goto free_dumb_buffer;
        }
        assert(fb_id != -1);
    }

    uint32_t connector_id = connected_connector_id;
    res = drmModeSetCrtc(fd, connected_crtc_id, fb_ids[0],
            0, 0, &connector_id, 1, &mode);
    if (res != 0) {
        fprintf(stderr, "drmModeSetCrtc failed:%d\n", res);
        goto free_dumb_buffer;
    }

    for (int i = 0; i < 300; i++) {
        struct timespec sleep_time = {.tv_nsec=1000000};
        thrd_sleep(&sleep_time, NULL);

        uint32_t page_flip_flag = DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_PAGE_FLIP_ASYNC;
        res = drmModePageFlip(fd, connected_crtc_id, fb_ids[i%2], page_flip_flag, NULL);
        if (res != 0) {
            fprintf(stderr, "drmModePageFlip failed:%d\n", res);
        }
        if (page_flip_flag & DRM_MODE_PAGE_FLIP_EVENT) {
            uint8_t buf[256];
            res = read(fd, buf, sizeof(buf));
            assert(res > 0);
            struct drm_event_vblank flip_done;
            memcpy(&flip_done, buf, sizeof(flip_done));
            //printf("drm_event_vblank.sequence = %" PRIu32 "\n", flip_done.sequence);
        }
    }

    struct timespec sleep_time = {.tv_sec=1};
    thrd_sleep(&sleep_time, NULL);

    res = drmModeSetCrtc(fd, connected_crtc_id, previous_fb_id,
            0, 0, &connector_id, 1, &mode);

free_dumb_buffer:
    for (int i = 0; i < 2; i++) {
        drmModeDestroyDumbBuffer(fd, fb_handles[i]);
    }
    
close_fd:
    drmClose(fd);
    return 0;
}
