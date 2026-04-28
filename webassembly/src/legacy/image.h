#pragma once

#include "macro/ptr_macro.h"

// Standard library
#include <memory>
#include <string>


DECLARE_PTR(Image)
class Image {
public:
    static ImageUPtr New(const char* filePath, bool flipVertical = true);

    ~Image();

    const uint8_t* GetData() const { return m_Data; }
    int32_t GetWidth() const { return m_Width; }
    int32_t GetHeight() const { return m_Height; }
    int32_t GetChannelCount() const { return m_ChannelCount; }

private:
    Image() = default;

    // Exponential form of 2 is the most efficient for texture size.
    int32_t m_Width { 0 };
    int32_t m_Height { 0 };
    int32_t m_ChannelCount { 0 };
    uint8_t* m_Data { nullptr };

    bool loadWithStb(const char* filepath, bool flipVertical);
};