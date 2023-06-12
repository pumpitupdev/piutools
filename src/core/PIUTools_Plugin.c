// Plugin Handling Module for PIUTools
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <PIUTools_SDK.h>

#include "ini.h"

static int module_initialized = 0;
static char plugin_config_path[1024] = {0x00};
static char plugin_base_path[1024] = {0x00};


static void load_plugin(const char* plugin_name){
    char plugin_file_path[1024] = {0x00}; 
    sprintf(plugin_file_path,"%s/%s.plugin",plugin_base_path,plugin_name);
    DBG_printf("[%s:%s] Loading Plugin: %s",__FILE__,__FUNCTION__, plugin_name);    
    plugin_init_t plugin_init;
    if(PIUTools_Hook_GetFunctionAddress(plugin_file_path,"plugin_init",(void**)&plugin_init) == 0){
        DBG_printf("[%s] Error Getting plugin_init address for: %s",__FILE__,plugin_file_path);
        return;
    }

    // Run our init function to get the entry table and number of entries.
    PHookEntry cur_entry = plugin_init();

    if (cur_entry != NULL) {
        while (cur_entry->hook_type != HOOK_ENTRY_END) {
            PIUTools_Plugin_LoadHook((void*)cur_entry, plugin_name);
            // Move to the next entry
            cur_entry++;
        }
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


void PIUTools_Plugin_Init(void){
    if(module_initialized){return;}
    module_initialized=1;
    
    PIUTools_Path_Resolve("${GAME_CONFIG_PATH}",plugin_config_path);
    PIUTools_Path_Resolve("${PLUGIN_ROOT_PATH}",plugin_base_path);
    ini_parse(plugin_config_path,parse_loader_config,NULL);
}

void PIUTools_Plugin_LoadHook(void* ventry, const char *plugin_name){
    PHookEntry entry = (PHookEntry)ventry;
    if(!module_initialized){PIUTools_Plugin_Init();}    
    if(entry->hook_enabled){           
        void* rfa;
        switch(entry->hook_type){
            case HOOK_TYPE_INLINE:
                rfa = PIUTools_Hook_Inline(entry->source_library_name,entry->target_function_name, entry->hook_function_addr, plugin_name);
                break;
            case HOOK_TYPE_IMPORT:
                rfa = PIUTools_Hook_Import(entry->target_binary_name,entry->source_library_name,entry->target_function_name, entry->hook_function_addr, plugin_name);
                break;           
            default:
                break;         
        }
        if(entry->original_function_addr_ptr != NULL && rfa != NULL){
            *entry->original_function_addr_ptr = rfa;
        }
    }
}
