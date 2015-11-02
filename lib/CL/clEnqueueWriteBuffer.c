/* OpenCL runtime library: clEnqueueWriteBuffer()

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
#include "utlist.h"
#include <assert.h>
#include "pocl_util.h"
#include "pocl_map.h"

CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueWriteBuffer)(cl_command_queue command_queue,
                     cl_mem buffer,
                     cl_bool blocking_write,
                     size_t offset,
                     size_t cb, 
                     const void *ptr,
                     cl_uint num_events_in_wait_list,
                     const cl_event *event_wait_list,
                     cl_event *event) CL_API_SUFFIX__VERSION_1_0
{
  cl_device_id device;
  unsigned i;
  _cl_command_node *cmd = NULL;
  int errcode;

  POCL_RETURN_ERROR_COND((command_queue == NULL), CL_INVALID_COMMAND_QUEUE);

  POCL_RETURN_ERROR_COND((buffer == NULL), CL_INVALID_MEM_OBJECT);

  POCL_RETURN_ERROR_ON((command_queue->context != buffer->context),
    CL_INVALID_CONTEXT, "buffer and command_queue are not from the same context\n");

  POCL_RETURN_ERROR_COND((ptr == NULL), CL_INVALID_VALUE)
  if (pocl_buffer_boundcheck(buffer, offset, cb) != CL_SUCCESS)
    return CL_INVALID_VALUE;

  POCL_RETURN_ERROR_COND((event_wait_list == NULL && num_events_in_wait_list > 0),
    CL_INVALID_EVENT_WAIT_LIST);

  POCL_RETURN_ERROR_COND((event_wait_list != NULL && num_events_in_wait_list == 0),
    CL_INVALID_EVENT_WAIT_LIST);

  for(i=0; i<num_events_in_wait_list; i++)
    POCL_RETURN_ERROR_COND((event_wait_list[i] == NULL), CL_INVALID_EVENT_WAIT_LIST);

  device = command_queue->device;

  for (i = 0; i < command_queue->context->num_devices; ++i)
    {
        if (command_queue->context->devices[i] == device)
            break;
    }
  assert(i < command_queue->context->num_devices);

  errcode = pocl_create_command (&cmd, command_queue, 
                                 CL_COMMAND_WRITE_BUFFER, 
                                 event, num_events_in_wait_list, 
                                 event_wait_list);
  if (errcode != CL_SUCCESS)
    return errcode;
  
  cmd->command.write.host_ptr = ptr;
  cmd->command.write.device_ptr =
    (char*)buffer->device_ptrs[device->dev_id].mem_ptr;
  cmd->command.write.offset = offset;
  cmd->command.write.cb = cb;
  cmd->command.write.buffer = buffer;
  POname(clRetainMemObject) (buffer);
  pocl_command_enqueue(command_queue, cmd);

  cl_mem_t* m = (cl_mem_t*)&buffer;
          char* name = get_mem_arg_map(buffer);
          //printf("buffer->flags:%d\n",buffer->flags);
          //printf("write_only:%d\n",CL_MEM_WRITE_ONLY);
          //printf("read_only:%d\n",CL_MEM_READ_ONLY);
          if(name != 0 && buffer->flags != CL_MEM_WRITE_ONLY) {
            put_output_arg_map(name, 0);
            char input_file[POCL_FILENAME_LENGTH];
            snprintf(input_file, POCL_FILENAME_LENGTH,"input_%s.h", name);
            //printf("Writing %u bytes to %s\n",cb,input_file);
            for(i = 0; i < cb; i++) {
              char byte_text[6];
              snprintf(byte_text,6,"%3d,\n",((uint8_t*)ptr)[i]);
              pocl_write_file(input_file, byte_text, 5, 1, 1);
            }
          } else {
            // Not writing it to the file now
            put_buffer_arg_map(buffer, ptr);
          }
  
  if (blocking_write)
    POname(clFinish) (command_queue);

  return CL_SUCCESS;
}
POsym(clEnqueueWriteBuffer)
