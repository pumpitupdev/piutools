// Logic for PIUTools Loader
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


typedef void* (*hook_func_t)(const char* module_name, const char* function_name, void* hook_function_address);
hook_func_t hook_function; 

static char piutools_root[1024] = {0x00};
static char plugin_config_path[1024] = {0x00};
static const char* hook_library_name = "libflh.so";


static int get_function_address(const char* library_name, const char* function_name, void** pfunction_address) {
    // If we didn't specify a place to store the function address, we can't do anything.
    if (pfunction_address == NULL) { return 0; }
    
    // Open our library or die.
    void* hLibrary = dlopen(library_name, RTLD_NOW);
    if(hLibrary == NULL){return 0;}
    // Resolve our Symbol
    *pfunction_address = dlsym(hLibrary,function_name);
    // If we didn't resolve our symbol - die.
    if(*pfunction_address == NULL){return 0;}

    return 1;
}

void load_plugin(const char* plugin_name){
    char plugin_file_path[1024] = {0x00}; 
    sprintf(plugin_file_path,"%s/plugins/%s.plugin",piutools_root,plugin_name);
    DBG_printf("[%s:%s] Loading Plugin: %s",__FILE__,__FUNCTION__, plugin_name);    
    plugin_init_t plugin_init;
    if(get_function_address(plugin_file_path,"plugin_init",(void**)&plugin_init) == 0){return;}

    // Run our init function to get the entry table and number of entries.
    PHookEntry table;
    int num_entries = plugin_init(plugin_config_path, &table);
    if(num_entries < 0){
        DBG_printf("[%s] Error Running %s:plugin_init", __FUNCTION__,plugin_name);
        return;
    }

    // Install hooks functions for each plugin entry
    PHookEntry cur_entry;
    for(int j=0;j<num_entries;j++){
        cur_entry = &table[j];
        void* rfa = hook_function(cur_entry->target_module_name,cur_entry->target_function_name, cur_entry->hook_function_addr);
        if(cur_entry->real_function_addr != NULL){
            *cur_entry->real_function_addr = rfa;
        }
        DBG_printf("[%s] Hooked: [%s:%s] = %p (%p)", __FUNCTION__, cur_entry->target_module_name, cur_entry->target_function_name, cur_entry->hook_function_addr,rfa);
    }
}


static int parse_loader_config(void* user, const char* section, const char* name, const char* value){
    char *ptr;  
    if(strcmp(section,"PLUGINS") == 0){
        if(value != NULL){
            if(strtoul(value,&ptr,10) == 1){
                load_plugin(name);
            }
        }

    }
    return 1;    
}

static int loader_initialized = 0;
void __attribute__((constructor)) PPL_Init() {
    if(loader_initialized){return;}
    loader_initialized = 1;

    // Set up PIUTOOLS Root
    char* piutools_root_path = getenv("PIUTOOLS_ROOT");
    if(piutools_root_path == NULL){
        realpath(".",piutools_root);
    }else{
        strcpy(piutools_root,piutools_root_path);
    }

    if(piutools_root[strlen(piutools_root)] == '/'){
        piutools_root[strlen(piutools_root)] = 0x00;
    }

    // Set Hook Library Path
    char hook_library_path[1024] = {0x00};
    sprintf(hook_library_path,"%s/%s",piutools_root,hook_library_name);
    //printf("Looking for Hook Library at: %s\n",hook_library_path);
    // First - Get Hook Entrypoint
    if(!get_function_address(hook_library_path,"flh_inline_hook_byname",(void**)&hook_function)){
        printf("Error Resolving Hook Address From Library!\n");
        return;
    }

    // Read our Configuration from the path denoted in PPL_CONF or locally via ppl.conf.
    char* config_path = getenv("PIUTOOLS_CONFIG");
    if(config_path == NULL){
        config_path = "./piutools.ini";
        
    }
    realpath(config_path, plugin_config_path);
    ini_parse(plugin_config_path,parse_loader_config,NULL);
}


