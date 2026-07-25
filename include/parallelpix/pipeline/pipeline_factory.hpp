#pragma once

#include "parallelpix/controller/controller.hpp"

#include <memory>

namespace parallelpix::m2 {

std::unique_ptr<IBenchmarkPipeline> make_benchmark_pipeline();

}  // namespace parallelpix::m2
