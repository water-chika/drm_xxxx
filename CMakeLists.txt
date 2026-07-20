cmake_minimum_required(VERSION 3.20)

project(drm_helper)

add_library(drm_helper PUBLIC
    drm_helper.hpp
    drm_helper.cpp
)

add_executable(amdgpu_drm_test
    amdgpu_test.cpp
)
