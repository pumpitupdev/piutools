// Hook Logic for PIUTools
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#include <PIUTools_SDK.h>

typedef void* (*hook_import_byname_t)(const char* module_name, const char* library_module_name, const char* function_name, void* redirect_function_address);
typedef void* (*hook_func_byname_t)(const char* module_name, const char* function_name, void* hook_function_address);
hook_func_byname_t hook_function_byname;
hook_import_byname_t hook_import_byname;

static int module_initialized = 0;

static int piutools_hook_init(void){
    if(module_initialized){return 1;}
    module_initialized = 1;

    // Set Hook Library Path
    char hook_library_path[1024] = {0x00};
    PIUTools_Path_Resolve("${PIUTOOLS_ROOT_PATH}/plugin_deps/libflh.so",hook_library_path);

    // First - Get Hook Entrypoint
    if(!PIUTools_Hook_GetFunctionAddress(hook_library_path,"flh_inline_hook_byname",(void**)&hook_function_byname)){
        printf("[%s] Error Resolving flh_inline_hook_byname From Library!\n",__FUNCTION__);
        return 0;
    }

    if(!PIUTools_Hook_GetFunctionAddress(hook_library_path,"flh_import_table_hook_byname",(void**)&hook_import_byname)){
        printf("[%s] Error Resolving flh_import_table_hook_byname From Library!\n",__FUNCTION__);
        return 0;
    }

    return 1;
}

static void print_hook_status(const char* hook_type, const char* plugin_name, const char* module_name, const char* library_name, const char* function_name, int hook_status){
    const char* hook_fail_msg = "\033[1;31mHook Fail:\033[0m";
    const char* hook_ok_msg = "\033[1;32mHook OK:\033[0m";
    const char* hook_status_msg = (hook_status) ? hook_ok_msg:hook_fail_msg;
    if(module_name == NULL){
        module_name = "Main Executable";
    }

    DBG_printf("[plugin:%s][%s]: %s -- %s %s %s", plugin_name, hook_type, hook_status_msg, module_name, library_name, function_name);
}

int PIUTools_Hook_GetFunctionAddress(const char* library_name, const char* function_name, void** pfunction_address){
    // If we didn't specify a place to store the function address, we can't do anything.
    if (pfunction_address == NULL) { 
        DBG_printf("[%s] (%s:%s) Error: pfunction_address cannot be NULL", __FUNCTION__, library_name, function_name);
        errno = EINVAL;
        return 0; 
    }
    
    // Open our library or die.
    DBG_printf("[%s] %s:%s",__FUNCTION__,library_name,function_name);
    void* hLibrary = dlopen(library_name, RTLD_LAZY);
    if(hLibrary == NULL){
        DBG_printf("[%s] (%s:%s) Error: %s", __FUNCTION__, library_name, function_name, dlerror());
        errno = EACCES;
        return 0;
    }
    // Resolve our Symbol
    *pfunction_address = dlsym(hLibrary,function_name);
    // If we didn't resolve our symbol - die.
    if(*pfunction_address == NULL){
        DBG_printf("[%s] (%s:%s) Error: %s", __FUNCTION__, library_name, function_name, dlerror());
        errno = EINVAL;
        return 0;
    }

    return 1;
}



void* PIUTools_Hook_Inline(const char* module_name, const char* function_name, void* hook_addr, const char *plugin_name){
    if(!module_initialized){if(!piutools_hook_init()){exit(-1);}}    
    void* res = hook_function_byname(module_name,function_name, hook_addr);
    print_hook_status("HOOK_INLINE",plugin_name,module_name,"",function_name,(res != NULL));
    return res;
}

void* PIUTools_Hook_Import(const char* module_name, const char* library_name, const char* function_name, void* hook_addr, const char *plugin_name){
    if(!module_initialized){if(!piutools_hook_init()){exit(-1);}}
    void* res = hook_import_byname(module_name,library_name,function_name,hook_addr);
    print_hook_status("HOOK_IMPORT",plugin_name,module_name,library_name,function_name,(res != NULL));
    return res;
}
