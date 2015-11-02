#include "pocl_map.h"

#include <map>

static std::map<void*, char*> target_mem_arg_map;
static std::map<char*, int> output_arg_map;
static std::map<void*, void*> buffer_arg_map;

void put_mem_arg_map(void* addr, char* name) {
  target_mem_arg_map[addr] = name;
}

char* get_mem_arg_map(void* addr) {
  auto it = target_mem_arg_map.find(addr);
  if(it != target_mem_arg_map.end()) return it->second;
  return 0;
}

void put_output_arg_map(char* name, int output) {
  output_arg_map[name] = output;
}

int get_output_arg_map(char* name) {
  auto it = output_arg_map.find(name);
  if(it != output_arg_map.end()) return it->second;
  return 0;
}

void put_buffer_arg_map(void* addr, void* ptr) {
  buffer_arg_map[addr] = ptr;
}

void* get_buffer_arg_map(void* addr) {
  auto it = buffer_arg_map.find(addr);
  if(it != buffer_arg_map.end()) return it->second;
  return 0;
}
