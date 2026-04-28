#include "image.h"
#include "config/log_config.h"

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


Image::~Image() {
    if (m_Data) {
        stbi_image_free(m_Data);
        m_Data = nullptr;
    }
}

ImageUPtr Image::New(const char* filePath, bool flipVertical) {
    ImageUPtr image = ImageUPtr(new Image());
    if (!image->loadWithStb(filePath, flipVertical)) {
        return nullptr;
    }
    return std::move(image);
}

bool Image::loadWithStb(const char* filepath, bool flipVertical) {
    stbi_set_flip_vertically_on_load(flipVertical);  // Make image upside down    
    m_Data = stbi_load(filepath, &m_Width, &m_Height, &m_ChannelCount, 0);

    if (!m_Data) {
        SPDLOG_ERROR("failed to load image: {}", filepath);
        return false;
    }
    return true;
}