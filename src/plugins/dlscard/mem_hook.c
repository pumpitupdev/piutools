#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>

#define PAGE_SIZE 4096

static void* exe_base_addr = NULL;
static void* exe_end_addr = NULL;

void resolve_base_end(){
    char exe_path[1024];
    FILE *fp;
    char line[1024];
    char *line_ptr;
    unsigned char *start_addr, *end_addr;

    // Find the base address of the executable
    snprintf(exe_path, sizeof(exe_path), "/proc/%d/maps", getpid());
    fp = fopen(exe_path, "r");
    if (fp == NULL) {
        perror("Error opening maps file");
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "r-xp") && strstr(line, ".so") == NULL) {
            line_ptr = strtok(line, "-");
            if (line_ptr != NULL) {
                start_addr = (unsigned char *)strtoul(line_ptr, NULL, 16);
                line_ptr = strtok(NULL, " ");
                if (line_ptr != NULL) {
                    exe_end_addr = (unsigned char *)strtoul(line_ptr, NULL, 16);
                    exe_base_addr = start_addr;
                    break;
                }
            }
        }
    }

    fclose(fp);	
}


void *find_signature_in_executable(unsigned char* sig_data, int sig_length, unsigned int sig_offset) {
	if(exe_base_addr == NULL || exe_end_addr == NULL){
		resolve_base_end();
	}
    unsigned char *p = exe_base_addr;

    while (p < (unsigned char*)exe_end_addr) {
        if (memcmp(p, sig_data, sig_length) == 0) {
            return p + sig_offset;
        }
        p++;
    }

    return NULL;
}

int mem_hook_patch_function(void *func_addr, void *detour_func) {
    uint8_t offset = 5;

    // Add extra offset for 64-bit binaries
    if (sizeof(uintptr_t) == 8) {
        offset += 4;
    }

    uintptr_t call = ((uintptr_t)detour_func) - (uintptr_t)func_addr - offset;

    // Unprotect the memory region
    if (mprotect((void *)((uintptr_t)func_addr & ~(PAGE_SIZE - 1)), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("Error unprotecting memory");
        return -1;
    }

    // Patch the function
    *((uint8_t *)func_addr) = 0xE8;
    *((uintptr_t *)(func_addr + 1)) = call;

    // Restore the protection on the memory region
    if (mprotect((void *)((uintptr_t)func_addr & ~(PAGE_SIZE - 1)), PAGE_SIZE, PROT_READ | PROT_EXEC) != 0) {
        perror("Error restoring memory protection");
        return -1;
    }

    return 0;
}


int mem_hook_signature(unsigned char* sig_data, unsigned int sig_length, int sig_offset, void* hook_func){
	void* sig_addr = find_signature_in_executable(sig_data,sig_length,sig_offset);
	if(sig_addr == NULL){return 0;}
	return(mem_hook_patch_function(sig_addr,hook_func) == 0);
	
}
