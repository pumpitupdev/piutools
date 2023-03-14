// A simple null handler for USBFS to not crash on older configurations. Doesn't emulate anything - disable if you're emulating the ticket dispenser or anything else non libusb.
#include <string.h>
#include <stdio.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

typedef int (*feof_t)(FILE *);
typedef int (*fclose_t)(FILE *);
static feof_t real_feof;
static fclose_t real_fclose;

int usbfs_feof_patch(FILE * fp){
    if(fp == NULL){return 1;}
    return real_feof(fp);
}

int usbfs_fclose_patch(FILE* fp){
    if(fp == NULL){return 1;}
    return real_fclose(fp);
}

static HookEntry entries[] = {
    {"libc.so.6","feof",(void*)usbfs_feof_patch,(void*)&real_feof},
    {"libc.so.6","fclose",(void*)usbfs_fclose_patch,(void*)&real_fclose}
};

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}