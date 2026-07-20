#pragma once

#include <cpp_helper.hpp>
#include <stdexcept>

#include <libdrm/drm.h>
#include <libdrm/amdgpu.h>

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
        close(fd);
    }
    auto get_drm_fd() {
        return fd;
    }
    static int open_drm() {
        int fd = open("/dev/dri/card1", O_RDWR);
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
        amdgpu_device_handle handle;
        int ret = amdgpu_device_initialize(fd,
                &major_version, &minor_version, &handle);
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
            .alloc_size = ?,
            .phys_alignment = ?,
            .preferred_heap = ?,
            .flags = ?,
        };

        amdgpu_bo_alloc(dev, &alloc_request, &handle);

        return handle;
    }
private:
    amdgpu_bo_handle bo_handle;
};
}
