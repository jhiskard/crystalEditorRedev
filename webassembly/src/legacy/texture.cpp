#include "texture.h"


TextureUPtr Texture::New(const Image* image) {
    TextureUPtr texture = TextureUPtr(new Texture());
    texture->createTexture();
    texture->setTextureFromImage(image);
    return std::move(texture);
}

Texture::~Texture() {
    if (m_Texture) {
        glDeleteTextures(1, &m_Texture);
    }
}

void Texture::Bind() const {
    glBindTexture(GL_TEXTURE_2D, m_Texture);
}

void Texture::setFilter(uint32_t minFilter, uint32_t magFilter) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void Texture::setWrap(uint32_t sWrap, uint32_t tWrap) const {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);  // X-direction of texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);  // Y-direction of texture
}

void Texture::createTexture() {
    glGenTextures(1, &m_Texture);
    // Bind and set default filter and wrap option
    Bind();
    setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // To use mipmap, the first argument shoule be "GL_LINEAR_MIPMAP_LINEAR".
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

    m_Width = image->GetWidth();
    m_Height = image->GetHeight();
    m_Format = format;  // Internal format and image format should be identical for WebGL.
    m_Type = GL_UNSIGNED_BYTE;

    // Copy image data to GPU
    // GL_RGBA: If image->GetChannelCount() is 3, alpha channel is automatically filled with 255.
    glTexImage2D(GL_TEXTURE_2D,
        0, m_Format, m_Width, m_Height,  // Describe data on GPU. 0: Mipmap level
        0, format, m_Type, image->GetData());  // Describe data on CPU. 0: Border size

    // Generate mipmap
    // - Use small texture image for small window size
    // - Require small size texture images. Its size becomes half for the next mipmap level. 
    // - Mipmap level 0's width and height(1024x1024) are twice of 1's width and height.
    glGenerateMipmap(GL_TEXTURE_2D);
}