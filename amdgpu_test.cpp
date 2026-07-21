#include "drm_helper.hpp"

using namespace drm_helper;

int main() {
    auto app =
        add_amdgpu_device<
        add_drm_fd<
        cpp_helper::empty_class
        >>
        {cpp_helper::empty_configure{}};
    return 0;
}
