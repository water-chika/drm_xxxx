#include "drm_helper.hpp"

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

using namespace drm_helper;

template<typename T>
class add_drm_test : public T {
public:
    using parent = T;
    add_drm_test(const configure auto& conf) : parent{conf}
    {}
    void test() {
        int fd = parent::get_drm_fd();
        int res= 0;
        drmModeRes* resources = drmModeGetResources(fd);
        assert(resources != NULL);

        std::cout << resources;

        uint32_t connected_connector_id = 0;
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
            if (connected_connector_id == 0 && connector->connection == DRM_MODE_CONNECTED){
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
        }
        struct timespec sleep_time = {.tv_sec=1};

        drmModeModeInfo mode = prefered_mode;
        mode.flags = 0;
        uint32_t fb_handles[2];
        uint32_t fb_ids[2];
        uint32_t* data_ptrs[2];
        uint32_t pitch;
        uint64_t fb_size;
        uint32_t width=mode.hdisplay, height=mode.vdisplay;
        for (int i =0; i < 2; i++) {
            uint32_t fb_id=0;
            uint32_t offset=0;
            uint32_t fb_handle{};
            int ret = amdgpu_bo_export(parent::get_amdgpu_bo(), amdgpu_bo_handle_type_kms, &fb_handle);
            if (ret < 0) {
                throw std::runtime_error{"amdgpu_bo_export failed"};
            }
            res = drmModeAddFB(fd, width, height, 24, 32, pitch, fb_handle, &fb_ids[i]);
            if (res != 0) {
                std::cerr << std::format("drmModAddFB failed: {}", res) << std::endl;
            }
            assert(fb_id != -1);
        }

        uint32_t connector_id = connected_connector_id;
        res = drmModeSetCrtc(fd, connected_crtc_id, fb_ids[0],
                0, 0, &connector_id, 1, &mode);
        if (res != 0) {
            fprintf(stderr, "drmModeSetCrtc failed:%d\n", res);
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

        thrd_sleep(&sleep_time, NULL);

        res = drmModeSetCrtc(fd, connected_crtc_id, previous_fb_id,
                0, 0, &connector_id, 1, &mode);
    }
private:
};

int main(int argc, const char* argv[]){
    auto drm_test =
        add_drm_test<
        add_amdgpu_bo<
        add_amdgpu_device<
        add_drm_fd<
        cpp_helper::empty_class
        >>>>
        {cpp_helper::empty_configure{}};
    drm_test.test();
    return 0;
}
