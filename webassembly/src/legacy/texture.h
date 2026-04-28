#pragma once

#include "macro/ptr_macro.h"
#include "image.h"

// GLFW
#define GLFW_INCLUDE_ES3    // Include OpenGL ES 3.0 headers
#define GLFW_INCLUDE_GLEXT  // Include to OpenGL ES extension headers
#include <GLFW/glfw3.h>


DECLARE_PTR(Texture)
class Texture {
public:
    static TextureUPtr New(const Image* image);

    ~Texture();

    uint32_t Get() const { return m_Texture; }
    int32_t GetWidth() const { return m_Width; }
    int32_t GetHeight() const { return m_Height; }

    void Bind() const;

private:
    Texture() = default;

    uint32_t m_Texture { 0 };
    int32_t m_Width { 0 };
    int32_t m_Height { 0 };
    uint32_t m_Format { GL_RGBA };
    uint32_t m_Type { GL_UNSIGNED_BYTE };

    void setFilter(uint32_t minFilter, uint32_t magFilter) const;    
    void setWrap(uint32_t sWrap, uint32_t tWrap) const;
    void createTexture();
    void setTextureFromImage(const Image* image);
};