#include "parallelpix/benchmark/backend.hpp"

#include <memory>

namespace parallelpix::benchmark {

std::unique_ptr<IBackendExecutor> make_cuda_executor()
{
    return nullptr;
}

}  // namespace parallelpix::benchmark
