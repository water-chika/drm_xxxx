#include "drm_helper.hpp"

using namespace drm_helper;

struct conf : public cpp_helper::empty_configure {
    const char* drm_device_path;
};

int main(int argc, const char** argv) {
    if (argc < 2) {
        throw std::runtime_error{std::format("Usage: {} <drm_device_path>", argc > 0 ? argv[0] : "test")};
    }
    auto drm_device_path = argv[1];
    auto app =
        cache_heap_infos<
        cache_gpu_info<
        add_amdgpu_device<
        add_drm_fd<
        cpp_helper::empty_class
        >>>>
        {conf{.drm_device_path = drm_device_path}};
    return 0;
}
