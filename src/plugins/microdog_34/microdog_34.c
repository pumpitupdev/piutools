// Plugin for RainbowChina/SafeNET MicroDOG API Version 3.4
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <microdog.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#define MD34_FAKE_FD 0x1337

static char dongle_file_path[1024] = {0x00};

typedef int (*ioctl_func_t)(int fd, int request, void* data);
static ioctl_func_t next_ioctl;


static int md34_ioctl(int fd, int request, void* data) {

    if(request == MD33_IOCTL || request == MD34_IOCTL){
        // This is somewhat of an issue with the MicroDog 3.x Stuff
        // The parameter is an unsigned long* which has to be dereferenced.        
        MicroDog_HandlePacket((unsigned char*)*(unsigned long*)data);
        return 0;
    }
    return next_ioctl(fd, request, data);
}

typedef int (*open_func_t)(const char *pathname, int flags);
static open_func_t next_open;
static int md34_open(const char *pathname, int flags) {
    // If this is a request to open our microdog, we're just going to return with a fake handle.     
    if(strcmp(pathname,MD34_PATH_USB) == 0){
        return MD34_FAKE_FD;
    }       

    // Call the original open function
    return next_open(pathname, flags);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","ioctl", md34_ioctl, &next_ioctl, 1),    
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", md34_open, &next_open, 1),
    {}    
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"MICRODOG_34") == 0){
        if(strcmp(name,"file") == 0){
            if(value == NULL){return 0;}
            if(value[0] == '.'){
                realpath(value,dongle_file_path);
            }else{
                strncpy(dongle_file_path,value,sizeof(dongle_file_path));
            }
            
            MicroDog_Init(dongle_file_path);
            return 1;
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  return entries;
}
