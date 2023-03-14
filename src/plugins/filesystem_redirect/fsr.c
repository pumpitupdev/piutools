// Uber-Basic Filesystem Redirect Logic for Linux
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <dbg.h>

#include "fsr.h"

// We don't need no stinkin' unistd here!
#ifndef F_OK
#define F_OK 0
#endif 

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif


char redirect_root[PATH_MAX] = {0x00};
const char** redirect_dirs;
size_t num_redirect_dirs;
static int fsr_initialized = 0;

void add_trailing_slash(char* path) {
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] != '/') {
        strcat(path, "/");
    }
}

void strip_trailing_slash(char* path) {
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
}

// Populate redirected targets list.
void parse_redirected_dirs_env(const char* redir_paths, const char*** redirect_dirs, size_t* num_redirect_dirs) {
    
    // Count the number of semicolons in the environment variable value
    const char* p = redir_paths;
    size_t count = 1;
    while (*p != '\0') {
        if (*p == ':') {
            count++;
        }
        p++;
    }

    // Allocate space for the array of redirect directories
    const char** dirs = (const char**)malloc(count * sizeof(const char*));

    // Parse the semicolon-delimited values in the environment variable
    p = redir_paths;
    int i = 0;
    while (*p != '\0') {
        const char* start = p;
        while (*p != '\0' && *p != ':') {
            p++;
        }
        if (p > start) {
            size_t len = p - start;
            char* dir = (char*)malloc(len + 1);
            strncpy(dir, start, len);
            dir[len] = '\0';
            dirs[i++] = dir;
        }
        if (*p == ':') {
            p++;
        }
    }

    *redirect_dirs = dirs;
    *num_redirect_dirs = count;
}



char* get_absolute_path(const char* path) {
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) == NULL) {
        // Failed to resolve path
        return NULL;
    }
    char* abs_path = (char*)malloc(strlen(resolved_path) + 1);
    strcpy(abs_path, resolved_path);
    return abs_path;
}

void reduce_path(char* path) {
    char* dest = path;
    char* src = path;
    while (*src) {
        *dest++ = *src++;
        if (*(src - 1) == '/' && *src == '/') {
            src++;
        }
    }
    *dest = '\0';
}

char* fsr_redirect_path(const char* original_path) {
    if(original_path == NULL || strlen(original_path) < 1){return (char*)original_path;}
    char* redirected_path;
    int root_path_len = strlen(redirect_root);
    const char* original_working_path = get_absolute_path(original_path);
    if(original_working_path == NULL){
        original_working_path = original_path;
    }
    int original_path_len = strlen(original_working_path);

    // Allocate space for the redirected path
    redirected_path = (char*)malloc(root_path_len + original_path_len + 1);

    // Check if the original path starts with the root path, in which case we don't need to redirect
    if (strncmp(original_working_path, redirect_root, root_path_len) == 0) {
        DBG_printf("[FSR_REDIRECT_PATH::DBG] NOREDIR - RROOT: %s\n",original_working_path);
        return (char*)original_path;
    }
    // Check if the original path starts with one of the directories we always redirect
    else {
        int i;
        for (i = 0; i < num_redirect_dirs; i++) {
            if (strncmp(original_working_path, redirect_dirs[i], strlen(redirect_dirs[i])) == 0) {
                sprintf(redirected_path, "%s%s", redirect_root, original_working_path);
                reduce_path(redirected_path);
           DBG_printf("[FSR_REDIRECT_PATH::DBG] REDIR - ALWAYS %s => %s\n",original_path,redirected_path);
                return  redirected_path;
            }
        }
        // For anything else, check if the redirected path exists with access()
        // Hack to add a slash to our redirect root if we don't have one, and if the working path doesn't end with one.
        if(original_working_path[0] != '/' && redirect_root[strlen(redirect_root)-1] != '/'){
            strcat(redirect_root,"/");
        }
        sprintf(redirected_path, "%s%s", redirect_root, original_working_path);
        reduce_path(redirected_path);        
        if (access(redirected_path, F_OK) != 0) { // If we couldn't find the path, return the original
    DBG_printf("[FSR_REDIRECT_PATH::DBG] NOREDIR - NOEXIST %s =/?> %s\n",original_path,redirected_path);
            return  (char*)original_path;
        }
    }
  DBG_printf("[FSR_REDIRECT_PATH::DBG] REDIR %s => %s\n",original_path,redirected_path);
    reduce_path(redirected_path);
    return redirected_path;
}

void init_fsr(const char* fsr_root, const char* fsr_redirect_paths){
    if(fsr_initialized){return;}
    fsr_initialized = 1;

    // If we don't have a root set, skip doing any of this.
    if(fsr_root == NULL || strlen(fsr_root) < 2){
        DBG_printf("[FS_REDIRECT] Warn: No FSR_ROOT was set, assuming cwd!");
        strcpy(redirect_root,getenv("PWD"));
    }else{
        strcpy(redirect_root,fsr_root);
    }

    strip_trailing_slash(redirect_root);

    DBG_printf("[FS_REDIRECT::DBG] Set Root Redirect Path to: %s",redirect_root);
    parse_redirected_dirs_env(fsr_redirect_paths, &redirect_dirs, &num_redirect_dirs);

    DBG_printf("[FS_REDIRECT::DBG] Number of hard-redirects: %zu", num_redirect_dirs);
    for (size_t i = 0; i < num_redirect_dirs; i++) {
        DBG_printf("[FS_REDIRECT::DBG] Redirected %zu: %s", i, redirect_dirs[i]);
    }
}