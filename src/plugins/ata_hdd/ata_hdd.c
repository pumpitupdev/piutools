// ATA Info Hook for KPUMP Prior to Prime2
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#ifndef HDIO_GET_IDENTITY
#define HDIO_GET_IDENTITY 0x030D
#endif

static unsigned char ATA_INFO_BLOCK[512] = {0x00};


// Handle Our HDD Security Calls
typedef int (*ioctl_func_t)(int, unsigned long, ...);
static ioctl_func_t next_ioctl;
static int ata_hdd_ioctl(int fd, int request, void* data) {
    // If this is our HDD, we'll copy our HDD Data.
    if(request == HDIO_GET_IDENTITY){
        memcpy((unsigned char*)data,ATA_INFO_BLOCK,sizeof(ATA_INFO_BLOCK));      
        return 0;
    }
    return next_ioctl(fd, request, data);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "ioctl", ata_hdd_ioctl, &next_ioctl, 1),
    {}
};

/* 
    NX had a couple of images (v1.05 so far) that, for whatever reason, 
    used a HDD where the model number had leading spaces which isn't ATA spec.
    This is clearly how the reporting was supposed to work considering the drive info
    on disk is the same, yet the game's hdd check routine fails on reading this for whatever reason.
    Instead, for... who knows why, if the leading space is swapped for trailing space and you start
    the buffer with the model number, it validates... wtf...

    As a result, we will skip leading spaces if they are present to avoid this potential issue.
*/
static void update_ata_info_block(const char* value, size_t offset, size_t size) {
    if(value == NULL){return;}
    // Prep the target area with the right number of spaces.
    memset(ATA_INFO_BLOCK + offset, 0x20, size);

    // Find the first non-space character in the input string.
    size_t value_len = strlen(value);
    while (value_len > 0 && *value == 0x20) {
        value++;
        value_len--;
    }

    // If our input string is smaller than the destination, pad with spaces and only copy what we need.
    if (value_len < size) {size = value_len;}

    memcpy((char*)ATA_INFO_BLOCK + offset, value, size);
}
static int parse_config(void* user, const char* section, const char* name, const char* value){
    if (strcmp(section, "ATA_HDD") == 0) {
        if (value == NULL) {
            return 0;
        }

        if (strcmp(name, "atainfo_model") == 0) {
            update_ata_info_block(value, 0x14, 0x14);
            DBG_printf("[%s:%s] Loaded HDD Model: %s", __FILE__, __FUNCTION__, value);
        } else if (strcmp(name, "atainfo_firmware") == 0) {
            update_ata_info_block(value, 0x2E, 8);
            DBG_printf("[%s:%s] Loaded HDD Firmware: %s", __FILE__, __FUNCTION__, value);
        } else if (strcmp(name, "atainfo_serial") == 0) {
            update_ata_info_block(value, 0x36, 0x24);
            DBG_printf("[%s:%s] Loaded HDD Serial: %s", __FILE__, __FUNCTION__, value);
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  return entries;
}