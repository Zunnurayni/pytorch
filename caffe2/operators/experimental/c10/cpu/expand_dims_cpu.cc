#include <ATen/core/dispatch/KernelRegistration.h>
#include "caffe2/operators/experimental/c10/schemas/expand_dims.h"
#include "caffe2/utils/math.h"
#include "caffe2/core/tensor.h"

using caffe2::Tensor;

namespace caffe2 {
namespace {

struct State final : public c10::KernelState {
  std::vector<int64_t> dims;
  bool initialized = false;
};

template <class DataType>
void expand_dims_op_cpu_impl(
    const at::Tensor& input_,
    const at::Tensor& output_,
    ArrayRef<int64_t> dims,
    State* state) {
  Tensor input{C10Tensor(input_)};
  Tensor output{C10Tensor(output_)};

  if (!state->initialized) {
    state->dims = dims.vec();
    auto originalSize = state->dims.size();
    CAFFE_ENFORCE(originalSize > 0, "Parameter `dims` must be provided.");
    std::sort(state->dims.begin(), state->dims.end());
    state->dims.erase(
        std::unique(state->dims.begin(), state->dims.end()), state->dims.end());
    if (state->dims.size() < originalSize) {
      LOG(WARNING) << "Parameter `dims` has repeated dimensions.";
    }
    CAFFE_ENFORCE(
        state->dims.front() >= 0, "Dimension ids must be non-negative.");
    state->initialized = true;
  }

  output.CopyFrom(input);
  if (state->dims.empty()) {
    return;
  }

  auto newDims = input.sizes().vec();
  CAFFE_ENFORCE_GE(
      input.sizes().size() + state->dims.size(),
      state->dims.back() + 1,
      "Input needs at least ",
      (1 + state->dims.back() - state->dims.size()),
      " dimensions given `dims`.");
  for (const auto dim : state->dims) {
    newDims.insert(newDims.begin() + dim, 1);
  }
  output.Reshape(newDims);
}
} // namespace
} // namespace caffe2

namespace c10 {
C10_REGISTER_KERNEL(caffe2::ops::ExpandDims)
    .withState<caffe2::State>()
    .kernel<&caffe2::expand_dims_op_cpu_impl<float>>()
    .dispatchKey(CPUTensorId());
} // namespace c10
