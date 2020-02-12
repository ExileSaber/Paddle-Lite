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

#ifdef FUSION_DEQUANT_ADD_BN_RELU_OP

#include "operators/fusion_dequant_add_bn_relu_op.h"

namespace paddle_mobile_lens {
namespace operators {

template <typename Dtype, typename T>
void FusionDequantAddBNReluOp<Dtype, T>::InferShape() const {
  const auto& input_dims = this->param_.input_->dims();
  this->param_.output_->Resize(input_dims);
}

}  // namespace operators
}  // namespace paddle_mobile_lens

namespace ops = paddle_mobile_lens::operators;
REGISTER_FUSION_MATCHER(fusion_dequant_add_bn_relu,
                        ops::FusionDequantAddBNReluMatcher);

#ifdef PADDLE_MOBILE_CPU
REGISTER_OPERATOR_CPU(fusion_dequant_add_bn_relu,
                      ops::FusionDequantAddBNReluOp);
#endif

#endif
