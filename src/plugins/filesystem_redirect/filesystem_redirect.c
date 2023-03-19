// Filesystem Redirect Plugin
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

enum _SUB_TYPE{
    SUB_TYPE_START,
    SUB_TYPE_FILE,
    SUB_TYPE_ANY
};

typedef struct _PATH_SUBST{
    char* src_path;
    char* replacement_path;
    char sub_type;
}PathSubst,*PPathSubst;

// This is a jank way to do it - I get it.
static unsigned int num_subst_paths = 0;
static PathSubst sub_paths[100];

#define SEARCH_ANY_TAG "{*}"
#define SEARCH_FILE_TAG "{F}"
//#define DEBUG_REDIRECT 1

typedef FILE* (*fopen_t)(const char*, const char*);
fopen_t next_fopen;
fopen_t next_fopen_lua;

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

const char* get_filename_from_path(const char* path) {
    const char* filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}

char* sub_path(const char* orig_path, char* sub_path){


    for(int i=0;i<num_subst_paths;i++){
        if(sub_paths[i].sub_type == SUB_TYPE_START){
            if(strncmp(orig_path,sub_paths[i].src_path,strlen(sub_paths[i].src_path)) == 0){
                sprintf(sub_path,"%s%s",sub_paths[i].replacement_path,orig_path+strlen(sub_paths[i].src_path));
                #ifdef DEBUG_REDIRECT
                    printf("[%s] Redirect 1: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                #endif                  
                return sub_path;
            }            
        }else if(sub_paths[i].sub_type == SUB_TYPE_ANY){
            if(strlen(orig_path) < strlen(sub_paths[i].src_path)){continue;}
            if(strstr(orig_path,sub_paths[i].src_path) != NULL){
                if(strcmp(orig_path,sub_paths[i].replacement_path)){
                    strcpy(sub_path,sub_paths[i].replacement_path);
                    #ifdef DEBUG_REDIRECT
                        printf("[%s] Redirect 2: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                    #endif                    
                    return sub_path;
                }
            }
        }else if(sub_paths[i].sub_type == SUB_TYPE_FILE){
            if(strlen(orig_path) < strlen(sub_paths[i].src_path)){continue;}
            if(strcmp(get_filename_from_path(orig_path),sub_paths[i].src_path) == 0){
                if(strcmp(orig_path,sub_paths[i].replacement_path)){
                    strcpy(sub_path,sub_paths[i].replacement_path);
                    #ifdef DEBUG_REDIRECT
                        printf("[%s] Redirect 3: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                    #endif                    
                    return sub_path;
                }
            }
        }         
    }
    #ifdef DEBUG_REDIRECT
        if(orig_path){printf("[%s] Bypass: %s\n",__FUNCTION__,orig_path);}
    #endif    
    return (char*)orig_path;
}

int redirect_fs_open(const char *pathname, int flags) {
    char n_path[1024] = {0x00};    
    return next_open(sub_path(pathname,n_path), flags);
}

FILE * redirect_fs_fopen(const char * filename, const char * mode){
    char n_path[1024] = {0x00};
    return next_fopen(sub_path(filename,n_path), mode);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", redirect_fs_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","fopen", redirect_fs_fopen, &next_fopen, 1),
    {}
};


static int parse_config(void* user, const char* section, const char* name, const char* value){    
    if(strcmp(section,"FILESYSTEM_REDIRECT") == 0){
        if(value == NULL){return 1;}
        
        sub_paths[num_subst_paths].sub_type = SUB_TYPE_START;

        if(strncmp(name,SEARCH_ANY_TAG,strlen(SEARCH_ANY_TAG)) == 0){
            sub_paths[num_subst_paths].sub_type = SUB_TYPE_ANY;
            name += strlen(SEARCH_ANY_TAG);
        }

        if(strncmp(name,SEARCH_FILE_TAG,strlen(SEARCH_FILE_TAG)) == 0){
            sub_paths[num_subst_paths].sub_type = SUB_TYPE_FILE;
            name += strlen(SEARCH_FILE_TAG);
        }        

        sub_paths[num_subst_paths].src_path = malloc(strlen(name)+1);
        strcpy(sub_paths[num_subst_paths].src_path,name);
        // We may have to deal with piutools wildcards for resolved paths so let's do that.
        char resolved_path[1024] = {0x00};
        piutools_resolve_path(value,resolved_path);
        if(strlen(resolved_path) > 1){value = resolved_path;}
        
        sub_paths[num_subst_paths].replacement_path = malloc(strlen(value)+1);
        strcpy(sub_paths[num_subst_paths].replacement_path,value);
        DBG_printf("[%s] %s => %s",__FILE__,sub_paths[num_subst_paths].src_path,sub_paths[num_subst_paths].replacement_path);
        num_subst_paths++;
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    return entries;
}
