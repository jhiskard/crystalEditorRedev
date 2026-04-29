#pragma once

#include <cstdint>
#include <memory>

namespace core::render {

class Image {
public:
    using Ptr = std::unique_ptr<Image>;

    static Ptr New(const char* filePath, bool flipVertical = true);
    ~Image();

    const uint8_t* GetData() const { return data_; }
    int32_t GetWidth() const { return width_; }
    int32_t GetHeight() const { return height_; }
    int32_t GetChannelCount() const { return channelCount_; }

private:
    Image() = default;

    bool loadWithStb(const char* filePath, bool flipVertical);

    int32_t width_ = 0;
    int32_t height_ = 0;
    int32_t channelCount_ = 0;
    uint8_t* data_ = nullptr;
};

} // namespace core::render
