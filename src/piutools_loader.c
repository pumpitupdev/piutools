// Logic for PIUTools Loader
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <limits.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


typedef void* (*hook_import_byname_t)(const char* module_name, const char* library_module_name, const char* function_name, void* redirect_function_address);
typedef void* (*hook_func_byname_t)(const char* module_name, const char* function_name, void* hook_function_address);
typedef void* (*hook_func_t)(void*, void*);
hook_func_byname_t hook_function_byname;
hook_import_byname_t hook_import_byname;
hook_func_t hook_function;

static const char* hook_library_name = "libflh.so";


static int get_function_address(const char* library_name, const char* function_name, void** pfunction_address) {
    // If we didn't specify a place to store the function address, we can't do anything.
    if (pfunction_address == NULL) { return 0; }
    
    // Open our library or die.
    void* hLibrary = dlopen(library_name, RTLD_NOW);
    if (hLibrary == NULL) {
        fprintf(stderr, "%s: dlopen(): %s\n", __FUNCTION__, dlerror());
        return 0;
    }
    // Resolve our Symbol
    *pfunction_address = dlsym(hLibrary,function_name);
    // If we didn't resolve our symbol - die.
    if(*pfunction_address == NULL){return 0;}

    return 1;
}

void load_plugin(const char* plugin_name){
    char plugin_file_path[PATH_MAX] = {0x00}; 
    piutools_get_plugin_path(plugin_name,plugin_file_path);
    DBG_printf("[%s:%s] Loading Plugin: %s",__FILE__,__FUNCTION__, plugin_name);    
    plugin_init_t plugin_init;
    if(get_function_address(plugin_file_path,"plugin_init",(void**)&plugin_init) == 0){return;}

    // Run our init function to get the entry table and number of entries.
    PHookEntry cur_entry = plugin_init(piutools_game_config_path);

    if (cur_entry != NULL) {
        while (cur_entry->hook_type != HOOK_ENTRY_END) {
            void* rfa;
            const char* hook_type_str = "";
            // Process the current entry
            if(cur_entry->hook_enabled){           
                switch(cur_entry->hook_type){
                    case HOOK_TYPE_INLINE:
                        hook_type_str = "[INLINE]";
                        printf("INLINE %s %s\n",cur_entry->source_library_name,cur_entry->target_function_name);
                        rfa = hook_function_byname(cur_entry->source_library_name,cur_entry->target_function_name, cur_entry->hook_function_addr);
                        break;
                    case HOOK_TYPE_IMPORT:
                        hook_type_str = "[IMPORT]";
                        printf("IMPORT %s %s\n",cur_entry->source_library_name,cur_entry->target_function_name);
                        rfa = hook_import_byname(cur_entry->target_binary_name,cur_entry->source_library_name,cur_entry->target_function_name, cur_entry->hook_function_addr);
                        break;           
                    default:
                        break;         
                }
                const char* tb = cur_entry->target_binary_name;
                if(tb == NULL){
                    tb = "Main Executable";
                }
                if(rfa == NULL){
                    printf("[%s] \033[1;31mHook Fail:\033[0m %s %s [%s:%s] = %p\n", __FUNCTION__, hook_type_str, tb, cur_entry->source_library_name, cur_entry->target_function_name, cur_entry->hook_function_addr);
                    cur_entry++;
                    continue;
                }

                DBG_printf("[%s] \033[1;32mHooked:\033[0m %s %s [%s:%s] = %p (%p)", __FUNCTION__, hook_type_str, tb, cur_entry->source_library_name, cur_entry->target_function_name, cur_entry->hook_function_addr,rfa);

                if(cur_entry->original_function_addr_ptr != NULL){
                    *cur_entry->original_function_addr_ptr = rfa;
                }
            }
            // Move to the next entry
            cur_entry++;
        }
    } else {
        DBG_printf("[%s] %s: no hook entries\n", __FUNCTION__, plugin_name);
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
    piutools_init_sdk();

    // Set Hook Library Path
    char hook_library_path[1024] = {0x00};
    sprintf(hook_library_path,"%s/%s",piutools_root_path,hook_library_name);

    // First - Get Hook Entrypoint
    if(!get_function_address(hook_library_path,"flh_inline_hook_byname",(void**)&hook_function_byname)){
        printf("Error Resolving flh_inline_hook_byname From Library!\n");
        return;
    }

    if(!get_function_address(hook_library_path,"flh_import_table_hook_byname",(void**)&hook_import_byname)){
        printf("Error Resolving flh_import_table_hook_byname From Library!\n");
        return;
    }

    if(!get_function_address(hook_library_path,"flh_inline_hook",(void**)&hook_function)){
        printf("Error Resolving flh_inline_hook From Library!\n");
        return;
    }

    ini_parse(piutools_game_config_path,parse_loader_config,NULL);
}


