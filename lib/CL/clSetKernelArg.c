/* OpenCL runtime library: clSetKernelArg()

   Copyright (c) 2011 Universidad Rey Juan Carlos
                 2013 Pekka Jääskeläinen / Tampere University of Technology
   
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

#include "config.h"
#include "pocl_cl.h"
#include "pocl_util.h"
#include "pocl_map.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

CL_API_ENTRY cl_int CL_API_CALL
POname(clSetKernelArg)(cl_kernel kernel,
               cl_uint arg_index,
               size_t arg_size,
               const void *arg_value) CL_API_SUFFIX__VERSION_1_0
{
  size_t arg_alignment, arg_alloc_size;
  struct pocl_argument *p;
  struct pocl_argument_info *pi;
  void *value;

  POCL_RETURN_ERROR_COND((kernel == NULL), CL_INVALID_KERNEL);

  POCL_RETURN_ERROR_ON((arg_index >= kernel->num_args), CL_INVALID_ARG_INDEX,
    "This kernel has %u args, cannot set arg %u\n",
    (unsigned)kernel->num_args, (unsigned)arg_index);

  POCL_RETURN_ERROR_ON((kernel->dyn_arguments == NULL), CL_INVALID_KERNEL,
    "This kernel has no arguments that could be set\n");

  pi = &(kernel->arg_info[arg_index]);

  POCL_RETURN_ERROR_ON((arg_size == 0 && pi->is_local),
    CL_INVALID_ARG_SIZE, "arg_size == 0 and arg %u is in local address space\n",
    arg_index);

  POCL_RETURN_ERROR_ON(((pi->type == POCL_ARG_TYPE_POINTER
    || pi->type == POCL_ARG_TYPE_IMAGE)
    && (!pi->is_local) && (arg_size != sizeof(cl_mem))),
    CL_INVALID_ARG_SIZE, "Arg %u is pointer/buffer/image, but arg_size is "
    "not sizeof(cl_mem)", arg_index);

  POCL_RETURN_ERROR_ON((pi->type == POCL_ARG_TYPE_SAMPLER
    && (arg_size != sizeof(cl_sampler_t))),
    CL_INVALID_ARG_SIZE, "Arg %u is sampler, but arg_size is "
    "not sizeof(cl_sampler_t)", arg_index);

  p = &(kernel->dyn_arguments[arg_index]); 
  POCL_LOCK_OBJ (kernel);
  pi->is_set = 0;
  
  if (arg_value != NULL && 
      !(pi->type == POCL_ARG_TYPE_POINTER &&
        *(const int*)arg_value == 0))
    {
      pocl_aligned_free (p->value);
      p->value = NULL;

      /* FIXME: this is a cludge to determine an acceptable alignment,
       * we should probably extract the argument alignment from the
       * LLVM bytecode during kernel header generation. */
      arg_alignment = pocl_size_ceil2(arg_size);
      if (arg_alignment >= MAX_EXTENDED_ALIGNMENT)
        arg_alignment = MAX_EXTENDED_ALIGNMENT;
      arg_alloc_size = arg_size;
      if (arg_alloc_size < arg_alignment)
        arg_alloc_size = arg_alignment;

      value = pocl_aligned_malloc (arg_alignment, arg_alloc_size);
      if (value == NULL)
      {
        POCL_UNLOCK_OBJ (kernel);
        return CL_OUT_OF_HOST_MEMORY;
      }
      
      memcpy (value, arg_value, arg_size);

      p->value = value;
    }
  else
    {
      pocl_aligned_free (p->value);
      p->value = NULL;
    }

  printf(
      "### clSetKernelArg for %s arg %d name %s type_name %s (size %u) set to %x arg_value %x points to %x\n", 
      kernel->name, arg_index, pi->name, pi->type_name, arg_size, p->value, arg_value, *(int*)arg_value);

  printf("mapping cl_mem[%lx]->arg[%s]\n",*(cl_mem*)arg_value, pi->name);
  put_mem_arg_map(*(cl_mem*)arg_value, pi->name);
  char input_file[POCL_FILENAME_LENGTH];
  snprintf(input_file, POCL_FILENAME_LENGTH,"input_%s.h", pi->name);
  char header[300];
  if(pi->type == POCL_ARG_TYPE_POINTER) {
    int pb = snprintf(header, 300, "char %s[%ld] = {\n",pi->name,(*(cl_mem*)arg_value)->size);
    pocl_write_file(input_file, header, pb, 0, 0);
    // Check if this buffer has already been filled
    void* ptr = get_buffer_arg_map(*(cl_mem*)arg_value);
    if(ptr != 0 && (*(cl_mem*)arg_value)->flags != CL_MEM_WRITE_ONLY) {
      //printf("Writing %u bytes to %s\n",(*(cl_mem*)arg_value)->size,input_file);
      int i;
      for(i = 0; i < (*(cl_mem*)arg_value)->size; i++) {
        char byte_text[6];
        snprintf(byte_text,6,"%3d,\n",((uint8_t*)ptr)[i]);
        pocl_write_file(input_file, byte_text, 5, 1, 1);
      }
    }
  } else if(pi->type == POCL_ARG_TYPE_NONE) {
    int pb;
    switch(arg_size) {
      case 1:
        pb = snprintf(header, 300, "%s %s = (%s)0x%hhx;\n",pi->type_name,pi->name,pi->type_name,*(uint8_t*)arg_value);
        break;
      case 2:
        pb = snprintf(header, 300, "%s %s = (%s)0x%hx;\n",pi->type_name,pi->name,pi->type_name,*(uint16_t*)arg_value);
        break;
      case 4:
        pb = snprintf(header, 300, "%s %s = (%s)0x%x;\n",pi->type_name,pi->name,pi->type_name,*(uint32_t*)arg_value);
        break;
      case 8:
        pb = snprintf(header, 300, "%s %s = (%s)0x%lx;\n",pi->type_name,pi->name,pi->type_name,*(uint64_t*)arg_value);
        break;
    }
    pocl_write_file(input_file, header, pb, 0, 0);
  }

  if(pi->type == POCL_ARG_TYPE_POINTER) {
    cl_mem_t* m = (cl_mem_t*)arg_value;
    printf(
      "### cl_mem hp:%lx &hp:%lx \n", m->mem_host_ptr, &(m->mem_host_ptr));
  }
  p->size = arg_size;
  pi->is_set = 1;

  POCL_UNLOCK_OBJ (kernel);
  return CL_SUCCESS;
}
POsym(clSetKernelArg)
