/*******************************************************************************
* Copyright (c) 2018-2021 Cadence Design Systems, Inc.
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
#include <string.h>
#include "xa_nnlib_common.h"
#include "xa_nn_conv2d_std_state.h"

WORD32 xa_nn_conv2d_std_getsize(
    WORD32 input_height,
    WORD32 input_channels,
    WORD32 kernel_height,
    WORD32 kernel_width,
    WORD32 y_stride,
    WORD32 y_padding,
    WORD32 out_height,
    WORD32 output_channels,
    WORD32 input_precision)
{
  XA_NNLIB_CHK_COND((input_height <= 0), -1);
  XA_NNLIB_CHK_COND((input_channels <= 0), -1);
  XA_NNLIB_CHK_COND((kernel_height <= 0), -1);
  XA_NNLIB_CHK_COND((kernel_width <= 0), -1);
  XA_NNLIB_CHK_COND((kernel_height > input_height), -1);
  XA_NNLIB_CHK_COND((y_stride <= 0), -1);
  XA_NNLIB_CHK_COND((y_padding < 0), -1);
  XA_NNLIB_CHK_COND((out_height <= 0), -1);

  /* Unused. HiFi4 API compatibility */
  (void)output_channels;

  WORD32 mem_req = 0;
  WORD32 input_size;
  WORD32 align_size;
  WORD32 input_channels_pad;

  mem_req += ALIGNED_SIZE(sizeof(xa_nn_conv_state_t), ALIGNMENT_16);
  /* Input precision is checked here */
  switch(input_precision)
  {
    case 8:
    case -4:
      input_size = sizeof(WORD8);
      align_size = ALIGNMENT>>1;
      break;
    case 16:
      input_size = sizeof(WORD16);
      align_size = ALIGNMENT>>1;
      break;
    case -1:
      input_size = sizeof(WORD32);
      align_size = ALIGNMENT>>2;
      break;
    case -3:
      input_size = sizeof(UWORD8);
      align_size = ALIGNMENT>>1;
      break;
    default:
      return -1;
      break;
  }

  // Computing circular buffer size
  // Determine y-bottom padding
  WORD32 y_b_pad = kernel_height + (out_height - 1) * y_stride - (y_padding + input_height);
  y_b_pad = y_b_pad < 0 ? 0 : y_b_pad;

  if(input_precision == PREC_8 || input_precision == PREC_ASYM8U || input_precision == PREC_ASYM8S) //TODO: remove the condition when the padding requirement is removed for other variants.
    input_channels_pad = input_channels;
  else
    input_channels_pad = PADDED_SIZE(input_channels, align_size);

  WORD32 cir_buf_size_bytes = (y_padding + input_height + y_b_pad) * kernel_width * input_channels_pad * input_size;
  while(cir_buf_size_bytes%16 !=0)
  {
      cir_buf_size_bytes+= kernel_width*input_channels_pad*input_size;
  }
  /* scratch memory for convolution using matrix multiplication */
  mem_req += cir_buf_size_bytes;
  mem_req += BUS_WIDTH;

  return mem_req;
}

WORD32 xa_nn_dilated_conv2d_std_getsize(
    WORD32 input_height,
    WORD32 input_channels,
    WORD32 kernel_height,
    WORD32 kernel_width,
    WORD32 y_stride,
    WORD32 y_padding,
    WORD32 out_height,
    WORD32 input_precision,
    WORD32 dilation_height
    )
{


  XA_NNLIB_CHK_COND((input_height <= 0), -1);
  XA_NNLIB_CHK_COND((input_channels <= 0), -1);
  XA_NNLIB_CHK_COND((kernel_height <= 0), -1);
  XA_NNLIB_CHK_COND((kernel_width <= 0), -1);
  //XA_NNLIB_CHK_COND((kernel_height > input_height), -1);
  XA_NNLIB_CHK_COND((dilation_height <= 0), -1);
  XA_NNLIB_CHK_COND((y_stride <= 0), -1);
  XA_NNLIB_CHK_COND((y_padding < 0), -1);
  XA_NNLIB_CHK_COND((out_height <= 0), -1);
  if(kernel_height==1)
	dilation_height = 1;
  WORD32 kernel_height_dilation = kernel_height + ( (dilation_height-1) * (kernel_height-1) );//dilation
  XA_NNLIB_CHK_COND((kernel_height_dilation > input_height), -1);

  if( (dilation_height>1) ) // Dilation not supporting stride presently
	  XA_NNLIB_CHK_COND((y_stride > 1), -1);

  WORD32 mem_req = 0;
  WORD32 input_size;
  WORD32 align_size;
  WORD32 input_channels_pad;

  mem_req += ALIGNED_SIZE(sizeof(xa_nn_conv_state_t), ALIGNMENT_16);
  /* Input precision is checked here */
  switch(input_precision)
  {
    case 8:
    case -4:
      input_size = sizeof(WORD8);
      align_size = ALIGNMENT>>1;
      break;
    case 16:
      input_size = sizeof(WORD16);
      align_size = ALIGNMENT>>1;
      break;
    case -1:
      input_size = sizeof(WORD32);
      align_size = ALIGNMENT>>2;
      break;
    case -3:
      input_size = sizeof(UWORD8);
      align_size = ALIGNMENT>>1;
      break;
    default:
      return -1;
      break;
  }

  // Computing circular buffer size
  // Determine y-bottom padding
  WORD32 y_b_pad = kernel_height_dilation + (out_height - 1) * y_stride - (y_padding + input_height);
  y_b_pad = y_b_pad < 0 ? 0 : y_b_pad;


  WORD32 input_height_pad = y_padding + input_height + y_b_pad;

  XA_NNLIB_CHK_COND((kernel_height_dilation > input_height_pad), -1);

  if(input_precision == PREC_8 || input_precision == PREC_ASYM8U || input_precision == PREC_ASYM8S) //TODO: remove the condition when the padding requirement is removed for other variants.
    input_channels_pad = input_channels;
  else
    input_channels_pad = PADDED_SIZE(input_channels, align_size);

  WORD32 total_height = ((y_padding + input_height + y_b_pad)/dilation_height) + 1;
  WORD32 cir_buf_size_bytes = total_height * kernel_width * input_channels_pad * input_size;
  while(cir_buf_size_bytes%16 !=0)
  {
      cir_buf_size_bytes+= kernel_width*input_channels_pad*input_size;
  }
  /* scratch memory for convolution using matrix multiplication */
  mem_req += cir_buf_size_bytes;
  mem_req += BUS_WIDTH;

  return mem_req;
}

VOID xa_nn_conv2d_std_init_state(
    VOID *p_scratch,
    VOID *p_kernel,
    WORD32 input_height,
    WORD32 input_channels,
    WORD32 kernel_height,
    WORD32 kernel_width,
    WORD32 x_stride,
    WORD32 y_stride,
    WORD32 y_padding,
    WORD32 out_height,
    WORD32 input_precision)
{
  WORD8 *p_mem = (WORD8 *)p_scratch;
  xa_nn_conv_state_t *p_state = (xa_nn_conv_state_t *)p_mem;
  size_t input_size = 0;
  UWORD32 align_size = 0;
  WORD32 input_channels_pad;

  switch(input_precision)
  {
    case 8:
    case -4:
      input_size = sizeof(WORD8);
      align_size = ALIGNMENT>>1;
      break;
    case 16:
      input_size = sizeof(WORD16);
      align_size = ALIGNMENT>>1;
      break;
    case -1:
      input_size = sizeof(WORD32);
      align_size = ALIGNMENT>>2;
      break;
    case -3:
      input_size = sizeof(UWORD8);
      align_size = ALIGNMENT>>1;
      break;
    default:
      break;
  }

  p_mem += sizeof(xa_nn_conv_state_t);
  p_mem = ALIGNED_ADDR(p_mem, ALIGNMENT_16);


  if(((UWORD32)p_kernel & BUS_WIDTH_MASK) == ((UWORD32)p_mem & BUS_WIDTH_MASK))
  {
    p_mem += BUS_WIDTH; /* Add a offset to avoid banking stall */
  }

  p_state->cir_buf.p_begin = p_mem;
  p_state->cir_buf.p_curr = p_mem;

  // Computing circular buffer size
  // Determine y-bottom padding
  WORD32 y_b_pad = kernel_height + (out_height - 1) * y_stride - (y_padding + input_height);
  y_b_pad = y_b_pad < 0 ? 0 : y_b_pad;

  if(input_precision == PREC_8 || input_precision == PREC_ASYM8U || input_precision == PREC_ASYM8S) //TODO: remove the condition when the padding requirement is removed for other variants.
    input_channels_pad = input_channels;
  else
    input_channels_pad = PADDED_SIZE(input_channels, align_size);

  WORD32 cir_buf_size_bytes = (y_padding + input_height + y_b_pad) * kernel_width * input_channels_pad * input_size;

  while(cir_buf_size_bytes%16 !=0)
  {
      cir_buf_size_bytes+= kernel_width*input_channels_pad*input_size;
  }

  p_mem += cir_buf_size_bytes;
  p_state->cir_buf.p_end = p_mem;

  AE_SETCBEGIN0(p_state->cir_buf.p_begin);
  AE_SETCEND0(p_state->cir_buf.p_end);

}

VOID xa_nn_conv2d_dilation_init_state(
    VOID *p_scratch,
    VOID *p_kernel,
    VOID *p_input)
{
	WORD8 *p_mem = (WORD8 *)p_scratch;
	xa_nn_conv_state_t *p_state = (xa_nn_conv_state_t *)p_mem;

	  p_mem += sizeof(xa_nn_conv_state_t);
	  p_mem = ALIGNED_ADDR(p_mem, ALIGNMENT_16);


	  if(((UWORD32)p_kernel & BUS_WIDTH_MASK) == ((UWORD32)p_mem & BUS_WIDTH_MASK))
	  {
	    p_mem += BUS_WIDTH; /* Add a offset to avoid banking stall */
	  }
	  p_state->cir_buf.p_base = p_mem;
	  p_state->p_inp_base = p_input;
}

VOID xa_nn_dilated_conv2d_std_init_circ_buf(
    VOID *p_scratch,
    VOID *p_kernel,
    WORD32 input_height,
    WORD32 input_channels,
    WORD32 kernel_height_dilation,
    WORD32 kernel_width,
    WORD32 x_stride,
    WORD32 y_stride,
    WORD32 y_padding,
    WORD32 out_height,
    WORD32 input_precision,
    WORD32 dilation_height,
    WORD32 dilation_h_offset)
{
  WORD8 *p_mem;// = (WORD8 *)p_scratch;
  xa_nn_conv_state_t *p_state = (xa_nn_conv_state_t *)p_scratch;
  size_t input_size = 0;
  UWORD32 align_size = 0;
  WORD32 input_channels_pad;

  switch(input_precision)
  {
    case 8:
    case -4:
      input_size = sizeof(WORD8);
      align_size = ALIGNMENT>>1;
      break;
    case 16:
      input_size = sizeof(WORD16);
      align_size = ALIGNMENT>>1;
      break;
    case -1:
      input_size = sizeof(WORD32);
      align_size = ALIGNMENT>>2;
      break;
    case -3:
      input_size = sizeof(UWORD8);
      align_size = ALIGNMENT>>1;
      break;
    default:
      break;
  }

  p_state->cir_buf.p_begin = p_state->cir_buf.p_base;
  p_state->cir_buf.p_curr = p_state->cir_buf.p_begin;

  p_mem = p_state->cir_buf.p_begin;

  // Computing circular buffer size
  // Determine y-bottom padding
  if(input_precision == PREC_8 || input_precision == PREC_ASYM8U || input_precision == PREC_ASYM8S) //TODO: remove the condition when the padding requirement is removed for other variants.
    input_channels_pad = input_channels;
  else
    input_channels_pad = PADDED_SIZE(input_channels, align_size);

  // calculate height for this offset case
  WORD32 y_b_pad_total = kernel_height_dilation + (out_height - 1) * y_stride - (y_padding + input_height);
  y_b_pad_total = y_b_pad_total < 0 ? 0 : y_b_pad_total;

  WORD32 total_height = (y_padding + input_height + y_b_pad_total);
  WORD32 height = (total_height/dilation_height) + (WORD32) (((total_height%dilation_height)-1)>=dilation_h_offset);

  WORD32 cir_buf_size_bytes = height * kernel_width * input_channels_pad * input_size;

  while(cir_buf_size_bytes%16 !=0)
  {
      cir_buf_size_bytes+= kernel_width*input_channels_pad*input_size;
  }

  p_mem += cir_buf_size_bytes;
  p_state->cir_buf.p_end = p_mem;

  AE_SETCBEGIN0(p_state->cir_buf.p_begin);
  AE_SETCEND0(p_state->cir_buf.p_end);

}

VOID conv2d_std_init_cir_buf(
    WORD32 input_channels,
    WORD32 input_channels_pad,
    WORD32 input_bytewidth,
    WORD32 input_width,
    WORD32 input_height,
    WORD32 y_padding,
    WORD32 y_b_pad,
    WORD32 x_padding,
    WORD32 kernel_width,
    WORD32 x_stride,
    VOID **pp_inp,
    xa_nn_conv_state_t *p_state)
{
  WORD32 i,k;
  WORD8 *p_inp = (WORD8 *)*pp_inp;
  WORD32 planes_to_add = x_stride > kernel_width ? 0 : kernel_width - x_stride;
  WORD32 planes_to_keep = kernel_width - planes_to_add;
  ae_int8x8 zero_pad = AE_MOVDA8(0);
  ae_int8x8 inp_val;
  WORD8 *p_dst = (WORD8 *)p_state->cir_buf.p_curr;
  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad * input_bytewidth);

  // Initialize circular buffer
  if(input_channels == 1 && input_bytewidth == 1)
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
  }
  else
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, 0, input_channels_pad * input_bytewidth);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad * input_bytewidth);
    }
  }

  // Set next 'input_height' rows of cir_buf with zero and/or input data
  WORD32 copy_x_pad_width = x_padding;
  WORD32 copy_inp_width = 0;
  if(planes_to_add <= x_padding)
  {
    copy_x_pad_width = planes_to_add;
  }
  else
  {
    copy_inp_width = planes_to_add - x_padding;
  }

  if(input_channels == 1 && input_bytewidth == 1)
  {
    for(i=0;i<input_height;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      for(k=0;k<copy_inp_width;k++)
      {
        AE_L8_IP(inp_val, (ae_int8 *)p_inp, 1);
        AE_S8_0_XC(inp_val, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
      p_inp += (input_width - copy_inp_width);
    }
    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
    p_inp += (-input_height * input_width + copy_inp_width);
    *pp_inp = (VOID *)p_inp;
  }
  else
  {
    for(i=0;i<input_height;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        memset(p_dst, 0, input_channels_pad * input_bytewidth);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
      }
      for(k=0;k<copy_inp_width;k++)
      {
        memcpy(p_dst, p_inp, input_channels * input_bytewidth);
        memset(&p_dst[input_channels * input_bytewidth], 0, (input_channels_pad - input_channels) * input_bytewidth);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
        p_inp += input_channels * input_bytewidth;
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad * input_bytewidth);
      p_inp += (input_width - copy_inp_width) * input_channels * input_bytewidth;
    }

    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, 0, input_channels_pad * input_bytewidth);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad * input_bytewidth);
    }
    p_inp += (-input_height * input_width + copy_inp_width) * input_channels * input_bytewidth;
    *pp_inp = (VOID *)p_inp;
  }
}

// Add x_stride (but not more than kernel_width) x (input_height x input_channels) new planes to circular buffer
VOID conv2d_std_update_cir_buf(
    WORD32 input_channels,
    WORD32 input_channels_pad,
    WORD32 input_bytewidth,
    WORD32 input_width,
    WORD32 input_height,
    WORD32 y_padding,
    WORD32 y_b_pad,
    WORD32 x_padding,
    WORD32 kernel_width,
    WORD32 x_stride,
    VOID **pp_inp,
    WORD32 idx_beg_inp_width_pad,
    xa_nn_conv_state_t *p_state)
{
  WORD32 i,k;
  WORD8 *p_inp = (WORD8 *)*pp_inp;
  WORD32 planes_to_add = x_stride > kernel_width ? kernel_width : x_stride;
  WORD32 planes_to_keep = kernel_width - planes_to_add;
  ae_int8x8 zero_pad = AE_MOVDA8(0);
  ae_int8x8 inp_val;
  WORD8* __restrict__ p_dst_temp;
  WORD8* __restrict__ p_inp_temp;

  if(idx_beg_inp_width_pad < 0)
  {
    /* x_stride > kernel_width case */
    idx_beg_inp_width_pad = 0;
  }

  WORD32 to_skip_inp_width = x_stride - planes_to_add;     // Non-zero for x_stride > kernel_width

  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_state->cir_buf.p_curr, planes_to_add * input_channels_pad * input_bytewidth);
  WORD8 *p_dst = (WORD8 *)p_state->cir_buf.p_curr;
  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad * input_bytewidth);

  // Copy 'planes_to_add' planes of data to circular buffer
  if(input_channels_pad == 1 && input_bytewidth == 1)
  {
    for(k = 0; k < planes_to_add; k++)
    {
      p_dst_temp = p_dst;
      p_inp_temp = p_inp;
      if((idx_beg_inp_width_pad < x_padding) || (idx_beg_inp_width_pad >= x_padding + input_width))
      {
        /* Add a padding frame */
        for(i = 0; i < y_padding + input_height + y_b_pad; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }
      }
      else
      {
        /* Add an input frame */
        /* Top padding */
        for(i = 0; i < y_padding; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }

        /* Input height */
        for(i = 0; i < input_height; i++)
        {
          AE_L8_XP(inp_val, (ae_int8 *)p_inp_temp, input_width);
          AE_S8_0_XC(inp_val, (ae_int8 *)p_dst_temp, kernel_width);
        }

        /* Bottom padding */
        for(i = 0; i < y_b_pad; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }
        p_inp += input_channels * input_bytewidth;
      }

      /* Update the index and destination frame pointer */
      idx_beg_inp_width_pad++;
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
    }
  }
  else
  {
    for(k = 0; k < planes_to_add; k++)
    {
      p_dst_temp = p_dst;
      p_inp_temp = p_inp;
      if((idx_beg_inp_width_pad < x_padding) || (idx_beg_inp_width_pad >= x_padding + input_width))
      {
        /* Add a padding frame */
        for(i = 0; i < y_padding + input_height + y_b_pad; i++)
        {
          memset(p_dst_temp, 0, input_channels_pad * input_bytewidth);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad * input_bytewidth);
        }
      }
      else
      {
        /* Add an input frame */
        /* Top padding */
        for(i = 0; i < y_padding; i++)
        {
          memset(p_dst_temp, 0, input_channels_pad * input_bytewidth);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad * input_bytewidth);
        }

        /* Input height */
        for(i = 0; i < input_height; i++)
        {
          memcpy(p_dst_temp, p_inp_temp, input_channels * input_bytewidth);
          p_inp_temp += input_width * input_channels * input_bytewidth;
          memset(&p_dst_temp[input_channels * input_bytewidth], 0, (input_channels_pad - input_channels) * input_bytewidth);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad * input_bytewidth);
        }

        /* Bottom padding */
        for(i = 0; i < y_b_pad; i++)
        {
          memset(p_dst_temp, 0, input_channels_pad * input_bytewidth);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad * input_bytewidth);
        }
        p_inp += input_channels * input_bytewidth;
      }

      /* Update the index and destination frame pointer */
      idx_beg_inp_width_pad++;
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad * input_bytewidth);
    }
  }

  /* Skip required number of input frames */
  p_inp += to_skip_inp_width * input_channels * input_bytewidth;
  *pp_inp = (VOID *)p_inp;
}

VOID xa_nn_dilated_conv2d_std_load_cir_buf_asym8(
    WORD32 input_channels,
    WORD32 input_channels_pad,
    WORD32 input_bytewidth,
    WORD32 input_width,
    WORD32 input_height,
    WORD32 y_padding,
    WORD32 y_b_pad,
    WORD32 x_padding,
    WORD32 kernel_width,
    WORD32 x_stride,
    VOID **pp_inp,
    xa_nn_conv_state_t *p_state,
    WORD32 pad_val,
    WORD32 dilation_height,
    WORD32 dilation_h_offset,
    WORD32 dilation_width,
    WORD32 dilation_w_offset,
    WORD32 x_padding_full,
    WORD32 *input_padding_consumed,
    WORD32 *input_width_consumed,
    WORD32 planes_to_add,
    WORD32 firstCall,
    WORD32 *circMatrixHeight)
{
  WORD32 i,k;
  WORD8 *p_inp = (WORD8 *)*pp_inp;
  //WORD32 planes_to_add = x_stride > kernel_width ? 0 : kernel_width - x_stride;
  WORD32 planes_to_keep = kernel_width - planes_to_add;
  //ae_int8x8 zero_pad = AE_MOVDA8(pad_val);
  UWORD8 pad_val_u8 = (UWORD8)pad_val;
  //ae_int8x8 inp_val;
  (void) input_bytewidth;
  WORD32 y_padding_dilation;

  if(!firstCall)
	  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_state->cir_buf.p_curr, planes_to_add * input_channels_pad);
  WORD8 *p_dst = (WORD8 *)p_state->cir_buf.p_curr;
  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);

  y_padding_dilation = (y_padding / dilation_height) + (WORD32)(((y_padding%dilation_height)-1)>=dilation_h_offset);
  // Initialize circular buffer
  /*if(input_channels == 1)
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
  }
  else*/
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding_dilation;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
    }
  }

  // Set next 'input_height' rows of cir_buf with zero and/or input data
	//estimate no.zeros for this offset

  ///Calculate number x padding remaining for this width offset which can participate in the convolution process
  WORD32 x_padding_full_dilation = (x_padding_full/dilation_width) + (WORD32) ( ((x_padding_full%dilation_width)-1) >= dilation_w_offset);//This is the contribution of zero padding(in total) towards this width offset
  WORD32 x_padding_dilation_initial_pad = ((x_padding_full-x_padding)/dilation_width) + (WORD32) ( (((x_padding_full-x_padding)%dilation_width)-1) >= dilation_w_offset); /// This offset's contribution which has been absorbed in initial analysis of zero padding
  WORD32 x_padding_dilation = x_padding_full_dilation - x_padding_dilation_initial_pad;//This is the num of zeros contribution from left padding for this dilation offset
  x_padding_dilation = x_padding_dilation - *input_padding_consumed; /// When this loop called repeatedly; some of the input will be consumed discounting for that

  /// Calculate number of input columns remaining for this width offset which can participate in the convolution process
  WORD32 x_padding_plus_input_dilation = ( (x_padding_full+input_width)/dilation_width) + (WORD32) ( (((x_padding_full+input_width)%dilation_width)-1) >= dilation_w_offset);//This is the num elements to be convolved for this offset in total(zeropad+input)
  WORD32 x_input_dilation = x_padding_plus_input_dilation - x_padding_full_dilation;// This is the number of elements from input that can potentially be populated
  x_input_dilation = x_input_dilation - *input_width_consumed;/// When this loop called repeatedly; some of the input will be consumed discounting for that

  WORD32 copy_x_pad_width, copy_x_r_pad_width, copy_inp_width;

  if(planes_to_add <= x_padding_dilation)
  {
    copy_x_pad_width = planes_to_add;
    copy_inp_width = 0;
    copy_x_r_pad_width = 0;
  }
  else if(planes_to_add <= (x_padding_dilation+x_input_dilation) )
  {
	  copy_x_pad_width = x_padding_dilation;
	  copy_inp_width = planes_to_add - copy_x_pad_width;
	  copy_x_r_pad_width = 0;
  }
  else
  {
	  copy_x_pad_width = x_padding_dilation;
	  copy_inp_width = x_input_dilation;
	  copy_x_r_pad_width = planes_to_add - (copy_x_pad_width+copy_inp_width) ;/// No need to calculate the right padding exactly as the loop outside i.e, calling function takes care of it
  }

  /*if(input_channels == 1)
  {
    for(i=0;i<input_height;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      for(k=0;k<copy_inp_width;k++)
      {
        AE_L8_IP(inp_val, (ae_int8 *)p_inp, 1);
        AE_S8_0_XC(inp_val, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
      p_inp += (input_width - copy_inp_width);
    }
    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
    p_inp += (-input_height * input_width + copy_inp_width);
    *pp_inp = (VOID *)p_inp;
  }
  else*/
  {
	// estimate total number of height values for height_offset value from the input matrix
	WORD32 input_padding_plus_height_dilation = ( (y_padding+input_height) / dilation_height) + (WORD32)((((y_padding+input_height)%dilation_height)-1)>=dilation_h_offset);
	WORD32 input_height_dilation = input_padding_plus_height_dilation - y_padding_dilation; /// This value is the height of the circular matrix that has to be iterated for non-zero input values i.e., without top padding and bottim padding iterations
	*circMatrixHeight = input_padding_plus_height_dilation;

	/// estimate the offset needed in the input matrix for this height offset
    WORD32 index_0_input_dilation_height_offset =  (y_padding % dilation_height) ; ///This value represent 0th index in input matrix (post top padding) correspond to which offset in height's dilation scale
    WORD32 input_offset_height_dilation = (dilation_h_offset - index_0_input_dilation_height_offset + dilation_height)%dilation_height;// "index_0_input_dilation_height_offset" represent the dilation offset corresponding to 0 th row of input but, the target is to reach "dilation_h_offset" in dilation scale. This calculation helps reach there from "index_0_input_dilation_height_offset"

	p_inp = p_inp + (input_offset_height_dilation * input_width * input_channels); // This offsets the pointer as per the dilation offset in height dimension
    for(i=0;i<input_height_dilation;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      WORD32 index_0_input_dilation_offset =  (x_padding_full % dilation_width) ; ///This represent 0th index in input matrix correspond to which offset in dilation
      WORD32 input_offset_dilation = (dilation_w_offset - index_0_input_dilation_offset + dilation_width)%dilation_width;// This is the offset corresponding to the present width offset
      p_inp = p_inp + (  (input_offset_dilation + (*input_width_consumed)*dilation_width)   *input_channels);/// This is the offset corresponding
      for(k=0;k<copy_inp_width;k++)
      {
        memcpy(p_dst, p_inp, input_channels);
        memset(&p_dst[input_channels], pad_val_u8, (input_channels_pad - input_channels));
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
        p_inp += (input_channels*dilation_width);
      }
      for(k=0;k<copy_x_r_pad_width;k++)
      {
          memset(p_dst, pad_val_u8, input_channels_pad);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
      p_inp += ( (input_width - ((copy_inp_width*dilation_width)+(input_offset_dilation + (*input_width_consumed)*dilation_width)) ) + ((dilation_height-1)*input_width) )* input_channels;
    }

    *input_width_consumed = *input_width_consumed + copy_inp_width;
    *input_padding_consumed = *input_padding_consumed + copy_x_pad_width;

    WORD32 input_height_toppadding_plus_input_plus_bottom_padding =  ((y_padding+input_height+y_b_pad) / dilation_height) + (WORD32)((((y_padding+input_height+y_b_pad)%dilation_height)-1)>=dilation_h_offset);// This is the total number of input points used for convolution for this height offset value
    WORD32 y_b_pad_dilation = input_height_toppadding_plus_input_plus_bottom_padding - (y_padding_dilation+input_height_dilation);/// This calculates number of bottom padding points for this dilation offset i.e., dilation_h_offset
    *circMatrixHeight = *circMatrixHeight + y_b_pad_dilation;
    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad_dilation;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
    }

  }
}

VOID conv2d_std_init_cir_buf_asym8(
    WORD32 input_channels,
    WORD32 input_channels_pad,
    WORD32 input_bytewidth,
    WORD32 input_width,
    WORD32 input_height,
    WORD32 y_padding,
    WORD32 y_b_pad,
    WORD32 x_padding,
    WORD32 kernel_width,
    WORD32 x_stride,
    VOID **pp_inp,
    xa_nn_conv_state_t *p_state,
    WORD32 pad_val)
{
  WORD32 i,k;
  WORD8 *p_inp = (WORD8 *)*pp_inp;
  WORD32 planes_to_add = x_stride > kernel_width ? 0 : kernel_width - x_stride;
  WORD32 planes_to_keep = kernel_width - planes_to_add;
  ae_int8x8 zero_pad = AE_MOVDA8(pad_val);
  UWORD8 pad_val_u8 = (UWORD8)pad_val;
  ae_int8x8 inp_val;
  WORD8 *p_dst = (WORD8 *)p_state->cir_buf.p_curr;
  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
  (void) input_bytewidth;

  // Initialize circular buffer
  if(input_channels == 1)
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
  }
  else
  {
    // Set first 'y_padding' rows of cir_buf to zero
    for(i=0;i<y_padding;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
    }
  }

  // Set next 'input_height' rows of cir_buf with zero and/or input data
  WORD32 copy_x_pad_width = x_padding;
  WORD32 copy_inp_width = 0;
  if(planes_to_add <= x_padding)
  {
    copy_x_pad_width = planes_to_add;
  }
  else
  {
    copy_inp_width = planes_to_add - x_padding;
  }

  if(input_channels == 1)
  {
    for(i=0;i<input_height;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      for(k=0;k<copy_inp_width;k++)
      {
        AE_L8_IP(inp_val, (ae_int8 *)p_inp, 1);
        AE_S8_0_XC(inp_val, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
      p_inp += (input_width - copy_inp_width);
    }
    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst, 1);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep);
    }
    p_inp += (-input_height * input_width + copy_inp_width);
    *pp_inp = (VOID *)p_inp;
  }
  else
  {
    for(i=0;i<input_height;i++)
    {
      for(k=0;k<copy_x_pad_width;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      for(k=0;k<copy_inp_width;k++)
      {
        memcpy(p_dst, p_inp, input_channels);
        memset(&p_dst[input_channels], pad_val_u8, (input_channels_pad - input_channels));
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
        p_inp += input_channels;
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
      p_inp += (input_width - copy_inp_width) * input_channels;
    }

    // Set last 'y_b_pad' rows of cir_buf to zero
    for(i=0;i<y_b_pad;i++)
    {
      for(k=0;k<planes_to_add;k++)
      {
        memset(p_dst, pad_val_u8, input_channels_pad);
        AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
      }
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);
    }
    p_inp += (-input_height * input_width + copy_inp_width) * input_channels;
    *pp_inp = (VOID *)p_inp;
  }
}

// Add x_stride (but not more than kernel_width) x (input_height x input_channels) new planes to circular buffer
VOID conv2d_std_update_cir_buf_asym8(
    WORD32 input_channels,
    WORD32 input_channels_pad,
    WORD32 input_bytewidth,
    WORD32 input_width,
    WORD32 input_height,
    WORD32 y_padding,
    WORD32 y_b_pad,
    WORD32 x_padding,
    WORD32 kernel_width,
    WORD32 x_stride,
    VOID **pp_inp,
    WORD32 idx_beg_inp_width_pad,
    xa_nn_conv_state_t *p_state,
    WORD32 pad_val)
{
  WORD32 i,k;
  WORD8 *p_inp = (WORD8 *)*pp_inp;
  WORD32 planes_to_add = x_stride > kernel_width ? kernel_width : x_stride;
  WORD32 planes_to_keep = kernel_width - planes_to_add;
  ae_int8x8 zero_pad = AE_MOVDA8(pad_val);
  UWORD8 pad_val_8 = (UWORD8) pad_val;
  ae_int8x8 inp_val;
  WORD8* __restrict__ p_dst_temp;
  WORD8* __restrict__ p_inp_temp;
  (void) input_bytewidth;//TODO: remove

  if(idx_beg_inp_width_pad < 0)
  {
    /* x_stride > kernel_width case */
    idx_beg_inp_width_pad = 0;
  }

  WORD32 to_skip_inp_width = x_stride - planes_to_add;     // Non-zero for x_stride > kernel_width

  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_state->cir_buf.p_curr, planes_to_add * input_channels_pad);
  WORD8 *p_dst = (WORD8 *)p_state->cir_buf.p_curr;
  AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, planes_to_keep * input_channels_pad);

  // Copy 'planes_to_add' planes of data to circular buffer
  if(input_channels_pad == 1)
  {
    for(k = 0; k < planes_to_add; k++)
    {
      p_dst_temp = p_dst;
      p_inp_temp = p_inp;
      if((idx_beg_inp_width_pad < x_padding) || (idx_beg_inp_width_pad >= x_padding + input_width))
      {
        /* Add a padding frame */
        for(i = 0; i < y_padding + input_height + y_b_pad; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }
      }
      else
      {
        /* Add an input frame */
        /* Top padding */
        for(i = 0; i < y_padding; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }

        /* Input height */
        for(i = 0; i < input_height; i++)
        {
          AE_L8_XP(inp_val, (ae_int8 *)p_inp_temp, input_width);
          AE_S8_0_XC(inp_val, (ae_int8 *)p_dst_temp, kernel_width);
        }

        /* Bottom padding */
        for(i = 0; i < y_b_pad; i++)
        {
          AE_S8_0_XC(zero_pad, (ae_int8 *)p_dst_temp, kernel_width);
        }
        p_inp += input_channels;
      }

      /* Update the index and destination frame pointer */
      idx_beg_inp_width_pad++;
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
    }
  }
  else
  {
    for(k = 0; k < planes_to_add; k++)
    {
      p_dst_temp = p_dst;
      p_inp_temp = p_inp;
      if((idx_beg_inp_width_pad < x_padding) || (idx_beg_inp_width_pad >= x_padding + input_width))
      {
        /* Add a padding frame */
        for(i = 0; i < y_padding + input_height + y_b_pad; i++)
        {
          memset(p_dst_temp, pad_val_8, input_channels_pad);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad);
        }
      }
      else
      {
        /* Add an input frame */
        /* Top padding */
        for(i = 0; i < y_padding; i++)
        {
          memset(p_dst_temp, pad_val_8, input_channels_pad);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad);
        }

        /* Input height */
        for(i = 0; i < input_height; i++)
        {
          memcpy(p_dst_temp, p_inp_temp, input_channels);
          p_inp_temp += input_width * input_channels;
          memset(&p_dst_temp[input_channels], pad_val_8, (input_channels_pad - input_channels));
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad);
        }

        /* Bottom padding */
        for(i = 0; i < y_b_pad; i++)
        {
          memset(p_dst_temp, pad_val_8, input_channels_pad);
          AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst_temp, kernel_width * input_channels_pad);
        }
        p_inp += input_channels;
      }

      /* Update the index and destination frame pointer */
      idx_beg_inp_width_pad++;
      AE_ADDCIRC16X4_XC((ae_int16x4 *)p_dst, input_channels_pad);
    }
  }

  /* Skip required number of input frames */
  p_inp += to_skip_inp_width * input_channels;
  *pp_inp = (VOID *)p_inp;

}
