#pragma once

// importted library
#include <cpp_helper.hpp>

// C++ std header
#include <stdexcept>
#include <iostream>

// Unix C header
#include <fcntl.h>
#include <unistd.h>

// DRM header
#include <libdrm/drm.h>
#include <libdrm/amdgpu.h>
#include <xf86drm.h>

namespace drm_helper {

using cpp_helper::configure;
using cpp_helper::empty_configure;

template<typename T>
class add_drm_fd : public T {
public:
    using parent = T;
    add_drm_fd(const configure auto& conf) : parent{conf},
        fd{open_drm()}
    {
    }
    ~add_drm_fd() {
        drmClose(fd);
    }
    auto get_drm_fd() {
        return fd;
    }
    static int open_drm() {
        int fd = drmOpenWithType("amdgpu", nullptr, DRM_NODE_PRIMARY);
        if (fd == -1) {
            throw std::runtime_error{"faild to open drm device"};
        }
        return fd;
    }
private:
    int fd;
};
template<typename T>
class add_amdgpu_device : public T {
public:
    using parent = T;
    add_amdgpu_device(const configure auto& conf) : parent{conf},
        device_handle{initialize_amdgpu_device()}
    {
    }
    ~add_amdgpu_device() {
        amdgpu_device_deinitialize(device_handle);
    }
    amdgpu_device_handle initialize_amdgpu_device() {
        int fd = parent::get_drm_fd();
        amdgpu_device_handle handle{};
        uint32_t major_version{}, minor_version{};
        int ret = amdgpu_device_initialize(fd,
                &major_version, &minor_version, &handle);
        if (ret < 0) {
            throw std::runtime_error{"amdgpu device initialize failed"};
        }
        std::clog << "amdgpu device version: " << major_version << "." << minor_version << std::endl;
        return handle;
    }
    auto get_amdgpu_device() {
        return device_handle;
    }
private:
    amdgpu_device_handle device_handle;
};
template<typename T>
class cache_heap_infos : public T {
public:
    using parent = T;
};
template<typename T>
class add_amdgpu_bo : public T {
public:
    using parent = T;
    add_amdgpu_bo(const configure auto& conf) : parent{conf},
        bo_handle{allocate_amdgpu_bo()}
    {}
    ~add_amdgpu_bo() {
        amdgpu_bo_free(bo_handle);
    }
    auto allocate_amdgpu_bo() {
        auto dev = parent::get_amdgpu_device();
        amdgpu_bo_handle handle;

        amdgpu_bo_alloc_request alloc_request{
            .alloc_size = 0,
            .phys_alignment = 0,
            .preferred_heap = 0,
            .flags = 0,
        };

        amdgpu_bo_alloc(dev, &alloc_request, &handle);

        return handle;
    }
private:
    amdgpu_bo_handle bo_handle;
};
}
