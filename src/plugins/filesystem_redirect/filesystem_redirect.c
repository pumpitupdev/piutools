// Filesystem Redirect Plugin
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

typedef struct _PATH_SUBST{
    char* src_path;
    char* replacement_path;
}PathSubst,*PPathSubst;

// This is a jank way to do it - I get it.
static unsigned int num_subst_paths = 0;
static PathSubst sub_paths[100];

#define DEBUG_REDIRECT 1

typedef FILE* (*fopen_t)(const char*, const char*);
fopen_t next_fopen;
fopen_t next_fopen_lua;

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

int redirect_fs_open(const char *pathname, int flags) {
    #ifdef DEBUG_REDIRECT
        if(pathname){printf("[%s] %s\n",__FUNCTION__,pathname);}
    #endif
    
    for(int i=0;i<num_subst_paths;i++){
        if(strncmp(pathname,sub_paths[i].src_path,strlen(sub_paths[i].src_path)) == 0){
            char n_path[1024] = {0x00};
            sprintf(n_path,"%s%s",sub_paths[i].replacement_path,pathname+strlen(sub_paths[i].src_path));
            return next_open(n_path,flags);
        }
    }
    return next_open(pathname, flags);
}

FILE * redirect_fs_fopen(const char * filename, const char * mode){
    #ifdef DEBUG_REDIRECT
        if(filename){printf("[%s] %s\n",__FUNCTION__,filename);}
    #endif
    for(int i=0;i<num_subst_paths;i++){
        if(strncmp(filename,sub_paths[i].src_path,strlen(sub_paths[i].src_path)) == 0){
            char n_path[1024] = {0x00};
            sprintf(n_path,"%s%s",sub_paths[i].replacement_path,filename+strlen(sub_paths[i].src_path));
            return next_fopen(n_path,mode);
        }
    }    
    return next_fopen(filename, mode);
}

static HookEntry entries[] = {
    {"libc.so.6","open",(void*)redirect_fs_open,(void*)&next_open,1},
    {"libc.so.6","fopen",(void*)redirect_fs_fopen,(void*)&next_fopen,1}
};

static int parse_config(void* user, const char* section, const char* name, const char* value){    
    if(strcmp(section,"FILESYSTEM_REDIRECT") == 0){
        if(value == NULL){return 1;}
        sub_paths[num_subst_paths].src_path = malloc(strlen(name)+1);
        strcpy(sub_paths[num_subst_paths].src_path,name);
        // Value may be a relative path - we'll account for this here.
        if(value[0] == '.'){
            char resolved_path[1024] = {0x00};
            realpath(value, resolved_path);   
            value = resolved_path;
        }
        sub_paths[num_subst_paths].replacement_path = malloc(strlen(value)+1);
        strcpy(sub_paths[num_subst_paths].replacement_path,value);
        DBG_printf("[%s] %s => %s",__FILE__,sub_paths[num_subst_paths].src_path,sub_paths[num_subst_paths].replacement_path);
        num_subst_paths++;
    }
    return 1;
}

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    if(ini_parse(config_path,parse_config,NULL) != 0){return 0;}
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}

