// Extensions for Filesystem Manipulation Support
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#include <PIUTools_SDK.h>

#include "ini.h"

#define SEARCH_ANY_TAG "{*}"
#define SEARCH_FILE_TAG "{F}"
#define SEARCH_FULLPATH_TAG "{FULL}"

unsigned int num_subst_paths = 0;
static int module_initialized = 0;
static char config_path[1024] = {0x00};
static int redirect_disabled = 0;

PathSubst PIUTools_Filesystem_Sub[MAX_FILESYSTEM_SUB];

// Redirect Hooks
typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

int redirect_fs_open(const char *pathname, int flags) {
    char n_path[1024] = {0x00};    
    return next_open(PIUTools_Filesystem_Redirect_Path("open",pathname,n_path), flags);
}

typedef int (*mkdir_func)(const char *, mode_t);
mkdir_func next_mkdir;

int redirect_fs_mkdir(const char *pathname, mode_t mode) {
    char n_path[1024] = {0x00};    
    return next_mkdir(PIUTools_Filesystem_Redirect_Path("mkdir",pathname,n_path), mode);    
}

typedef FILE* (*fopen_t)(const char*, const char*);
fopen_t next_fopen;

FILE * redirect_fs_fopen(const char * filename, const char * mode){
    char n_path[1024] = {0x00};
    return next_fopen(PIUTools_Filesystem_Redirect_Path("fopen",filename,n_path), mode);
}

typedef int (*openat_t)(int dirfd, const char *pathname, int flags);
openat_t next_openat;
int redirect_fs_openat(int dirfd, const char *pathname, int flags){
    char n_path[1024] = {0x00};
    return next_openat(dirfd,PIUTools_Filesystem_Redirect_Path("openat",pathname,n_path),flags);
}

typedef DIR *(*opendir_func_t)(const char *name);
opendir_func_t next_opendir;

DIR* redirect_fs_opendir(const char *pathname) {
    char n_path[1024] = {0x00};
    return next_opendir(PIUTools_Filesystem_Redirect_Path("opendir",pathname,n_path));
}

typedef int (*stat64_func_t)(const char *pathname, void *statbuf);
stat64_func_t next_stat64;
int redirect_fs_stat64(const char* pathname, void*statbuf){
    char n_path[1024] = {0x00};
    return next_stat64(PIUTools_Filesystem_Redirect_Path("stat64",pathname,n_path),statbuf);
}

static HookEntry filesystem_redirect_entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", redirect_fs_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","openat", redirect_fs_openat, &next_openat, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","mkdir", redirect_fs_mkdir, &next_mkdir, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","opendir", redirect_fs_opendir, &next_opendir, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","stat64", redirect_fs_stat64, &next_stat64, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","fopen", redirect_fs_fopen, &next_fopen, 1),
    {}
};

// Filesystem Operations

static int parse_filesystem_redirect_config(void* user, const char* section, const char* name, const char* value){    
    if(strcmp(section,"FILESYSTEM_REDIRECT") == 0){
        if(value == NULL){return 1;}
        if(name != NULL){
            if(!strcmp(name,"REDIRECT_DISABLED")){
                redirect_disabled = 1;
                return 1;
            }
        }
        PIUTools_Filesystem_AddRedirect(name,value);        
    }
    return 1;
}

static void piutools_filesystem_init(void){
    if(module_initialized){return;}
    module_initialized = 1;
    for(int i=0;i<MAX_FILESYSTEM_SUB;i++){
        memset(&PIUTools_Filesystem_Sub[i],0,sizeof(PathSubst));
    }
}

void PIUTools_Filesystem_Redirect_Init(void){
    // Check if Filesystem_Redirect Is Enabled... if so, we'll process the hooks here.
    // Run our init function to get the entry table and number of entries.
    PIUTools_Path_Resolve("${GAME_CONFIG_PATH}",config_path);    
    ini_parse(config_path,parse_filesystem_redirect_config,NULL);   
    if(!redirect_disabled){
        PHookEntry fsredirect_entry = filesystem_redirect_entries;
        if (fsredirect_entry != NULL) {
            while (fsredirect_entry->hook_type != HOOK_ENTRY_END) {
                PIUTools_Plugin_LoadHook(fsredirect_entry);
                // Move to the next entry
                fsredirect_entry++;
            }
        }
    }
}

void PIUTools_Filesystem_RemoveRedirect(PPathSubst entry){
    entry->enabled = 0;
    num_subst_paths--;
}

const char* PIUTools_Filesystem_GetFileName(const char* path){
    const char* filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}


PPathSubst PIUTools_Filesystem_AddRedirect(const char* path_from, const char* path_to){
        if(!module_initialized){piutools_filesystem_init();}
        PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_START;

        if(strncmp(path_from,SEARCH_ANY_TAG,strlen(SEARCH_ANY_TAG)) == 0){
            PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_ANY;
            path_from += strlen(SEARCH_ANY_TAG);
        }

        if(strncmp(path_from,SEARCH_FILE_TAG,strlen(SEARCH_FILE_TAG)) == 0){
            PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_FILE;
            path_from += strlen(SEARCH_FILE_TAG);
        }        

        if(strncmp(path_from,SEARCH_FULLPATH_TAG,strlen(SEARCH_FULLPATH_TAG)) == 0){
            PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_FULL;
            path_from += strlen(SEARCH_FULLPATH_TAG);
        }        

        PIUTools_Filesystem_Sub[num_subst_paths].src_path = (char*)malloc(strlen(path_from)+1);
        strcpy(PIUTools_Filesystem_Sub[num_subst_paths].src_path,path_from);
        // We may have to deal with piutools wildcards for resolved paths so let's do that.
        char resolved_path[1024] = {0x00};
        PIUTools_Path_Resolve(path_to,resolved_path);
        if(strlen(resolved_path) > 1){path_to = resolved_path;}
        
        PIUTools_Filesystem_Sub[num_subst_paths].replacement_path = (char*)malloc(strlen(path_to)+1);
        strcpy(PIUTools_Filesystem_Sub[num_subst_paths].replacement_path,path_to);
        DBG_printf("[%s] %s => %s",__FILE__,PIUTools_Filesystem_Sub[num_subst_paths].src_path,PIUTools_Filesystem_Sub[num_subst_paths].replacement_path);
        num_subst_paths++;
        return &PIUTools_Filesystem_Sub[num_subst_paths-1];        
}

int PIUTools_Filesystem_Path_Exist(const char* path) {
    return access(path, R_OK) == 0;
}

int PIUTools_Filesystem_Create_Directory(const char* path){
    struct stat st;
    if (!PIUTools_Filesystem_Path_Exist(path)) {
        return mkdir(path, 0755);
    }
    return 0;
}


static char* fix_slashes_in_place(const char *path, char *buf, size_t nbuf) {
    int pos = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (i > 0 && path[i-1] == '/' && path[i] == '/') {
            continue;
        }
        buf[pos++] = path[i];
        if (pos >= nbuf) {
            return buf;
        }
    }
    return NULL;
}

char* PIUTools_Filesystem_Redirect_Path(const char* func, const char* orig_path, char* sub_path){
    if(!module_initialized){piutools_filesystem_init();}
    
    for(int i=0;i<num_subst_paths;i++){
        if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_START){
            if(strncmp(orig_path,PIUTools_Filesystem_Sub[i].src_path,strlen(PIUTools_Filesystem_Sub[i].src_path)) == 0){
                sprintf(sub_path,"%s%s",PIUTools_Filesystem_Sub[i].replacement_path,orig_path+strlen(PIUTools_Filesystem_Sub[i].src_path));
                DBG_printf("[%s] Redirect 1: %s => %s",func,orig_path,sub_path);                  
                return sub_path;
            }            
        }else if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_ANY){
            if(strlen(orig_path) < strlen(PIUTools_Filesystem_Sub[i].src_path)){continue;}
            if(strstr(orig_path,PIUTools_Filesystem_Sub[i].src_path) != NULL){
                if(strcmp(orig_path,PIUTools_Filesystem_Sub[i].replacement_path)){
                    strcpy(sub_path,PIUTools_Filesystem_Sub[i].replacement_path);
                    DBG_printf("[%s] Redirect 2: %s => %s",func,orig_path,sub_path);                    
                    return sub_path;
                }
            }
        }else if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_FULL){
            if(strlen(orig_path) != strlen(PIUTools_Filesystem_Sub[i].src_path)){continue;}
            if(strcmp(orig_path,PIUTools_Filesystem_Sub[i].src_path) == 0){
                strcpy(sub_path,PIUTools_Filesystem_Sub[i].replacement_path);
                DBG_printf("[%s] Redirect FULL: %s => %s",func,orig_path,sub_path);                    
                return sub_path;
            }
        }else if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_FILE){
            if(strlen(orig_path) < strlen(PIUTools_Filesystem_Sub[i].src_path)){continue;}
            if(strcmp(PIUTools_Filesystem_GetFileName(orig_path),PIUTools_Filesystem_Sub[i].src_path) == 0){
                if(strcmp(orig_path,PIUTools_Filesystem_Sub[i].replacement_path)){
                    strcpy(sub_path,PIUTools_Filesystem_Sub[i].replacement_path);
                    DBG_printf("[%s] Redirect 3: %s => %s",func,orig_path,sub_path);                    
                    return sub_path;
                }
            }
        }         
    }
    
    if(strstr(orig_path,"//")){
        fix_slashes_in_place(orig_path,sub_path,1024);
        if(orig_path){DBG_printf("[%s] Bypass w/ Pathfix: %s",func,sub_path);}        
        return sub_path;
    }
    
    if(orig_path){DBG_printf("[%s] Bypass: %s",func,orig_path);}    
    return (char*)orig_path;
}


