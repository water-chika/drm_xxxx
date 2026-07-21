#include "drm_helper.hpp"

using namespace drm_helper;

int main() {
    auto app =
        add_amdgpu_bo<
        cache_heap_infos<
        cache_gpu_info<
        add_amdgpu_device<
        add_drm_fd<
        cpp_helper::empty_class
        >>>>>
        {cpp_helper::empty_configure{}};
    return 0;
}
