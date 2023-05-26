// Execution Blocker for System and Execvp and Others
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <PIUTools_SDK.h>

typedef int (*fork_t)(void);
static fork_t next_fork;
typedef int (*system_func_ptr)(const char *command);
static system_func_ptr next_system;

typedef int (*execvp_func_ptr)(const char *file, char *const argv[]);
typedef int (*execv_func_ptr)(const char *file, char *const argv[]);
static execv_func_ptr next_execv;
static execvp_func_ptr next_execvp;

typedef int (*spawnvp_func_t)(int mode, const char *path, char *const argv[]);
static spawnvp_func_t next_spawnvp;

typedef int (*original_execl_t)(const char *path, const char *arg, ...);
static original_execl_t next_execl;

int block_spawnvp(int mode, const char *path, char *const argv[]){
    int res = 0;//next_spawnvp(mode,path,argv);
    DBG_printf("[%s] Block spawnvp for %s: %d",__FILE__,path,res);
    return res;
}
int block_execv(const char* file, char* const argv[]){
    int res = 0;//next_execvp(file,argv);
    DBG_printf("[%s] Block execv for %s: %d",__FILE__,file,res);
    return res;
}

int block_execvp(const char* file, char* const argv[]){
    int res = 0;//next_execvp(file,argv);
    DBG_printf("[%s] Block execvp for %s: %d",__FILE__,file,res);
    return res;
}

int block_system(const char* command){
    int res = 0;//next_system(command);
    DBG_printf("[%s] Block System: %s: %d",__FILE__,command,res);
    return res;
}

int block_execl(const char *path, const char *arg, ...) {
    // You can add custom logic here if needed
    DBG_printf("[%s] Blocked\n",__FUNCTION__);
    // Always return 0
    return 0;
}

int block_fork(void){
    DBG_printf("[%s] Blocked\n",__FUNCTION__);
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

const PHookEntry plugin_init(){
  return entries;
}