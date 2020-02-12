/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "operators/kernel/feed_kernel.h"

namespace paddle_mobile_lens {
namespace operators {

template <>
bool FeedKernel<FPGA, float>::Init(FeedParam<FPGA> *param) {
  auto output = param->Out();
  if (output->dims().size() != 4) {
    output->init(type_id<float>().hash_code());
    return true;
  }
  fpga::format_ofm(output);
  return true;
}

template <>
void FeedKernel<FPGA, float>::Compute(const FeedParam<FPGA> &param) {
  auto output = param.Out();
  int col = param.Col();
  auto input = const_cast<LoDTensor *>(&param.InputX()->at(col));
  if (output->dims().size() != 4) {
    size_t size = output->numel() * sizeof(float);
    auto output_ptr = output->data<float>();
    auto input_ptr = input->data<float>();
    auto external_ptr = reinterpret_cast<float *>(input->external_data);
    float *p_data = external_ptr == nullptr ? input_ptr : external_ptr;
    memcpy(output_ptr, p_data, size);
    input->external_data = nullptr;
    return;
  }
  fpga::format_image(input);

  auto output_ptr = output->data<int8_t>();
  int channel = output->dims()[1];
  int height = output->dims()[2];
  int width = output->dims()[3];
  int size = fpga::align_to_x(channel * width, IMAGE_ALIGNMENT) * height;
  auto input_ptr = input->data<int8_t>();
  fpga::fpga_invalidate(input_ptr, size * sizeof(int8_t));
  memcpy(output_ptr, input_ptr, size * sizeof(int8_t));

  fpga::fpga_flush(output_ptr,
                   fpga::align_to_x(channel * width, IMAGE_ALIGNMENT) * height *
                       sizeof(int8_t));
}
template class FeedKernel<FPGA, float>;

}  // namespace operators
}  // namespace paddle_mobile_lens
