/*******************************************************************************
* Copyright (c) 2018-2020 Cadence Design Systems, Inc.
* 
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to use this Software with Cadence processor cores only and 
* not with any other processors and platforms, subject to
* the following conditions:
* 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************/
//Do not modify
//Generated by ./examples_hifi.py using ../examples/l2_normalization_large_relaxed.example.cpp

std::vector<MixedTypedExample>& get_examples() {

    Float32Operands inputs_float, outputs_float;
    Int32Operands inputs_int32, outputs_int32;
    Quant8Operands inputs_quant8, outputs_quant8;
    MixedTyped inputs, outputs;
    MixedTypedExample eg;
    static std::vector<MixedTypedExample> examples;

    inputs_float[0] = {0, 3, 4, 0, 5, 12, 0, 8, 15, 0, 7, 24};

    outputs_float[0] = {0, 0.6f, 0.8f, 0, 0.38461539149284363f, 0.9230769872665405f, 0, 0.47058823704719543f, 0.8823529481887817f, 0, 0.28f, 0.96f};
    inputs = std::make_tuple(inputs_float, inputs_int32, inputs_quant8);
    outputs = std::make_tuple(outputs_float, outputs_int32, outputs_quant8);

    eg = std::make_pair(inputs, outputs);

    examples.push_back(eg);
    inputs_float.clear();
    outputs_float.clear();
    inputs_int32.clear();
    outputs_int32.clear();
    inputs_quant8.clear();
    outputs_quant8.clear();

    return examples;
};
