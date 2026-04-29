#include "image.h"

#include "config/log_config.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace core::render {

Image::~Image() {
    if (data_ != nullptr) {
        stbi_image_free(data_);
        data_ = nullptr;
    }
}

Image::Ptr Image::New(const char* filePath, bool flipVertical) {
    Ptr image(new Image());
    if (!image->loadWithStb(filePath, flipVertical)) {
        return nullptr;
    }
    return image;
}

bool Image::loadWithStb(const char* filePath, bool flipVertical) {
    stbi_set_flip_vertically_on_load(flipVertical);
    data_ = stbi_load(filePath, &width_, &height_, &channelCount_, 0);
    if (!data_) {
        SPDLOG_ERROR("Failed to load image: {}", filePath);
        return false;
    }
    return true;
}

} // namespace core::render
