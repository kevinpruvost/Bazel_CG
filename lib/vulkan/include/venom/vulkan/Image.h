///
/// Project: VenomEngine
/// @file Image.h
/// @date Sep, 24 2024
/// @brief 
/// @author Pruvost Kevin | pruvostkevin (pruvostkevin0@gmail.com)
///
#pragma once

#include <venom/vulkan/Debug.h>

namespace venom
{
namespace vulkan
{
class Image
{
public:
    Image();
    ~Image();
    Image(const Image& image) = delete;
    Image& operator=(const Image& image) = delete;
    Image(Image&& image) noexcept;
    Image& operator=(Image&& image) noexcept;

    vc::Error Load(unsigned char* pixels, int width, int height, int channels,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    VkImage GetVkImage() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

private:
    VkImage __image;
    VkDeviceMemory __imageMemory;
    uint32_t __width, __height;
};
}
}
