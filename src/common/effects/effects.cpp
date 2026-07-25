#include "parallelpix/common/processing.hpp"

#include <cmath>
#include <cstdint>

namespace parallelpix {

std::uint8_t round_and_clamp(double value) noexcept
{
    if (!std::isfinite(value) || value <= 0.0)
    {
        return 0;
    }
    if (value >= 255.0)
    {
        return 255;
    }
    return static_cast<std::uint8_t>(std::floor(value + 0.5));
}

std::uint8_t apply_brightness(
    std::uint8_t value,
    double brightness_factor) noexcept
{
    return round_and_clamp(
        static_cast<double>(value) * brightness_factor);
}

std::uint8_t blend_watermark_channel(
    std::uint8_t base,
    std::uint8_t watermark,
    std::uint8_t alpha,
    double global_opacity) noexcept
{
    const auto effective_alpha =
        static_cast<double>(alpha) / 255.0 * global_opacity;
    return round_and_clamp(
        static_cast<double>(base) * (1.0 - effective_alpha) +
        static_cast<double>(watermark) * effective_alpha);
}

}  // namespace parallelpix
