#pragma once

#include "image.h"

#include <cstdint>
#include <memory>

#define GLFW_INCLUDE_ES3
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

namespace core::render {

class Texture {
public:
    using Ptr = std::unique_ptr<Texture>;

    static Ptr New(const Image* image);
    ~Texture();

    uint32_t Get() const { return texture_; }
    int32_t GetWidth() const { return width_; }
    int32_t GetHeight() const { return height_; }

    void Bind() const;

private:
    Texture() = default;

    void setFilter(uint32_t minFilter, uint32_t magFilter) const;
    void setWrap(uint32_t sWrap, uint32_t tWrap) const;
    void createTexture();
    void setTextureFromImage(const Image* image);

    uint32_t texture_ = 0;
    int32_t width_ = 0;
    int32_t height_ = 0;
    uint32_t format_ = GL_RGBA;
    uint32_t type_ = GL_UNSIGNED_BYTE;
};

} // namespace core::render
