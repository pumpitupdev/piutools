// Filesystem Redirect Plugin
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>




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

typedef int (*openat_t)(int dirfd, const char *pathname, int flags);
openat_t next_openat;
int redirect_fs_openat(int dirfd, const char *pathname, int flags){
    char n_path[1024] = {0x00};
    return next_openat(dirfd,PIUTools_Filesystem_Resolve_Path(pathname,n_path),flags);
}



typedef int(*fxstat_t)(int ver, int fildes, struct stat *stat_buf);
fxstat_t next_fxstat;
int redirect_fxstat(int ver, int fildes, struct stat *stat_buf){
    int res = next_fxstat(ver,fildes,stat_buf);
    if(res == -1){res = 0;}
    return res;
}

typedef DIR *(*opendir_func_t)(const char *name);
opendir_func_t next_opendir;

DIR* redirect_fs_opendir(const char *pathname) {
    char n_path[1024] = {0x00};
    return next_opendir(PIUTools_Filesystem_Resolve_Path(pathname,n_path));
}
// stat64

typedef int (*stat64_func_t)(const char *pathname, void *statbuf);
stat64_func_t next_stat64;
int redirect_fs_stat64(const char* pathname, void*statbuf){
    char n_path[1024] = {0x00};
    return next_stat64(PIUTools_Filesystem_Resolve_Path(pathname,n_path),statbuf);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", redirect_fs_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","opendir", redirect_fs_opendir, &next_opendir, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","stat64", redirect_fs_stat64, &next_stat64, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","fopen", redirect_fs_fopen, &next_fopen, 1),
    //HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","__fxstat", redirect_fxstat, &next_fxstat, 1),    
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
