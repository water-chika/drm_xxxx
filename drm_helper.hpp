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
    add_amdgpu_device(const configure auto& conf) : parent{conf} {
    }
private:
    amdgpu_device_handle device_handle;
};
}
