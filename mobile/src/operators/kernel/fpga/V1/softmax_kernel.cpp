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

#ifdef SOFTMAX_OP

#include "operators/kernel/softmax_kernel.h"
#include "operators/kernel/central-arm-func/softmax_arm_func.h"

namespace paddle_mobile_lens {
namespace operators {

template <>
bool SoftmaxKernel<FPGA, float>::Init(SoftmaxParam<FPGA> *param) {
  auto input = const_cast<LoDTensor *>(param->InputX());
  auto dims = framework::vectorize(input->dims());
  half *input_ptr;
  auto out = param->Out();
  if (input->type() == type_id<float>()) {
    out->Resize(framework::make_ddim(dims));
    out->mutable_data<float>(framework::make_ddim(dims));
  } else {
    input_ptr = input->data<half>();
  }

  auto float_input = new LoDTensor;

  int input_n = 1, input_c = 1, input_h = 1, input_w = 1;
  if (dims.size() == 4) {
    input_h = dims[1];
    input_w = dims[2];
    input_c = dims[3];
    if (input_c == 1) {  // This input is generated by FC op, dims = [N C 1 1]
      PADDLE_MOBILE_ENFORCE(input_w == 1, "Softmax input must come from FC op");
      input_c = dims[1];
      input_h = 1;
    }
  } else if (dims.size() == 2) {
    input_c = dims[1];
  }
  input->Resize(framework::make_ddim(dims));
  float_input->Resize(framework::make_ddim(dims));

  if (input_c == 2 && input->type() == type_id<half>()) {  // Use FPGA
    fpga::format_fp16_ofm(out);
    fpga::BypassArgs args = {fpga::DATA_TYPE_FP16};
    args.input_layout_type = fpga::LAYOUT_HWC;
    args.output_layout_type = fpga::LAYOUT_CHW;
    args.input_data_type = fpga::DATA_TYPE_FP16;
    args.output_data_type = fpga::DATA_TYPE_FP16;
    args.image.address = input_ptr;
    args.image.height = input_h;
    args.image.width = input_w;
    args.image.channels = input_c;
    args.output.address = out->data<half>();
    args.output.scale_address = out->scale;
    args.output.activation.activation_type = fpga::SOFTMAX;
    param->SetFpgaArgs(args);
  } else {  // Use CPU
    out->Resize(framework::make_ddim(dims));
    out->mutable_data<float>(framework::make_ddim(dims));
    float_input->init(type_id<float>().hash_code());
    float_input->mutable_data<float>(framework::make_ddim(dims));
    fpga::format_fp32_ofm(float_input);
    fpga::format_fp32_ofm(out);

    fpga::BypassArgs args = {fpga::DATA_TYPE_FP16};
    args.input_layout_type = fpga::LAYOUT_HWC;
    args.output_layout_type = fpga::LAYOUT_CHW;
    args.input_data_type = fpga::DATA_TYPE_FP16;
    args.output_data_type = fpga::DATA_TYPE_FP32;
    args.image.address = input_ptr;
    args.image.height = input_h;
    args.image.width = input_w;
    args.image.channels = input_c;
    args.output.address = float_input->data<float>();
    args.output.scale_address = float_input->scale;
    param->SetFloatInput(float_input);
    param->SetFpgaArgs(args);
  }

  return true;
}

template <>
void SoftmaxKernel<FPGA, float>::Compute(const SoftmaxParam<FPGA> &param) {
  auto *in_x = (param.InputX());
  auto dims = in_x->dims();
  auto n = 1;
  auto h = 1;
  auto w = 1;
  auto c = 1;
  if (dims.size() == 4) {
    h = dims[1];
    w = dims[2];
    c = dims[3];
    if (c == 1) {  // This input is generated by FC op, dims = [N C 1 1]
      PADDLE_MOBILE_ENFORCE(w == 1, "Softmax input must come from FC op");
      c = dims[1];
      h = 1;
    }
  } else if (dims.size() == 2) {
    c = dims[1];
  }
  if (in_x->type() == type_id<half>()) {
    fpga::PerformBypass(param.FpgaArgs());
    if (param.FpgaArgs().output.activation.activation_type != fpga::SOFTMAX) {
      Tensor *out = param.Out();
      Tensor *in_x2 = param.FloatInput();

      fpga::fpga_invalidate(in_x2->data<float>(),
                            in_x2->numel() * sizeof(float));
      math::SoftmaxFuntor<CPU, float>()(in_x2, out);
      fpga::fpga_flush(out->data<float>(), out->memory_size());
    }
  } else {
    if (param.FpgaArgs().output.activation.activation_type != fpga::SOFTMAX) {
      Tensor *out = param.Out();
      out->Resize({n, h, w, c});
      math::SoftmaxFuntor<CPU, float>()(in_x, out);
    }
  }
}

}  // namespace operators
}  // namespace paddle_mobile_lens

#endif
