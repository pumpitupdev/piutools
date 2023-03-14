// Emulation Module for USBFS
// Eventually, this will run its own abstraction layer and support the old USBFS, the profile stuff, etc.


// Plugin for libusb0 Fixes
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ini.h"
#include "dbg.h"

#include "plugin.h"

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

typedef FILE *(*fopen_t)(const char *, const char *);
fopen_t next_fopen;

static char usbfs_replacement_path[1024];
// to enable the usbfs endpoint on wsl
// sudo mount -t debugfs none /sys/kernel/debug


// if path is /proc/bus/usb/
    // if path is /proc/bus/usb/devices -> /sys/kernel/debug/usb/devices 
    // else replace /proc/bus/usb with /dev/bus/usb
    // honestly patching paths is stupid - make a fake usbfs that emulates everything 
int libusb0_open(const char *pathname, int flags) {
    if(strcmp(pathname,"/proc/bus/usb/devices") == 0){
        return next_open(usbfs_replacement_path, flags);
    }    
    return next_open(pathname, flags);
}

FILE *libusb0_fopen(const char *filename, const char *mode) {
    if (filename != NULL && strcmp(filename, "/proc/bus/usb/devices") == 0) {       
        return next_fopen(usbfs_replacement_path, mode);
    }
    return next_fopen(filename, mode);    
}

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"LIBUSB0") == 0){
        if(strcmp(name,"usbfs_path") == 0){
            if(value == NULL){return 0;}
            strncpy(usbfs_replacement_path,value,sizeof(usbfs_replacement_path));
            DBG_printf("[%s] Custom libusb0 USBFS Path: %s",__FILE__, usbfs_replacement_path);
            if (access(usbfs_replacement_path, F_OK) == -1) {                
                strcpy(usbfs_replacement_path,"/fake_devices");
                DBG_printf("[%s] Custom libusb0 Fake Devices Used: %s",__FILE__, usbfs_replacement_path);
            } 
        }
    }
    return 1;
}

static HookEntry entries[] = {
    {"libc.so.6","open",(void*)libusb0_open,(void*)&next_open},
    {"libc.so.6","fopen",(void*)libusb0_fopen,(void*)&next_fopen}
};

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    if(ini_parse(config_path,parse_config,NULL) != 0){return 0;}
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}