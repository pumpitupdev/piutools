// Module to Redirect Filesystem Paths
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <libconfig.h>
#include <fcntl.h>

#include <ppl.h>
#include <dbg.h>
#include <fsr.h>


typedef FILE* (*fopen_t)(const char*, const char*);
fopen_t next_fopen;
fopen_t next_fopen_lua;

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

static const char* fs_redirect_rootpath;
static const char* fs_redirect_forcepaths;

int redirect_fs_open(const char *pathname, int flags) {
        printf("[%s:%s] Open %s\n",__FILE__,__FUNCTION__,pathname);
        printf("[%s:%s] Final Open %s\n",__FILE__,__FUNCTION__,fsr_redirect_path(pathname));
    return next_open(fsr_redirect_path(pathname), flags);
}

FILE * redirect_fs_fopen(const char * filename, const char * mode){
    return next_fopen(fsr_redirect_path(filename), mode);
}

FILE * redirect_fopen_lua(const char * filename, const char * mode){
    return next_fopen_lua(fsr_redirect_path(filename), mode);
}



static HCCHookInfoEntry entries[] = {
    {"libc.so.6","open",(void*)redirect_fs_open,(void*)&next_open},
    {"libc.so.6","fopen",(void*)redirect_fs_fopen,(void*)&next_fopen},
    {"libc.so.6","fopen",(void*)redirect_fopen_lua,(void*)&next_fopen_lua}
};

static int read_config_file(const char *filename){
    config_t cfg;
    config_setting_t *setting;
    int success = 1;

    config_init(&cfg);

    if (!config_read_file(&cfg, filename)) {
        DBG_printf("[%s] Error: %s:%d - %s", __FILE__, config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        success = 0;
        goto cleanup;
    }

    setting = config_lookup(&cfg, "fs_redirect.root");
    if (setting != NULL && config_setting_type(setting) == CONFIG_TYPE_STRING) {
        fs_redirect_rootpath = strdup(config_setting_get_string(setting));
    }

    setting = config_lookup(&cfg, "fs_redirect.force_redirect_paths");
    if (setting != NULL && config_setting_type(setting) == CONFIG_TYPE_STRING) {
        fs_redirect_forcepaths = strdup(config_setting_get_string(setting));
    }

cleanup:
    config_destroy(&cfg);
    return success;
}

int plugin_init(const char* config_path, PHCCHookInfoEntry *hook_entry_table){
    read_config_file(config_path);  
    init_fsr(fs_redirect_rootpath,fs_redirect_forcepaths);
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HCCHookInfoEntry);
}