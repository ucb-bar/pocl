/* OpenCL runtime library: clReleaseKernel()

   Copyright (c) 2011 Universidad Rey Juan Carlos
   
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
#include "pocl_util.h"

CL_API_ENTRY cl_int CL_API_CALL
POname(clReleaseKernel)(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0
{
  int new_refcount;
  cl_kernel *pk;
  unsigned i;

  POCL_RETURN_ERROR_COND((kernel == NULL), CL_INVALID_KERNEL);

  POCL_RELEASE_OBJECT (kernel, new_refcount);


  if (new_refcount == 0)
    {
  //Fix up include_* files
  char include[100];
  char* end_file = "};";
  for(i = 0; i < kernel->num_args; i++) {
    if(kernel->arg_info[i].type == POCL_ARG_TYPE_POINTER) {
      snprintf(include, 100, "input_%s.h", kernel->arg_info[i].name);
      pocl_write_file(include, end_file, 2, 1, 1);
    }
  }
  // Write Main Body
  char* glue_file = "glue.c";
  char main_body[200];
  int main_body_size = snprintf(main_body, 200, "\nint main() {\n");
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);

  for(i = 0; i < kernel->num_args; i++) {
    if(get_output_arg_map(kernel->arg_info[i].name) == 1) {
      main_body_size = snprintf(main_body, 200, "  char test_%s[sizeof(%s)];\n",
        kernel->arg_info[i].name,
        kernel->arg_info[i].name);
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
    }
  }

  main_body_size = snprintf(main_body, 200, "  void* args[WG_LENGTH] = {");
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
  for(i = 0; i < kernel->num_args; i++) {
    if(get_output_arg_map(kernel->arg_info[i].name) == 1) {
      main_body_size = snprintf(main_body, 200, " &test_%s%c", kernel->arg_info[i].name, i == kernel->num_args - 1 ? '}' : ',');
    } else {
      main_body_size = snprintf(main_body, 200, " &%s%c", kernel->arg_info[i].name, i == kernel->num_args - 1 ? '}' : ',');
    }
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
  }
  main_body_size = snprintf(main_body, 200, ";\n  for(int i = 0; i < WG_LENGTH; i++) {\n");
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
  main_body_size = snprintf(main_body, 200, "    _pocl_launcher_%s_workgroup_fast(args, wg_array[i]);\n",kernel->name);
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
  main_body_size = snprintf(main_body, 200, "  }\n",kernel->name);
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
  // check outputs
  for(i = 0; i < kernel->num_args; i++) {
    if(get_output_arg_map(kernel->arg_info[i].name) == 1) {
      main_body_size = snprintf(main_body, 200, "  for(int i = 0; i < sizeof(%s); i++) {\n", kernel->arg_info[i].name);
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
      main_body_size = snprintf(main_body, 200, "    if(%s[i] != test_%s[i]) { \n",
          kernel->arg_info[i].name, kernel->arg_info[i].name);
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
      main_body_size = snprintf(main_body, 200, "      printf(\"FAIL\\n\");\n      return 1;\n");
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
      main_body_size = snprintf(main_body, 200, "    }\n  }\n");
      pocl_write_file(glue_file, main_body, main_body_size, 1, 1);
    }
  }
  main_body_size = snprintf(main_body, 200, "  printf(\"PASS\\n\");\n  return 0;\n}\n");
  pocl_write_file(glue_file, main_body, main_body_size, 1, 1);

      if (kernel->program != NULL)
        {
          /* Find the kernel in the program's linked list of kernels */
          for (pk=&kernel->program->kernels; *pk != NULL; pk = &(*pk)->next)
            {
              if (*pk == kernel) break;
            }
          if (*pk == NULL)
            {
              /* The kernel is not on the kernel's program's linked list
                 of kernels -- something is wrong */
              return CL_INVALID_VALUE;
            }
          
          /* Remove the kernel from the program's linked list of
             kernels */
          *pk = (*pk)->next;
          POname(clReleaseProgram) (kernel->program);
        }
      
      POCL_MEM_FREE(kernel->name);

      for (i = 0; i < kernel->num_args; i++)
        {
          struct pocl_argument *p = &(kernel->dyn_arguments[i]);
          if (p->value != NULL)
            {
              pocl_aligned_free (p->value);
              p->value = NULL;
            }
        }

      POCL_MEM_FREE(kernel->dyn_arguments);
      POCL_MEM_FREE(kernel->reqd_wg_size);
      POCL_MEM_FREE(kernel);
    }
  
  return CL_SUCCESS;
}
POsym(clReleaseKernel)
