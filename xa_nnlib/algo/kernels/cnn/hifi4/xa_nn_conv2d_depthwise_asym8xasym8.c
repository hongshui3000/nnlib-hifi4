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
#include "xa_type_def.h"
#include "common.h"
#include "xa_nnlib_kernels_api.h"
#include "xa_nn_conv2d_depthwise_state.h"
#include "xa_nnlib_common_macros.h"
#include "xa_nnlib_err_chk.h"

#define MULTIPLYBYQUANTIZEDMULTIPLIER_X2(inp, multiplier, left_shift, right_shift) \
    inp = AE_SLAA32(inp, left_shift); \
    inp = AE_MULFP32X2RAS(inp, AE_MOVDA32(multiplier)); \
    inp = AE_ROUND32X2F64SSYM(AE_SRAA64(AE_CVT64F32_H(inp), right_shift), AE_SRAA64(AE_CVT64F32_L(inp), right_shift));

/* 2D Convolution implementation */
static inline void conv2d_nchw_asym8xasym8_hf4_convmul
(pWORD8 __restrict__ p_out  /* Output:  [Stream] [(out_stride): (actual_out_height): (actual_out_width)] */
 ,const WORD8 *__restrict__ p_ker  /* Kernel:  [Block] [1:             kernel_height:       kernel_width_pad] */
 ,const WORD8 *__restrict__ p_inp  /* Input:   [Block] [1:             input_height:        input_width] */
 ,WORD32 bias
 ,int input_height
 ,int input_width
 ,int kernel_height
 ,int kernel_width
 ,int actual_out_height      /* This is the actual output height, processing should be limited to it. */
 ,int actual_out_width       /* This is the actual output width, processing should be limited to it. */
 ,int out_stride
 ,int x_stride
 ,int y_stride
 ,WORD32  input_zero_bias
 ,WORD32  kernel_zero_bias
 ,WORD32  out_multiplier
 ,WORD32  out_shift
 ,WORD32  out_zero_bias
 ,pWORD32 __restrict__ p_scratch /* Scratch: [Block] [1:             (actual_out_height): (out_width)] */
 )
{
    /* Importance of actual_out_width, since we are appending zeros input left
     * and right side. No problem with left padding, but for right padding that
     * is done to make sure that input_width is multiple of 4. Here
     * 'output_width_for_x_stride_1' value is calculated based on this padded value. But
     * actually expected output width to pick correct values from 'output_width_for_x_stride_1' on
     * jumps of 'x_stride'. */

    int kernel_width_pad = (kernel_width+3)&(~3);

    /* Generic case */
    int i, j, k, l;
    int output_height = input_height - kernel_height + 1;
    int output_width_for_x_stride_1;

    /* Here input_width is nothing but circ_buf_width, which is taken care to be
     * multiple of 4. */
    output_width_for_x_stride_1 = (1 + ((input_width - kernel_width)/1));
    /* output_width_for_x_stride_1 loop is unrolled by 4 so keeping this dimension to multiple of 4 */
    output_width_for_x_stride_1 = ALIGNED_SIZE(output_width_for_x_stride_1, (ALIGNMENT/2));

    /* Please note that below addition of 1 is done to adjust in C style indices
     * */
    if ((actual_out_height - 1) > ((output_height + 1) / (y_stride)))
    {
        return;
    }
    if ((actual_out_width - 1) > ((output_width_for_x_stride_1 + 1) / (x_stride)))
    {
        return;
    }

    ae_int64 accu_int64_0, accu_int64_1, accu_int64_2, accu_int64_3;
    ae_int32x2 *scratch_ptr = (ae_int32x2 *)p_scratch;

    ae_int16x4 d_input_zero_bias, d_kernel_zero_bias;
    d_input_zero_bias = AE_MOVDA16(input_zero_bias);
    d_kernel_zero_bias = AE_MOVDA16(kernel_zero_bias);
    ae_int32x2 _ae_int32_sat_bias;
    _ae_int32_sat_bias = AE_MOVDA32X2(bias, bias);

    for(i = 0; i < actual_out_height; i++)
    {
        scratch_ptr = (ae_int32x2 *) (p_scratch + (i * output_width_for_x_stride_1));
        for(j = 0; j < (output_width_for_x_stride_1>>2); j++)
        {
            accu_int64_0 = AE_ZERO64();
            accu_int64_1 = AE_ZERO64();
            accu_int64_2 = AE_ZERO64();
            accu_int64_3 = AE_ZERO64();
#pragma loop_count min=1
            for(k = 0; k < kernel_height; k++)
            {
                const WORD8 *pt_inp = (p_inp);
                AE_ADDCIRC16X4_XC
                    ((ae_int16x4 *)pt_inp
                     ,((sizeof(WORD8)) * ((i * y_stride * input_width) + j*4 + k*input_width))
                    );
                const WORD8 *pt_ker = (p_ker + k*kernel_width_pad);
                ae_int16x4 d_inp, d_ker;
                ae_int16x4 d_inp0, d_inp1, d_inp2, d_inp3;
                AE_L8X4F_IP(d_inp0, pt_inp, 4);
                d_inp0 = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_inp0), 8));
                d_inp0 = AE_ADD16(d_inp0, d_input_zero_bias);
#pragma loop_count min=1
#pragma no_unroll
                for(l = 0; l < (kernel_width_pad>>2); l++)
                {
                    AE_L8X4F_IP(d_inp, pt_inp, 4);
                    AE_L8X4F_IP(d_ker, pt_ker, 4);
                    d_inp = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_inp), 8));
                    d_ker = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_ker), 8));
                    d_inp = AE_ADD16(d_inp, d_input_zero_bias);
                    d_ker = AE_ADD16(d_ker, d_kernel_zero_bias);
                    d_inp1 = AE_SEL16_6543(d_inp0, d_inp);
                    d_inp2 = AE_SEL16_5432(d_inp0, d_inp);
                    d_inp3 = AE_SEL16_4321(d_inp0, d_inp);
                    AE_MULAAAAQ16(accu_int64_0, d_inp0, d_ker);
                    AE_MULAAAAQ16(accu_int64_1, d_inp1, d_ker);
                    AE_MULAAAAQ16(accu_int64_2, d_inp2, d_ker);
                    AE_MULAAAAQ16(accu_int64_3, d_inp3, d_ker);
                    d_inp0 = d_inp;
                }
            }
            *scratch_ptr++ = AE_SEL32_LL(AE_MOVINT32X2_FROMINT64(accu_int64_0), AE_MOVINT32X2_FROMINT64(accu_int64_1));
            *scratch_ptr++ = AE_SEL32_LL(AE_MOVINT32X2_FROMINT64(accu_int64_2), AE_MOVINT32X2_FROMINT64(accu_int64_3));
        }
    }

    /* Here we store output based on strides. For values in a row, values
     * will be picked from it as per 'x_stride'. No need to worry about
     * height dimension, since we took care of it by efficient row
     * accesses. */
    ae_int32 *scratch_ptr1 = (ae_int32 *) p_scratch;
    int left_shift = XT_MAX(0, out_shift);
    int right_shift = XT_MAX(0, -out_shift);

    for(i = 0; i < actual_out_height; i++)
    {
        scratch_ptr1 = (ae_int32 *) p_scratch + (i * output_width_for_x_stride_1);
        WORD8 *out_ptr  = (WORD8 *) p_out + (i * out_stride * actual_out_width);
        ae_int32x2 accu_int32_0;

        for(j = 0; j < actual_out_width; j++)
        {
            accu_int32_0 = scratch_ptr1[(j * x_stride)];

            accu_int32_0 = AE_ADD32S(accu_int32_0, _ae_int32_sat_bias);
            accu_int32_0 = AE_SLAA32(accu_int32_0, left_shift);
            accu_int32_0 = AE_MULFP32X2RAS(accu_int32_0, AE_MOVDA32(out_multiplier));
            accu_int64_0 = AE_SLAI64(AE_MOVINT64_FROMINT32X2(accu_int32_0), 32);
            accu_int64_0 = AE_SRAA64(accu_int64_0, right_shift);
            accu_int32_0 = AE_ROUND32F64SSYM(accu_int64_0);
            accu_int32_0 = AE_ADD32S(accu_int32_0, AE_MOVDA32X2(out_zero_bias, out_zero_bias));
            accu_int32_0 = AE_MAX32(AE_MIN32(accu_int32_0, AE_MOVDA32(255)), AE_ZERO32());

            out_ptr[(j * out_stride)] = (UWORD8)AE_MOVAD32_L(accu_int32_0);
        }
    }
}

static void xa_nn_conv2d_depthwise_nchw_asym8xasym8
(pUWORD8 __restrict__ p_out
 ,const UWORD8 *__restrict__ p_kernel
 ,const UWORD8 *__restrict__ p_inp
 ,const WORD32 *__restrict__ p_bias
 ,WORD32  input_height
 ,WORD32  input_width
 ,WORD32  input_channels
 ,WORD32  kernel_height
 ,WORD32  kernel_width
 ,WORD32  channels_multiplier
 ,WORD32  x_stride
 ,WORD32  y_stride
 ,WORD32  x_padding
 ,WORD32  y_padding
 ,WORD32  out_height
 ,WORD32  out_width
 ,WORD32  input_zero_bias
 ,WORD32  kernel_zero_bias
 ,WORD32  out_multiplier
 ,WORD32  out_shift
 ,WORD32  out_zero_bias
,WORD32  out_data_format
,pVOID p_scratch
)
{
    xa_nn_conv2d_depthwise_init
        (p_scratch
         ,input_height
         ,input_width
         ,input_channels
         ,kernel_height
         ,kernel_width
         ,channels_multiplier
         ,x_stride
         ,y_stride
         ,x_padding
         ,y_padding
         ,out_height
         ,out_width
         ,8
         ,1
        );

    xa_nn_conv2d_dw_state_t *p_state = (xa_nn_conv2d_dw_state_t *)p_scratch;
    xa_nn_circ_buf_t *p_circ_buf = &(p_state->circ_buf);
    int itr_ic, itr_cm, itr_oh;
    int circ_out_height = (p_circ_buf->rows - kernel_height)/y_stride + 1;
    int kernel_width_pad = ALIGNED_SIZE(kernel_width, 4);
    int rows_to_add, top_pad, bottom_pad, rows_added;
    int input_row;
    int input_zero_bias_neg = -input_zero_bias;
    const WORD8 *pt_ker;
    const WORD8 *pt_inp;
    pWORD8 p_inp_circ;
    p_scratch = (pWORD64)(p_state->p_scratch);

    AE_SETCBEGIN0(p_circ_buf->p_begin);
    AE_SETCEND0(p_circ_buf->p_end);

    WORD32 bias = 0;

    for(itr_ic = 0; itr_ic < input_channels; itr_ic++)
    {
        pt_inp = (const WORD8 *)&p_inp[itr_ic*input_height*input_width];

        CIRC_BUF_ADD_ROWS_INIT_WITH_PAD_VAL(rows_added
                ,rows_to_add
                ,top_pad
                ,bottom_pad
                ,input_row
                ,input_height
                ,input_width
                ,kernel_height
                ,y_stride
                ,x_padding
                ,y_padding
                ,p_circ_buf
                ,pt_inp
                ,&input_zero_bias_neg
                );

        for(itr_oh = 0; itr_oh < out_height - (circ_out_height - 1); itr_oh += circ_out_height)
        {
            CIRC_BUF_ADD_ROWS_WITH_PAD_VAL(rows_added
                    ,rows_to_add
                    ,top_pad
                    ,bottom_pad
                    ,input_row
                    ,input_height
                    ,input_width
                    ,circ_out_height
                    ,y_stride
                    ,x_padding
                    ,y_padding
                    ,p_circ_buf
                    ,pt_inp
                    ,&input_zero_bias_neg
                    );

            p_inp_circ = (WORD8 *)p_circ_buf->p_curr;

            for(itr_cm = 0; itr_cm < channels_multiplier; itr_cm++)
            {
                pt_ker = (const WORD8 *)&p_kernel[(itr_ic*channels_multiplier+itr_cm)*kernel_height*kernel_width_pad];
                bias = p_bias[(itr_ic*channels_multiplier+itr_cm)];

                conv2d_nchw_asym8xasym8_hf4_convmul
                    ((pWORD8)(&p_out[(itr_ic*channels_multiplier+itr_cm)+itr_oh*out_width*(input_channels*channels_multiplier)])
                     ,pt_ker
                     ,p_inp_circ
                     ,bias
                     ,p_circ_buf->rows
                     ,p_circ_buf->row_offset
                     ,kernel_height
                     ,kernel_width
                     ,circ_out_height
                     ,out_width
                     ,(input_channels * channels_multiplier)
                     ,x_stride
                     ,y_stride
                     ,input_zero_bias
                     ,kernel_zero_bias
                     ,out_multiplier
                     ,out_shift
                     ,out_zero_bias
                     ,p_scratch
                    );
            }
        }

        CIRC_BUF_ADD_ROWS_WITH_PAD_VAL(rows_added
                ,rows_to_add
                ,top_pad
                ,bottom_pad
                ,input_row
                ,input_height
                ,input_width
                ,circ_out_height
                ,y_stride
                ,x_padding
                ,y_padding
                ,p_circ_buf
                ,pt_inp
                ,&input_zero_bias_neg
                );

        p_inp_circ = (WORD8 *)p_circ_buf->p_curr;

        for(itr_cm = 0; itr_cm < channels_multiplier; itr_cm++)
        {
            pt_ker = (const WORD8 *)&p_kernel[(itr_ic*channels_multiplier+itr_cm)*kernel_height*kernel_width_pad];
            bias = p_bias[(itr_ic*channels_multiplier+itr_cm)];

            conv2d_nchw_asym8xasym8_hf4_convmul
                ((pWORD8)(&p_out[(itr_ic*channels_multiplier+itr_cm)+itr_oh*out_width*(input_channels*channels_multiplier)])
                 ,pt_ker
                 ,p_inp_circ
                 ,bias
                 ,p_circ_buf->rows
                 ,p_circ_buf->row_offset
                 ,kernel_height
                 ,kernel_width
                 ,(out_height - itr_oh)
                 ,out_width
                 ,(input_channels * channels_multiplier)
                 ,x_stride
                 ,y_stride
                 ,input_zero_bias
                 ,kernel_zero_bias
                 ,out_multiplier
                 ,out_shift
                 ,out_zero_bias
                 ,p_scratch
                );
        }
    }
}

/* 2D Convolution implementation */
static inline void conv2d_nhwc_asym8xasym8
(pWORD8 __restrict__ p_out
 ,const UWORD8 *__restrict__ p_ker
 ,const WORD8 *__restrict__ p_inp
 ,const WORD32 *p_bias
 ,int kernel_height
 ,int kernel_width
 ,int out_height
 ,int out_width
 ,int out_channels
 ,int x_stride
 ,int y_stride
 ,WORD32  input_zero_bias
 ,WORD32  kernel_zero_bias
 ,WORD32  out_multiplier
 ,WORD32  out_shift
 ,WORD32  out_zero_bias
 ,pWORD32 __restrict__ p_scratch
 )
{
    WORD32 out_channels_pad;
    WORD32 i, itr_oh, itr_ch, itr_kh, itr_kw;
    pWORD8 pt_inp0, pt_inp1, pt_ker;
    pUWORD8 out_ptr0, out_ptr1;
    ae_int16x4 d_inp0, d_inp1, d_ker;
    const ae_int32x2 *pt_bias;
    ae_valign bias_a;
    ae_int32x2 d_acc0, d_acc1, d_bias0, d_bias1;
    ae_int32x2 d_acc2, d_acc3;
    ae_int16x4 d_acc16x4;
    int left_shift = XT_MAX(0, out_shift);
    int right_shift = XT_MAX(0, -out_shift);

    out_channels_pad = (out_channels + 3)&(~3);

    for(itr_oh = 0; itr_oh < (out_height-1); itr_oh+=2)
    {
        out_ptr0 = (UWORD8 *)(&p_out[itr_oh*out_channels*out_width]);
        out_ptr1 = (UWORD8 *)(&p_out[(itr_oh+1)*out_channels*out_width]);
        pt_bias = (const ae_int32x2 *)p_bias;
        bias_a = AE_LA64_PP(pt_bias);
        for(itr_ch = 0; itr_ch < out_channels; itr_ch+=4)
        {
            pt_inp0 = (WORD8 *)p_inp;
            pt_inp1 = (WORD8 *)p_inp;
            AE_ADDCIRC16X4_XC((ae_int16x4 *)pt_inp0, itr_ch + itr_oh*y_stride*kernel_width*out_channels_pad);
            AE_ADDCIRC16X4_XC((ae_int16x4 *)pt_inp1, itr_ch + (itr_oh+1)*y_stride*kernel_width*out_channels_pad);
            pt_ker = (WORD8 *)(&p_ker[itr_ch]);
            d_acc0 = AE_ZERO32();
            d_acc1 = AE_ZERO32();
            d_acc2 = AE_ZERO32();
            d_acc3 = AE_ZERO32();
            for(itr_kh = 0; itr_kh < kernel_height; itr_kh++)
            {
                WORD8 *ptt_inp0, *ptt_inp1;
                ptt_inp0 = pt_inp0;
                ptt_inp1 = pt_inp1;
                AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp0, itr_kh*kernel_width*out_channels_pad);
                AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp1, itr_kh*kernel_width*out_channels_pad);
#pragma no_unroll
                for(itr_kw = 0; itr_kw < kernel_width; itr_kw++)
                {
                    d_inp0 = AE_L8X4F_I(ptt_inp0, 0);
                    d_inp1 = AE_L8X4F_I(ptt_inp1, 0);
                    d_ker = AE_L8X4F_I(pt_ker, 0);
                    pt_ker += out_channels_pad;
                    d_inp0 = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_inp0), 8));
                    d_inp1 = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_inp1), 8));
                    d_ker = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_ker), 8));
                    d_inp0 = AE_ADD16(d_inp0, AE_MOVDA16(input_zero_bias));
                    d_inp1 = AE_ADD16(d_inp1, AE_MOVDA16(input_zero_bias));
                    d_ker = AE_ADD16(d_ker, AE_MOVDA16(kernel_zero_bias));
                    AE_MULA16X4(d_acc0, d_acc1, d_inp0, d_ker);
                    AE_MULA16X4(d_acc2, d_acc3, d_inp1, d_ker);
                    AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp0, out_channels_pad);
                    AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp1, out_channels_pad);
                }
            }
            AE_LA32X2_IP(d_bias0, bias_a, pt_bias);
            AE_LA32X2_IP(d_bias1, bias_a, pt_bias);
            d_acc0 = AE_ADD32S(d_acc0, d_bias0);
            d_acc1 = AE_ADD32S(d_acc1, d_bias1);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc0, out_multiplier, left_shift, right_shift);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc1, out_multiplier, left_shift, right_shift);
            d_acc0 = AE_ADD32S(d_acc0, AE_MOVDA32(out_zero_bias));
            d_acc1 = AE_ADD32S(d_acc1, AE_MOVDA32(out_zero_bias));
            d_acc0 = AE_MAX32(AE_MIN32(d_acc0, AE_MOVDA32(255)), AE_ZERO32());
            d_acc1 = AE_MAX32(AE_MIN32(d_acc1, AE_MOVDA32(255)), AE_ZERO32());

            d_acc16x4 = AE_SAT16X4(d_acc0, d_acc1);
#pragma no_unroll
            for(i = 0; i < XT_MIN(out_channels-itr_ch, 4); i++)
            {
                out_ptr0[itr_ch+i] = (UWORD8)AE_MOVAD16_3(d_acc16x4);
                d_acc16x4 = AE_SEL16_6543(d_acc16x4, d_acc16x4);
            }

            d_acc2 = AE_ADD32S(d_acc2, d_bias0);
            d_acc3 = AE_ADD32S(d_acc3, d_bias1);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc2, out_multiplier, left_shift, right_shift);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc3, out_multiplier, left_shift, right_shift);
            d_acc2 = AE_ADD32S(d_acc2, AE_MOVDA32(out_zero_bias));
            d_acc3 = AE_ADD32S(d_acc3, AE_MOVDA32(out_zero_bias));
            d_acc2 = AE_MAX32(AE_MIN32(d_acc2, AE_MOVDA32(255)), AE_ZERO32());
            d_acc3 = AE_MAX32(AE_MIN32(d_acc3, AE_MOVDA32(255)), AE_ZERO32());

            d_acc16x4 = AE_SAT16X4(d_acc2, d_acc3);
#pragma no_unroll
            for(i = 0; i < XT_MIN(out_channels-itr_ch, 4); i++)
            {
                out_ptr1[itr_ch+i] = (UWORD8)AE_MOVAD16_3(d_acc16x4);
                d_acc16x4 = AE_SEL16_6543(d_acc16x4, d_acc16x4);
            }
        }
    }
    if(itr_oh < out_height)
    {
        out_ptr0 = (UWORD8 *)(&p_out[itr_oh*out_channels*out_width]);
        pt_bias = (const ae_int32x2 *)p_bias;
        bias_a = AE_LA64_PP(pt_bias);
        for(itr_ch = 0; itr_ch < out_channels; itr_ch+=4)
        {
            pt_inp0 = (WORD8 *)p_inp;
            AE_ADDCIRC16X4_XC((ae_int16x4 *)pt_inp0, itr_ch + itr_oh*y_stride*kernel_width*out_channels_pad);
            pt_ker = (WORD8 *)(&p_ker[itr_ch]);
            d_acc0 = AE_ZERO32();
            d_acc1 = AE_ZERO32();
            for(itr_kh = 0; itr_kh < kernel_height; itr_kh++)
            {
                WORD8 *ptt_inp0 = pt_inp0;
                AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp0, itr_kh*kernel_width*out_channels_pad);
#pragma no_unroll
                for(itr_kw = 0; itr_kw < kernel_width; itr_kw++)
                {
                    d_inp0 = AE_L8X4F_I(ptt_inp0, 0);
                    d_ker = AE_L8X4F_I(pt_ker, 0);
                    pt_ker += out_channels_pad;
                    d_inp0 = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_inp0), 8));
                    d_ker = AE_MOVINT16X4_FROMINT64(AE_SRLI64(AE_MOVINT64_FROMINT16X4(d_ker), 8));
                    d_inp0 = AE_ADD16(d_inp0, AE_MOVDA16(input_zero_bias));
                    d_ker = AE_ADD16(d_ker, AE_MOVDA16(kernel_zero_bias));
                    AE_MULA16X4(d_acc0, d_acc1, d_inp0, d_ker);
                    AE_ADDCIRC16X4_XC((ae_int16x4 *)ptt_inp0, out_channels_pad);
                }
            }
            AE_LA32X2_IP(d_bias0, bias_a, pt_bias);
            AE_LA32X2_IP(d_bias1, bias_a, pt_bias);
            d_acc0 = AE_ADD32S(d_acc0, d_bias0);
            d_acc1 = AE_ADD32S(d_acc1, d_bias1);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc0, out_multiplier, left_shift, right_shift);
            MULTIPLYBYQUANTIZEDMULTIPLIER_X2(d_acc1, out_multiplier, left_shift, right_shift);
            d_acc0 = AE_ADD32S(d_acc0, AE_MOVDA32(out_zero_bias));
            d_acc1 = AE_ADD32S(d_acc1, AE_MOVDA32(out_zero_bias));
            d_acc0 = AE_MAX32(AE_MIN32(d_acc0, AE_MOVDA32(255)), AE_ZERO32());
            d_acc1 = AE_MAX32(AE_MIN32(d_acc1, AE_MOVDA32(255)), AE_ZERO32());

            d_acc16x4 = AE_SAT16X4(d_acc0, d_acc1);
#pragma no_unroll
            for(i = 0; i < XT_MIN(out_channels-itr_ch, 4); i++)
            {
                out_ptr0[itr_ch+i] = (UWORD8)AE_MOVAD16_3(d_acc16x4);
                d_acc16x4 = AE_SEL16_6543(d_acc16x4, d_acc16x4);
            }
        }
    }
}

static void xa_nn_conv2d_depthwise_nhwc_asym8xasym8
(pUWORD8 __restrict__ p_out
 ,const UWORD8 *__restrict__ p_kernel
 ,const UWORD8 *__restrict__ p_inp
 ,const WORD32 *__restrict__ p_bias
 ,WORD32  input_height
 ,WORD32  input_width
 ,WORD32  input_channels
 ,WORD32  kernel_height
 ,WORD32  kernel_width
 ,WORD32  channels_multiplier
 ,WORD32  x_stride
 ,WORD32  y_stride
 ,WORD32  x_padding
 ,WORD32  y_padding
 ,WORD32  out_height
 ,WORD32  out_width
 ,WORD32  input_zero_bias
 ,WORD32  kernel_zero_bias
 ,WORD32  out_multiplier
 ,WORD32  out_shift
 ,WORD32  out_zero_bias
,WORD32  out_data_format
,pVOID p_scratch
)
{
    xa_nn_conv2d_depthwise_init
        (p_scratch
         ,input_height
         ,input_width
         ,input_channels
         ,kernel_height
         ,kernel_width
         ,channels_multiplier
         ,x_stride
         ,y_stride
         ,x_padding
         ,y_padding
         ,out_height
         ,out_width
         ,8
         ,0
        );

    xa_nn_circ_buf_t *p_state = (xa_nn_circ_buf_t *)p_scratch;
    xa_nn_circ_buf_t *p_circ_buf = p_state;
    int itr_ow;
    int cols_to_add, left_pad, right_pad, cols_added;
    int input_col;
    int input_zero_bias_neg = -input_zero_bias;
    const WORD8 *pt_inp;
    pWORD8 p_inp_circ;

    AE_SETCBEGIN0(p_circ_buf->p_begin);
    AE_SETCEND0(p_circ_buf->p_end);

    pt_inp = (const WORD8 *)p_inp;

    CIRC_BUF_ADD_COLS_INIT_WITH_PAD_VAL(cols_added
            ,cols_to_add
            ,left_pad
            ,right_pad
            ,input_col
            ,input_height
            ,input_width
            ,input_channels
            ,kernel_width
            ,channels_multiplier
            ,x_stride
            ,x_padding
            ,y_padding
            ,out_height
            ,p_circ_buf
            ,pt_inp
            ,&input_zero_bias_neg
            );

    for(itr_ow = 0; itr_ow < out_width; itr_ow++)
    {
        CIRC_BUF_ADD_COLS_WITH_PAD_VAL(cols_added
                ,cols_to_add
                ,left_pad
                ,right_pad
                ,input_col
                ,input_height
                ,input_width
                ,input_channels
                ,kernel_width
                ,channels_multiplier
                ,x_stride
                ,x_padding
                ,y_padding
                ,out_height
                ,p_circ_buf
                ,pt_inp
                ,&input_zero_bias_neg
                );

        p_inp_circ = (WORD8 *)p_circ_buf->p_curr;

        conv2d_nhwc_asym8xasym8
            ((pWORD8)(&p_out[itr_ow*input_channels*channels_multiplier])
             ,p_kernel
             ,p_inp_circ
             ,p_bias
             ,kernel_height
             ,kernel_width
             ,out_height
             ,out_width
             ,(input_channels * channels_multiplier)
             ,x_stride
             ,y_stride
             ,input_zero_bias
             ,kernel_zero_bias
             ,out_multiplier
             ,out_shift
             ,out_zero_bias
             ,p_scratch
            );
    }
}

WORD32 xa_nn_conv2d_depthwise_asym8xasym8
(pUWORD8 __restrict__ p_out
 ,const UWORD8 *__restrict__ p_kernel
 ,const UWORD8 *__restrict__ p_inp
 ,const WORD32 *__restrict__ p_bias
 ,WORD32  input_height
 ,WORD32  input_width
 ,WORD32  input_channels
 ,WORD32  kernel_height
 ,WORD32  kernel_width
 ,WORD32  channels_multiplier
 ,WORD32  x_stride
 ,WORD32  y_stride
 ,WORD32  x_padding
 ,WORD32  y_padding
 ,WORD32  out_height
 ,WORD32  out_width
 ,WORD32  input_zero_bias
 ,WORD32  kernel_zero_bias
 ,WORD32  out_multiplier
 ,WORD32  out_shift
 ,WORD32  out_zero_bias
,WORD32  inp_data_format
,WORD32  out_data_format
,pVOID p_scratch)
{
    /* NULL pointer checks */
    XA_NNLIB_ARG_CHK_PTR(p_out, -1);
    XA_NNLIB_ARG_CHK_PTR(p_kernel, -1);
    XA_NNLIB_ARG_CHK_PTR(p_inp, -1);
    XA_NNLIB_ARG_CHK_PTR(p_bias, -1);
    XA_NNLIB_ARG_CHK_PTR(p_scratch, -1);
    /* Pointer alignment checks */
    XA_NNLIB_ARG_CHK_ALIGN(p_out, (ALIGNMENT>>1), -1);
    XA_NNLIB_ARG_CHK_ALIGN(p_kernel, (ALIGNMENT>>1), -1);
    XA_NNLIB_ARG_CHK_ALIGN(p_inp, (ALIGNMENT>>1), -1);
    XA_NNLIB_ARG_CHK_ALIGN(p_bias, (ALIGNMENT>>1), -1);
    XA_NNLIB_ARG_CHK_ALIGN(p_scratch, ALIGNMENT, -1);
    /* Basic Parameter checks */
    XA_NNLIB_ARG_CHK_COND((input_height <= 0 || input_width <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((input_channels <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((kernel_height <= 0 || kernel_width <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((kernel_height > input_height), -1);
    XA_NNLIB_ARG_CHK_COND((kernel_width > input_width), -1);
    XA_NNLIB_ARG_CHK_COND((channels_multiplier <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((y_stride <= 0 || x_stride <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((y_padding < 0 || x_padding < 0), -1);
    XA_NNLIB_ARG_CHK_COND((out_height <= 0 || out_width <= 0), -1);
    XA_NNLIB_ARG_CHK_COND((input_zero_bias > 0 || input_zero_bias < -255), -1);
    XA_NNLIB_ARG_CHK_COND((kernel_zero_bias > 0 || kernel_zero_bias < -255), -1);
    XA_NNLIB_ARG_CHK_COND((out_shift>31), -1);
    XA_NNLIB_ARG_CHK_COND((inp_data_format != 0 && inp_data_format != 1), -1);
    XA_NNLIB_ARG_CHK_COND((out_data_format != 0), -1);
    /* Implementation dependent checks */
    XA_NNLIB_ARG_CHK_COND((y_stride > kernel_height), -1);
    XA_NNLIB_ARG_CHK_COND((x_stride > kernel_width), -1);

    if(inp_data_format == 0)
    {
        xa_nn_conv2d_depthwise_nhwc_asym8xasym8
            (p_out
             ,p_kernel
             ,p_inp
             ,p_bias
             ,input_height
             ,input_width
             ,input_channels
             ,kernel_height
             ,kernel_width
             ,channels_multiplier
             ,x_stride
             ,y_stride
             ,x_padding
             ,y_padding
             ,out_height
             ,out_width
             ,input_zero_bias
             ,kernel_zero_bias
             ,out_multiplier
             ,out_shift
             ,out_zero_bias
             ,out_data_format
             ,p_scratch);
    }
    else if(inp_data_format == 1)
    {
        xa_nn_conv2d_depthwise_nchw_asym8xasym8
            (p_out
             ,p_kernel
             ,p_inp
             ,p_bias
             ,input_height
             ,input_width
             ,input_channels
             ,kernel_height
             ,kernel_width
             ,channels_multiplier
             ,x_stride
             ,y_stride
             ,x_padding
             ,y_padding
             ,out_height
             ,out_width
             ,input_zero_bias
             ,kernel_zero_bias
             ,out_multiplier
             ,out_shift
             ,out_zero_bias
             ,out_data_format
             ,p_scratch);
    }
    return 0;
}
