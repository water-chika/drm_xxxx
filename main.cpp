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

#include <hip_helper.hpp>

using namespace drm_helper;

template<typename T>
class set_bo_alloc_size : public T {
public:
    using parent = T;
    set_bo_alloc_size(const configure auto& conf) : parent{conf} {
    }

    auto get_bo_alloc_size() {
        auto mode = parent::get_drm_mode();
        return mode->hdisplay*mode->vdisplay*sizeof(uint32_t);
    }
};


template<typename T>
class add_drm_test : public T {
public:
    using parent = T;
    add_drm_test(const configure auto& conf) : parent{conf}
    {}
    void test() {
        int fd = parent::get_drm_fd();
        int ret= 0;
        drmModeRes* resources = parent::get_drm_resources();

        auto connector = parent::get_drm_connector();

        auto encoder = parent::get_drm_encoder();
        auto crtc = parent::get_drm_crtc();
        auto fb = parent::get_drm_fb();

        ret = drmSetMaster(fd);
        if (ret != 0) {
            fprintf(stderr, "drmSetMaster failed\n");
        }
        auto mode = parent::get_drm_mode();
        uint32_t width=mode->hdisplay, height=mode->vdisplay;

        hipExternalMemory_t hip_memory{};
        {
            uint32_t dma_buf_fd{};
            int ret = amdgpu_bo_export(parent::get_amdgpu_bo(), amdgpu_bo_handle_type_dma_buf_fd, &dma_buf_fd);
            if (ret < 0) {
                throw std::runtime_error{"amdgpu_bo_export failed"};
            }
            hipExternalMemoryHandleDesc desc = {};
            desc.size = parent::get_bo_alloc_size();
            desc.type = hipExternalMemoryHandleTypeOpaqueFd;
            desc.handle.fd = dma_buf_fd;
            ret = hipImportExternalMemory(&hip_memory, &desc);
            if (ret != hipSuccess) {
                throw std::runtime_error{std::format("hipImportExternalMemory failed: {}", ret)};
            }
        }
        uint32_t* ptr;
        {
            hipExternalMemoryBufferDesc desc = {};
            desc.offset                      = 0;
            desc.size = parent::get_bo_alloc_size();
            desc.flags                       = 0;

            ret = hipExternalMemoryGetMappedBuffer(reinterpret_cast<void**>(&ptr), hip_memory, &desc);
            if (ret != hipSuccess) {
                throw std::runtime_error{std::format("hipExternalMemoryGetMappedBuffer failed: {}", ret)};
            }
        }
        test_global<<<dim3(8,8,1),dim3(32,1,1),0>>>(ptr, width, height, (width+32*8-1)/(32*8), (height+1*8-1)/(1*8));
        ret = hipDeviceSynchronize();
        if (ret != hipSuccess) {
            throw std::runtime_error{std::format("hipDeviceSynchronize failed: {}", ret)};
        }

        struct timespec sleep_time = {.tv_sec=1};
        thrd_sleep(&sleep_time, NULL);
    }
    __global__
    static void test_global(uint32_t* ptr, uint32_t width, uint32_t height, uint32_t tile_width, uint32_t tile_height) {
        for (int y_ = 0; y_ < tile_height; y_++) {
            int y = y_ + (threadIdx.y+blockIdx.y*blockDim.y)*tile_height;
            if (y >= height) break;
            for (int x_ = 0; x_ < tile_width; x_++) {
                int x = x_ + (threadIdx.x+blockIdx.x*blockDim.x)*tile_width;
                if (x >= width) break;
                ptr[y*width+x] = 0x00ffff00;
            }
        }
    }
private:
};

struct conf : public cpp_helper::empty_configure {
    const char* drm_device_path;
};

int main(int argc, const char* argv[]){
    if (argc < 2) {
        throw std::runtime_error{std::format("Usage: {} <drm_device_path>", argc > 0 ? argv[0] : "test")};
    }
    auto drm_device_path = argv[1];
    auto drm_test =
        add_drm_test<
        replace_fb<
        add_fb_with_amdgpu_bo<
        select_default_crtc<
        add_amdgpu_bo<
        set_bo_alloc_size<
        add_amdgpu_device<
        select_default_encoder<
        select_first_connected_connector<
        cache_drm_resources<
        add_drm_fd<
        cpp_helper::empty_class
        >>>>>>>>>>>
        {conf{.drm_device_path = drm_device_path}};
    drm_test.test();
    return 0;
}
