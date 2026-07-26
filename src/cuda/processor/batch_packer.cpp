#include "processor/batch_packer.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace parallelpix::cuda::detail {
namespace {

std::size_t checked_add(std::size_t left, std::size_t right)
{
    if (right > std::numeric_limits<std::size_t>::max() - left)
    {
        throw std::overflow_error(
            "Packed CUDA input size exceeds addressable memory.");
    }
    return left + right;
}

}  // namespace

std::size_t checked_byte_count(
    std::size_t item_bytes,
    std::size_t item_count)
{
    if (item_bytes != 0 &&
        item_count >
            std::numeric_limits<std::size_t>::max() / item_bytes)
    {
        throw std::overflow_error(
            "CUDA batch size exceeds addressable memory.");
    }
    return item_bytes * item_count;
}

void pack_chunk(
    const std::vector<Image>& images,
    const std::vector<CropRegion>& crops,
    std::size_t begin,
    std::size_t count,
    std::vector<std::uint8_t>& packed_input,
    std::vector<DeviceImageDescriptor>& descriptors)
{
    if (begin > images.size() ||
        count > images.size() - begin ||
        images.size() != crops.size())
    {
        throw std::invalid_argument(
            "CUDA chunk range does not match the prepared image batch.");
    }

    std::size_t required_bytes = 0;
    for (std::size_t offset = 0; offset < count; ++offset)
    {
        required_bytes = checked_add(
            required_bytes, images[begin + offset].pixels.size());
    }

    packed_input.clear();
    descriptors.clear();
    packed_input.reserve(required_bytes);
    descriptors.reserve(count);

    for (std::size_t offset = 0; offset < count; ++offset)
    {
        const auto index = begin + offset;
        const auto& image = images[index];
        const auto& crop = crops[index];
        descriptors.push_back({
            static_cast<std::uint64_t>(packed_input.size()),
            image.width,
            image.height,
            static_cast<std::uint64_t>(image.stride),
            crop.x,
            crop.y,
            crop.width,
            crop.height,
        });
        packed_input.insert(
            packed_input.end(),
            image.pixels.begin(),
            image.pixels.end());
    }
}

Image make_output_image(
    const Image& input,
    const ProcessingConfig& config,
    const std::uint8_t* pixels,
    std::size_t pixel_count)
{
    Image output;
    output.width = config.output_width;
    output.height = config.output_height;
    output.channels = image_channel_count;
    output.stride =
        static_cast<std::size_t>(output.width) * output.channels;
    output.source_path = input.source_path;
    output.pixels.assign(pixels, pixels + pixel_count);
    return output;
}

std::size_t output_image_bytes(const ProcessingConfig& config)
{
    return static_cast<std::size_t>(config.output_width) *
        config.output_height * image_channel_count;
}

}  // namespace parallelpix::cuda::detail
