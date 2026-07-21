cmake_minimum_required(VERSION 3.20)

project(drm_helper)

add_library(drm_helper
    drm_helper.hpp
    drm_helper.cpp
)

target_link_libraries(drm_helper PUBLIC
	cpp_helper
    drm
)
target_include_directories(drm_helper PUBLIC
    /usr/include/drm
)

add_executable(amdgpu_drm_test
    amdgpu_test.cpp
)
target_link_libraries(amdgpu_drm_test PUBLIC
	drm_helper
    drm_amdgpu
)

