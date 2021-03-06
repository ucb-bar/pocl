/* OpenCL runtime library: pocl_image_util image utility functions

   Copyright (c) 2012 Timo Viitanen / Tampere University of Technology
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "pocl_cl.h"
#include "pocl_image_util.h"
#include "assert.h"

extern cl_int 
pocl_check_image_origin_region (const cl_mem image, 
                                const size_t *origin, 
                                const size_t *region)
{
  if (image == NULL)
    return CL_INVALID_MEM_OBJECT;

  if (origin == NULL || region == NULL)
    return CL_INVALID_VALUE;
  
  /* check if origin + region in each dimension is with in image bounds */
  if (((origin[0] + region[0]) > image->image_row_pitch) || 
      (image->image_height > 0 && 
       ((origin[1] + region[1]) > image->image_height)) ||
      (image->image_depth > 0 && (origin[2] + region[2]) > image->image_depth))
    return CL_INVALID_VALUE;

  return CL_SUCCESS;
}

extern void
pocl_get_image_information (cl_channel_order ch_order, 
                            cl_channel_type ch_type,
                            int* channels_out,
                            int* elem_size_out)
{
  if (ch_type == CL_SNORM_INT8 || ch_type == CL_UNORM_INT8 ||
      ch_type == CL_SIGNED_INT8 || ch_type == CL_UNSIGNED_INT8)
    {
      *elem_size_out = 1; /* 1 byte */
    }
  else if (ch_type == CL_UNSIGNED_INT32 || ch_type == CL_SIGNED_INT32 ||
           ch_type == CL_FLOAT || ch_type == CL_UNORM_INT_101010)
    {
      *elem_size_out = 4; /* 32bit -> 4 bytes */
    }
  else if (ch_type == CL_SNORM_INT16 || ch_type == CL_UNORM_INT16 ||
           ch_type == CL_SIGNED_INT16 || ch_type == CL_UNSIGNED_INT16 ||
           ch_type == CL_UNORM_SHORT_555 || ch_type == CL_UNORM_SHORT_565 ||
           ch_type == CL_HALF_FLOAT)
    {
      *elem_size_out = 2; /* 16bit -> 2 bytes */
    }
  
  /* channels TODO: verify num of channels*/
  if (ch_order == CL_RGB || ch_order == CL_RGBx || ch_order == CL_R || 
      ch_order == CL_Rx || ch_order == CL_A)
    {
      *channels_out = 1;
    }
  else
    {
      *channels_out = 4;
    }
}

cl_int
pocl_write_image(cl_mem               image,
                 cl_device_id         device_id,
                 const size_t *       origin, /*[3]*/
                 const size_t *       region, /*[3]*/
                 size_t               host_row_pitch,
                 size_t               host_slice_pitch, 
                 const void *         ptr)
{
  
  if (image == NULL)
    return CL_INVALID_MEM_OBJECT;

  if ((ptr == NULL) || (region == NULL) || origin == NULL)
    return CL_INVALID_VALUE;
    
  size_t dev_elem_size = sizeof(cl_float);
  int dev_channels = 4;
    
  int host_elem_size;
  int host_channels;
  pocl_get_image_information (image->image_channel_order,
                              image->image_channel_data_type, 
                              &host_channels, &host_elem_size);
    
  size_t tuned_origin[3] = {origin[0]*dev_elem_size*dev_channels, origin[1], 
                            origin[2]};
  size_t tuned_region[3] = {region[0]*dev_elem_size*dev_channels, region[1], 
                            region[2]};
    
  size_t image_row_pitch = image->image_width*dev_elem_size*dev_channels;
  size_t image_slice_pitch = 0;
    
  if ((tuned_region[0]*tuned_region[1]*tuned_region[2] > 0) &&
      (tuned_region[0]-1 +
       image_row_pitch * (tuned_region[1]-1) +
       image_slice_pitch * (tuned_region[2]-1) >= image->size))
    return CL_INVALID_VALUE;
  
  device_id->ops->write_rect (device_id->data, ptr, 
                         image->device_ptrs[device_id->dev_id].mem_ptr,
                         tuned_origin, tuned_origin, tuned_region,
                         image_row_pitch, image_slice_pitch,
                         image_row_pitch, image_slice_pitch);
  
  
  return CL_SUCCESS;
}
           
extern cl_int         
pocl_read_image(cl_mem               image,
                cl_device_id         device_id,
                const size_t *       origin, /*[3]*/
                const size_t *       region, /*[3]*/
                size_t               host_row_pitch,
                size_t               host_slice_pitch, 
                void *               ptr) 
{
    
  if (image == NULL)
    return CL_INVALID_MEM_OBJECT;

  if ((ptr == NULL) || (region == NULL) || origin == NULL)
    return CL_INVALID_VALUE;
    
  int width = image->image_width;
  int height = image->image_height;

  int host_channels, host_elem_size;
      
  pocl_get_image_information(image->image_channel_order,
                             image->image_channel_data_type,
                             &host_channels, &host_elem_size);

  /* dev imagetype = host imagetype, in current implementation */
  int dev_elem_size = host_elem_size;
  int dev_channels = host_channels;

  size_t tuned_origin[3] = {origin[0]*dev_elem_size*dev_channels, origin[1], 
                            origin[2]};
  size_t tuned_region[3] = {region[0]*dev_elem_size*dev_channels, region[1], 
                            region[2]};
  
  size_t image_row_pitch = width*dev_elem_size*dev_channels; 
  size_t image_slice_pitch = height*image_row_pitch;
    
  if ((tuned_origin[0] + tuned_region[0] > image_row_pitch) || 
      (tuned_origin[1] + tuned_region[1] > height))
     return CL_INVALID_VALUE;
  
  if ((image->type == CL_MEM_OBJECT_IMAGE3D && 
       (tuned_origin[2] + tuned_region[2] > image->image_depth)))
    return CL_INVALID_VALUE;
  
  if (image->type != CL_MEM_OBJECT_IMAGE3D && region[2] != 1)
    return CL_INVALID_VALUE;
  
  device_id->ops->read_rect(device_id->data, ptr, 
                       image->device_ptrs[device_id->dev_id].mem_ptr,
                       tuned_origin, tuned_origin, tuned_region,
                       image_row_pitch, image_slice_pitch,
                       image_row_pitch, image_slice_pitch);
  
  return CL_SUCCESS;
}
