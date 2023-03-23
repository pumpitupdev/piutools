// Filesystem Redirect Plugin
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>


typedef FILE* (*fopen_t)(const char*, const char*);
fopen_t next_fopen;
fopen_t next_fopen_lua;

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

int redirect_fs_open(const char *pathname, int flags) {
    char n_path[1024] = {0x00};    
    return next_open(PIUTools_Filesystem_Resolve_Path(pathname,n_path), flags);
}

FILE * redirect_fs_fopen(const char * filename, const char * mode){
    char n_path[1024] = {0x00};
    return next_fopen(PIUTools_Filesystem_Resolve_Path(filename,n_path), mode);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", redirect_fs_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","fopen", redirect_fs_fopen, &next_fopen, 1),
    {}
};


static int parse_config(void* user, const char* section, const char* name, const char* value){    
    if(strcmp(section,"FILESYSTEM_REDIRECT") == 0){
        if(value == NULL){return 1;}
        PIUTools_Filesystem_Add(name,value);        
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    return entries;
}
