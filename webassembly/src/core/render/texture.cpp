#include "texture.h"

namespace core::render {

Texture::Ptr Texture::New(const Image* image) {
    if (image == nullptr) {
        return nullptr;
    }

    Ptr texture(new Texture());
    texture->createTexture();
    texture->setTextureFromImage(image);
    return texture;
}

Texture::~Texture() {
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
    }
}

void Texture::Bind() const {
    glBindTexture(GL_TEXTURE_2D, texture_);
}

void Texture::setFilter(uint32_t minFilter, uint32_t magFilter) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void Texture::setWrap(uint32_t sWrap, uint32_t tWrap) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
}

void Texture::createTexture() {
    glGenTextures(1, &texture_);
    Bind();
    setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

void Texture::setTextureFromImage(const Image* image) {
    GLenum format = GL_RGBA;
    switch (image->GetChannelCount()) {
    case 1:
        format = GL_RED;
        break;
    case 2:
        format = GL_RG;
        break;
    case 3:
        format = GL_RGB;
        break;
    default:
        break;
    }

    width_ = image->GetWidth();
    height_ = image->GetHeight();
    format_ = format;
    type_ = GL_UNSIGNED_BYTE;

    glTexImage2D(GL_TEXTURE_2D, 0, format_, width_, height_, 0,
                 format, type_, image->GetData());
    glGenerateMipmap(GL_TEXTURE_2D);
}

} // namespace core::render
