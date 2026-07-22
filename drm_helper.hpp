#pragma once

// importted library
#include <cpp_helper.hpp>

// C++ std header
#include <stdexcept>
#include <iostream>
#include <format>
#include <cinttypes>

// Unix C header
#include <fcntl.h>
#include <unistd.h>

// DRM header
#include <libdrm/drm.h>
#include <libdrm/amdgpu.h>
#include <libdrm/amdgpu_drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace drm_helper {

using cpp_helper::configure;
using cpp_helper::empty_configure;

template<typename Conf>
concept contain_drm_device_path = requires(Conf c) {
    c.drm_device_path;
};

template<typename T>
class add_drm_fd : public T {
public:
    using parent = T;
    template<configure Conf>
        requires contain_drm_device_path<Conf>
    add_drm_fd(const Conf& conf) : parent{conf},
        fd{open_drm(conf.drm_device_path)}
    {
    }
    ~add_drm_fd() {
        close(fd);
    }
    auto get_drm_fd() {
        return fd;
    }
    static int open_drm(const char* drm_device_path) {
        int fd = open(drm_device_path, O_RDWR);
        if (fd == -1) {
            throw std::runtime_error{"faild to open drm device"};
        }
        return fd;
    }
private:
    int fd;
};

template<typename T>
class cache_drm_resources : public T {
public:
    using parent = T;
    cache_drm_resources(const configure auto& conf) : parent{conf},
        resources{query_resources()}
    {}
    ~cache_drm_resources() {
        drmModeFreeResources(resources);
    }
    auto get_drm_resources() {
        return resources;
    }
private:
    auto query_resources() {
        auto fd = parent::get_drm_fd();
        drmModeRes* resources = drmModeGetResources(fd);
        if (resources == nullptr) {
            throw std::runtime_error{"drmModeGetResources failed"};
        }
        return resources;
    }
    drmModeRes* resources;
};

template<typename T>
class select_first_connected_connector : public T {
public:
    using parent = T;
    select_first_connected_connector(const configure auto& conf) : parent{conf},
        connector{find_first_connected_connector()}
    {}
    ~select_first_connected_connector() {
        if (connector != nullptr) {
            drmModeFreeConnector(connector);
            connector = nullptr;
        }
    }
    auto get_drm_connector() {
        return connector;
    }
    auto get_drm_mode() {
        return &connector->modes[0];
    }
private:
    drmModeConnector* find_first_connected_connector() {
        auto fd = parent::get_drm_fd();
        auto resources = parent::get_drm_resources();
        for (int i = 0; i < resources->count_connectors; i++) {
            uint32_t connector_id = resources->connectors[i];
            drmModeConnector* connector = drmModeGetConnector(fd, connector_id);
            if (connector->connection == DRM_MODE_CONNECTED){
                return connector;
            }
            drmModeFreeConnector(connector);
        }
        return nullptr;
    }
    drmModeConnector* connector;
};

template<typename T>
class select_default_encoder : public T {
public:
    using parent = T;
    select_default_encoder(const configure auto& conf) : parent{conf},
        encoder{query_encoder()}
    {}
    ~select_default_encoder() {
        if (encoder != nullptr) {
            drmModeFreeEncoder(encoder);
            encoder = nullptr;
        }
    }
    auto get_drm_encoder() {
        return encoder;
    }
private:
    drmModeEncoder* query_encoder() {
        auto fd = parent::get_drm_fd();
        auto connector = parent::get_drm_connector();
        drmModeEncoder* encoder = drmModeGetEncoder(fd, connector->encoder_id);
        return encoder;
    }
    drmModeEncoder* encoder;
};

template<typename T>
class select_default_crtc : public T {
public:
    using parent = T;
    select_default_crtc(const configure auto& conf) : parent{conf},
        crtc{query_crtc()}
    {}
    ~select_default_crtc() {
        if (crtc != nullptr) {
            drmModeFreeCrtc(crtc);
            crtc = nullptr;
        }
    }
    auto get_drm_crtc() {
        return crtc;
    }
private:
    drmModeCrtc* query_crtc() {
        auto fd = parent::get_drm_fd();
        auto encoder = parent::get_drm_encoder();
        drmModeCrtc* crtc = drmModeGetCrtc(fd, encoder->crtc_id);
        return crtc;
    }
    drmModeCrtc* crtc;
};

template<typename T>
class replace_fb : public T {
public:
    using parent = T;
    replace_fb(const configure auto& conf) : parent{conf},
        previous_fb_id{parent::get_drm_crtc()->buffer_id}
    {
        auto fd = parent::get_drm_fd();
        auto crtc = parent::get_drm_crtc();
        auto connector = parent::get_drm_connector();
        auto mode = parent::get_drm_mode();
        auto fb = parent::get_drm_fb();
        auto connector_id = connector->connector_id;
        int res = drmModeSetCrtc(fd, crtc->crtc_id, fb->fb_id,
                0, 0, &connector_id, 1, mode);
        if (res != 0) {
            throw std::runtime_error{"drmModeSetCrtc failed"};
        }
    }
    ~replace_fb() {
        auto fd = parent::get_drm_fd();
        auto crtc = parent::get_drm_crtc();
        auto connector = parent::get_drm_connector();
        auto mode = parent::get_drm_mode();
        auto connector_id = connector->connector_id;
        int res = drmModeSetCrtc(fd, crtc->crtc_id, previous_fb_id,
                0, 0, &connector_id, 1, mode);
        assert(res == 0);
    }
private:
    uint32_t previous_fb_id;
};

std::ostream& operator<<(std::ostream& out, const drmModeRes& res) {
    out << "fbs count: " << res.count_fbs << std::endl;
    out << "crtcs count: " << res.count_crtcs << std::endl;
    out << "connectors count: " << res.count_connectors << std::endl;
    out << "encoders count: " << res.count_encoders << std::endl;
    out << std::format("min res: {}x{}", res.min_width, res.min_height) << std::endl;
    out << std::format("max res: {}x{}", res.max_width, res.max_height) << std::endl;
}
const char* modeset_connection_to_str(drmModeConnection connection) {
    switch (connection) {
        case DRM_MODE_CONNECTED: return "connected";
        case DRM_MODE_DISCONNECTED: return "disconnected";
        case DRM_MODE_UNKNOWNCONNECTION: return "unknownconnection";
        default: return "unkown value";
    }
}
const char* modeset_sub_pixel_to_str(drmModeSubPixel sub_pixel) {
    switch (sub_pixel) {
        case DRM_MODE_SUBPIXEL_UNKNOWN: return "unknown";
        case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB: return "horizontal RGB";
        case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR: return "horizontal BGR";
        case DRM_MODE_SUBPIXEL_VERTICAL_RGB: return "vertical RGB";
        case DRM_MODE_SUBPIXEL_VERTICAL_BGR: return "vertical BGR";
        case DRM_MODE_SUBPIXEL_NONE: return "none";
    }
}
static const char* modeset_property_flag_to_str(uint32_t flag) {
    switch (flag) {
        case DRM_MODE_PROP_PENDING:
            return "pending";
        case DRM_MODE_PROP_RANGE:
            return "range";
        case DRM_MODE_PROP_IMMUTABLE:
            return "immutable";
        case DRM_MODE_PROP_ENUM:
            return "enum";
        case DRM_MODE_PROP_BLOB:
            return "blob";
        case DRM_MODE_PROP_BITMASK:
            return "bitmask";
        case DRM_MODE_PROP_OBJECT:
            return "object";
        case DRM_MODE_PROP_SIGNED_RANGE:
            return "signed_range";
        case DRM_MODE_PROP_ATOMIC:
            return "atomic";
        default:
            return "unknown";
    }
}

static void modeset_property_print(int fd, uint32_t property_id, uint32_t property_value) {
    drmModePropertyPtr property = drmModeGetProperty(fd, property_id);

    printf("flags: ");
    for (uint32_t i = 0; i < 32; i++) {
        if ((1u<<i) & property->flags) {
            printf("%s | ", modeset_property_flag_to_str(1u<<i));
        }
    }
    printf("\n");

    printf("name: %s\n", property->name);

    if (property->count_values > 0) {
        printf("values: ");
        for (int i = 0; i < property->count_values; i++) {
            std::cout << std::format("{},", property->values[i]);
        }
        printf("\n");
    }

    if (property->count_enums > 0) {
        printf("enums: ");
        for (int i = 0; i < property->count_enums; i++) {
            std::cout << std::format("{}={},", property->enums[i].name, property->enums[i].value);
        }
        printf("\n");
    }

    if (property->count_blobs > 0) {
        printf("count blobs: %d\n", property->count_blobs);
    }

    if (DRM_MODE_PROP_BLOB & property->flags && property_value != 0) {
        uint32_t blob_id = property_value;
        drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(fd, blob_id);
        std::cout << std::format("blob (length={}): ", blob->length);
        uint8_t* data = reinterpret_cast<uint8_t*>(blob->data);
        for (uint32_t i = 0; i < blob->length; i++) {
            printf("%d ", data[i]);
        }
        printf("\n");
        drmModeFreePropertyBlob(blob);
    }

    drmModeFreeProperty(property);
}

auto modeset_connector_print(int fd, drmModeConnector* connector) {
    printf("connector id: %d\n", connector->connector_id);
    printf("encoder id: %d\n", connector->encoder_id);
    printf("connection: %s\n", modeset_connection_to_str(connector->connection));
    std::cout << std::format("width x height: %{}mm x %{}mm\n", connector->mmWidth, connector->mmHeight);
    printf("sub pixel: %s\n", modeset_sub_pixel_to_str(connector->subpixel));

    printf("count of properties: %d\n", connector->count_props);
    for (int i = 0; i < connector->count_props; i++) {
        printf("\n");
        std::cout << std::format("property id: {}\n", connector->props[i]);
        std::cout << std::format("property value: {}\n", connector->prop_values[i]);
        modeset_property_print(fd, connector->props[i], connector->prop_values[i]);
        printf("\n");
    }
}

static void modeset_encoder_print(drmModeEncoder* encoder) {
    printf("encoder id: %d\n", encoder->encoder_id);
    printf("crtc id: %d\n", encoder->crtc_id);
    printf("possible crtcs: %x\n", encoder->possible_crtcs);
    printf("possible clones: %x\n", encoder->possible_clones);
}

static void modeset_mode_info_print(drmModeModeInfo* mode) {
    printf("clock: %d\n", mode->clock);
    std::cout << std::format("hdisplay,hsync_start,hsync_end,htotal,hskew: {}, {}, {}, {}, {}\n",
            mode->hdisplay, mode->hsync_start, mode->hsync_end, mode->htotal, mode->hskew);
    std::cout << std::format("vdisplay,vsync_start,vsync_end,vtotal,vscan: {}, {}, {}, {}, {}\n",
            mode->vdisplay, mode->vsync_start, mode->vsync_end, mode->vtotal, mode->vscan);
    std::cout << std::format("vrefresh: {}\n", mode->vrefresh);
    std::cout << std::format("flags: {}\n", mode->flags);
    std::cout << std::format("type: {}\n", mode->type);
    printf("name: %s\n", mode->name);
}

static void modeset_crtc_print(drmModeCrtc* crtc) {
    printf("crtc id: %d\n", crtc->crtc_id);
    printf("buffer id: %d\n", crtc->buffer_id);
    std::cout << std::format("(x,y): ({},{})\n", crtc->x, crtc->y);
    std::cout << std::format("width x height: {}x{}\n", crtc->width, crtc->height);
    printf("mode valid: %d\n", crtc->mode_valid);
    modeset_mode_info_print(&crtc->mode);
    printf("gamma size: %d\n", crtc->gamma_size);
}

static void modeset_fb_print(drmModeFB* fb) {
    std::cout << std::format("fb id: {}\n", fb->fb_id);
    std::cout << std::format("width x height: {}x{}\n", fb->width, fb->height);
    std::cout << std::format("pitch: {}\n", fb->pitch);
    std::cout << std::format("bpp: {}\n", fb->bpp);
    std::cout << std::format("depth: {}\n", fb->depth);
    std::cout << std::format("handle: {}\n", fb->handle);
}

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
class cache_gpu_info : public T {
public:
    using parent = T;
    cache_gpu_info(const configure auto& conf) : parent{conf},
        info{query_gpu_info()}
    {}
    auto query_gpu_info() {
        amdgpu_gpu_info info{};
        auto dev = parent::get_amdgpu_device();
        amdgpu_query_gpu_info(dev, &info);
        std::cout << "asic id: " << info.asic_id << std::endl;
        std::cout << "chip rev: " << info.chip_rev << std::endl;
        std::cout << "family id: " << info.family_id << std::endl;
        return info;
    }
private:
    amdgpu_gpu_info info;
};
template<typename T>
class cache_heap_infos : public T {
public:
    using parent = T;
    cache_heap_infos(const configure auto& conf) : parent{conf},
        infos{query_heap_infos()}
    {}
    auto query_heap_infos() {
        std::vector<amdgpu_heap_info> infos;
        auto dev = parent::get_amdgpu_device();
        for (int i = 0; ;i++) {
            infos.push_back({});
            int ret = amdgpu_query_heap_info(dev, i, 0, &infos.back());
            if (ret < 0) {
                infos.pop_back();
                break;
            }
            else {
                auto& info = infos.back();
                std::cout << "Heap " << i << ": " << info.heap_size
                    << ", " << info.heap_usage << ", " << info.max_allocation << std::endl;
            }
        }
        return infos;
    }
private:
    std::vector<amdgpu_heap_info> infos;
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

        auto alloc_size = parent::get_bo_alloc_size();

        amdgpu_bo_alloc_request alloc_request{
            .alloc_size = alloc_size,
            .phys_alignment = 4096,
            .preferred_heap = AMDGPU_GEM_DOMAIN_VRAM | AMDGPU_GEM_DOMAIN_GTT,
            .flags = AMDGPU_GEM_CREATE_CPU_GTT_USWC,
        };

        int ret = amdgpu_bo_alloc(dev, &alloc_request, &handle);
        if (ret < 0) {
            throw std::runtime_error{"amdgpu_bo_alloc failed"};
        }

        return handle;
    }
    auto get_amdgpu_bo() {
        return bo_handle;
    }
private:
    amdgpu_bo_handle bo_handle;
};

template<typename T>
class add_fb_with_amdgpu_bo : public T {
public:
    using parent = T;
    add_fb_with_amdgpu_bo(const configure auto& conf) : parent{conf},
        fb{add_fb()}
    {}
    ~add_fb_with_amdgpu_bo() {
        if (fb != nullptr) {
            auto fd = parent::get_drm_fd();
            drmModeRmFB(fd, fb->fb_id);
            drmModeFreeFB(fb);
            fb = nullptr;
        }
    }
    auto get_drm_fb() {
        return fb;
    }
private:
    auto add_fb() {
        uint32_t fb_handle{};
        uint32_t fb_id=0;
        uint64_t fb_size = parent::get_bo_alloc_size();
        auto fd = parent::get_drm_fd();
        auto mode = parent::get_drm_mode();
        auto width = mode->hdisplay, height = mode->vdisplay;
        uint32_t pitch=mode->hdisplay*sizeof(uint32_t);
        int ret = amdgpu_bo_export(parent::get_amdgpu_bo(), amdgpu_bo_handle_type_kms, &fb_handle);
        if (ret < 0) {
            throw std::runtime_error{"amdgpu_bo_export failed"};
        }
        ret = drmModeAddFB(fd, width, height, 24, 32, pitch, fb_handle, &fb_id);
        if (ret != 0) {
            std::cerr << std::format("drmModAddFB failed: {}", ret) << std::endl;
        }
        return drmModeGetFB(fd, fb_id);
    }
    drmModeFB* fb;
};

}
