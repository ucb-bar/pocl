#ifndef POCL_MAP_H
#define POCL_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

void put_mem_arg_map(void* addr, char* name);
char* get_mem_arg_map(void* addr);

void put_output_arg_map(char* name, int output);
int get_output_arg_map(char* name);

void put_buffer_arg_map(void* addr, void* ptr);
void* get_buffer_arg_map(void* addr);

#ifdef __cplusplus
}
#endif
#endif
