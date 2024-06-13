#pragma once 

void *find_signature_in_executable(unsigned char* sig_data, unsigned int sig_length, int sig_offset);
void mem_hook_patch_function(void* func_addr, void *detour_func);

int mem_hook_signature(unsigned char* sig_data, unsigned int sig_length, int sig_offset, void* hook_func);
