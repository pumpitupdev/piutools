// libstdc++ legacy swap
#define _GNU_SOURCE
#include <elf.h>
#include <link.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "plthook.h"

#include <plugin_sdk/plugin.h>

static char old_cpp_lib_path[1024];
static void* old_cpp_lib_handle;

int get_function_byname(void* hLibrary, const char* function_name, void** pfunction_address) {
    // If we didn't specify a place to store the function address, we can't do anything.
    if (pfunction_address == NULL) { return 0; }
    // Resolve our Symbol
    *pfunction_address = dlsym(hLibrary,function_name);
    // If we didn't resolve our symbol - die.
    if(*pfunction_address == NULL){return 0;}
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    // Resolve path to old cpp library.
    piutools_resolve_path("${GAME_ROM_PATH}/libs/libstdc++.so.6.0.8",old_cpp_lib_path);
    // Get Handle - If we can't do this, the plugin won't work.
    old_cpp_lib_handle = dlopen(old_cpp_lib_path,RTLD_NOW);
    if(old_cpp_lib_handle == NULL){
        printf("[%s] Warning: Old CPP Failed to dlopen, skipping plugin...\n",__FILE__);
        return NULL;
    }

    void* exe_handle = dlopen(NULL, RTLD_LAZY);
    plthook_t* plthook;
    plthook_open_by_handle(&plthook, exe_handle);
    void **addr = NULL;
    int pos = 0;
    const char* name;     
     while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        // This is a shitty way to do it but what can ya do?
        if(name != NULL && !strncmp(name,"_Z",2)){
            void* old_func_address = NULL;
            if(get_function_byname(old_cpp_lib_handle,name,&old_func_address)){
                uintptr_t *got_entry = (uintptr_t *)addr;
                *got_entry = (uintptr_t)old_func_address;
            }
        }
    }
    return NULL;
}
