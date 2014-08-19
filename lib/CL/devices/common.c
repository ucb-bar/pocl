/* common.c - common code that can be reused between device driver 
              implementations

   Copyright (c) 2011-2013 Universidad Rey Juan Carlos and
                           Pekka Jääskeläinen / Tampere Univ. of Technology
   
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
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#include "pocl_image_util.h"
#include "pocl_util.h"
#include "devices.h"
#include "pocl_mem_management.h"
#include "pocl_runtime_config.h"

#define COMMAND_LENGTH 2048


/**
 * Generate code from the final bitcode using the LLVM
 * tools.
 *
 * Uses an existing (cached) one, if available.
 *
 * @param tmpdir The directory of the work-group function bitcode.
 * @param return the generated binary filename.
 */
const char*
llvm_codegen (const char* tmpdir) {

  const char* wg_method = 
    pocl_get_string_option("POCL_WORK_GROUP_METHOD", "loopvec");
  const int cross_compile = pocl_get_bool_option("POCL_CROSS_COMPILE", 0);
  const char* pocl_verbose_ptr = 
    pocl_get_string_option("POCL_VERBOSE", (char*)NULL);
  int pocl_verbose = pocl_verbose_ptr && *pocl_verbose_ptr;
  pocl_verbose = 1;

  char command[COMMAND_LENGTH];
  char bytecode[POCL_FILENAME_LENGTH];
  char assembly[POCL_FILENAME_LENGTH];

  char* module;
  if(strcmp(wg_method,"spmd") == 0){
    module = malloc(min(POCL_FILENAME_LENGTH, 
	   strlen(tmpdir) + strlen("/parallel_hwacha.so") + 1)); 
  }else{
    module = malloc(min(POCL_FILENAME_LENGTH, 
	   strlen(tmpdir) + strlen("/parallel.so") + 1)); 
  }
  int error;

  if(strcmp(wg_method,"spmd") == 0){
    error = snprintf 
      (module, POCL_FILENAME_LENGTH,
       "%s/parallel_hwacha.so", tmpdir);
    assert (error >= 0);
  }else{
    error = snprintf 
      (module, POCL_FILENAME_LENGTH,
       "%s/parallel.so", tmpdir);
    assert (error >= 0);
  }

  if (access (module, F_OK) != 0)
    {
      if(strcmp(wg_method,"spmd") == 0){
        error = snprintf (bytecode, POCL_FILENAME_LENGTH,
                          "%s/%s", tmpdir, "parallel_hwacha.bc");
        assert (error >= 0);
      }else{
        error = snprintf (bytecode, POCL_FILENAME_LENGTH,
                          "%s/%s", tmpdir, POCL_PARALLEL_BC_FILENAME);
        assert (error >= 0);
      }
      
      if(strcmp(wg_method,"spmd") == 0){
        error = snprintf (assembly, POCL_FILENAME_LENGTH,
        "%s/parallel_hwacha.s",
        tmpdir);
        assert (error >= 0);
      }else{
        error = snprintf (assembly, POCL_FILENAME_LENGTH,
        "%s/parallel.s",
        tmpdir);
        assert (error >= 0);
      }
      
      if(cross_compile){
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
			    CROSS_LLC " " CROSS_HWACHA_LLC_FLAGS " -o %s %s",
			    assembly,
			    bytecode);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
			    CROSS_LLC " " TARGET_LLC_FLAGS " -o %s %s",
			    assembly,
			    bytecode);
          assert (error >= 0);
        }
      }else{
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
			    LLC " " HWACHA_LLC_FLAGS " -o %s %s",
			    assembly,
			    bytecode);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
			    LLC " " HOST_LLC_FLAGS " -o %s %s",
			    assembly,
			    bytecode);
          assert (error >= 0);
        }
      }
      
      if (pocl_verbose) {
        fprintf(stderr, "[pocl] executing [%s]\n", command);
        fflush(stderr);
      }
      error = system (command);
      perror("system error:");
      fprintf(stderr, "LLC Error status:%d\n", error);
      fflush(stderr);
      assert (error == 0);
          
      // For the pthread device, use device type is always the same as
      // the host.
      if(cross_compile){
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
          CROSS_CLANG " " CROSS_HWACHA_AS_FLAGS " -v -c -o %s.o %s ",
          module,
          assembly);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
          CROSS_CLANG " " TARGET_AS_FLAGS " -v -c -o %s.o %s ",
          module,
          assembly);
          assert (error >= 0);
        }
      }else{
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
          CLANG " " HWACHA_AS_FLAGS " -v -c -o %s.o %s ",
          module,
          assembly);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
          CLANG " " HOST_AS_FLAGS " -v -c -o %s.o %s ",
          module,
          assembly);
          assert (error >= 0);
        }
      }
      
      if (pocl_verbose) {
        fprintf(stderr, "[pocl] executing [%s]\n", command);
        fflush(stderr);
      }
      error = system (command);
      assert (error == 0);

      // clang is used as the linker driver in LINK_CMD
      if(cross_compile){
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
                           CROSS_LINK_CMD " " CROSS_HWACHA_CLANG_FLAGS " " CROSS_HWACHA_LD_FLAGS " "
                            "-v -o %s %s.o",
                           module,
                           module);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
                           CROSS_LINK_CMD " " TARGET_CLANG_FLAGS " " TARGET_LD_FLAGS " "
                            "-v -o %s %s.o",
                           module,
                           module);
          assert (error >= 0);
        }
      }else{
        if(strcmp(wg_method,"spmd") == 0){
          error = snprintf (command, COMMAND_LENGTH,
                           LINK_CMD " " HWACHA_CLANG_FLAGS " " HWACHA_LD_FLAGS " "
                            "-v -o %s %s.o",
                           module,
                           module);
          assert (error >= 0);
        }else{
          error = snprintf (command, COMMAND_LENGTH,
                           LINK_CMD " " HOST_CLANG_FLAGS " " HOST_LD_FLAGS " "
                            "-v -o %s %s.o",
                           module,
                           module);
          assert (error >= 0);
        }
      }

      if (pocl_verbose) {
        fprintf(stderr, "[pocl] executing [%s]\n", command);
        fflush(stderr);
      }
      error = system (command);
      assert (error == 0);
    }
  return module;
}

/**
 * Populates the device specific image data structure used by kernel
 * from given kernel image argument
 */
void fill_dev_image_t (dev_image_t* di, struct pocl_argument* parg, 
                       cl_int device)
{
  cl_mem mem = *(cl_mem *)parg->value;
  di->data = (mem->device_ptrs[device].mem_ptr);  
  di->width = mem->image_width;
  di->height = mem->image_height;
  di->depth = mem->image_depth;
  di->row_pitch = mem->image_row_pitch;
  di->slice_pitch = mem->image_slice_pitch;
  di->order = mem->image_channel_order;
  di->data_type = mem->image_channel_data_type;
  pocl_get_image_information (mem->image_channel_order,
                              mem->image_channel_data_type, &(di->num_channels),
                              &(di->elem_size));
}
