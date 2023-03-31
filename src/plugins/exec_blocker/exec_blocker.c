// Execution Blocker for System and Execvp and Others
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

typedef int (*fork_t)(void);
fork_t next_fork;
typedef int (*system_func_ptr)(const char *command);
system_func_ptr next_system;

typedef int (*execvp_func_ptr)(const char *file, char *const argv[]);
typedef int (*execv_func_ptr)(const char *file, char *const argv[]);
execv_func_ptr next_execv;
execvp_func_ptr next_execvp;

typedef int (*spawnvp_func_t)(int mode, const char *path, char *const argv[]);
spawnvp_func_t next_spawnvp;

typedef int (*original_execl_t)(const char *path, const char *arg, ...);
original_execl_t next_execl;

int block_spawnvp(int mode, const char *path, char *const argv[]){
    int res = 0;//next_spawnvp(mode,path,argv);
    printf("[%s] Block spawnvp for %s: %d\n",__FILE__,path,res);
    return res;
}
int block_execv(const char* file, char* const argv[]){
    int res = 0;//next_execvp(file,argv);
    printf("[%s] Block execv for %s: %d\n",__FILE__,file,res);
    return res;
}

int block_execvp(const char* file, char* const argv[]){
    int res = 0;//next_execvp(file,argv);
    printf("[%s] Block execvp for %s: %d\n",__FILE__,file,res);
    return res;
}

int block_system(const char* command){
    int res = 0;//next_system(command);
    printf("[%s] Block System: %s: %d\n",__FILE__,command,res);
    return res;
}

int block_execl(const char *path, const char *arg, ...) {
    // You can add custom logic here if needed
    printf("[%s] Block\n",__FUNCTION__);
    // Always return 0
    return 0;
}


int block_fork(void){
    return -1;
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "execvp", block_execvp, &next_execvp, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "execv", block_execv, &next_execv, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "execv", block_execl, &next_execl, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "spawnvp", block_spawnvp, &next_spawnvp, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "system", block_system, &next_system, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "fork", block_fork, &next_fork, 1),
    {}
};

const PHookEntry plugin_init(const char* config_path){
  return entries;
}