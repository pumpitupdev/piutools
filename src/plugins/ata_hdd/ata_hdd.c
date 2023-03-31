// ATA Info Hook for KPUMP Prior to Prime2
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>

#ifndef HDIO_GET_IDENTITY
#define HDIO_GET_IDENTITY 0x030D
#endif

static unsigned char ATA_INFO_BLOCK[512] = {0x00};
static char HDD_GAME_LABEL[256] = {0x00};
static char fake_hdd_file_path[1024] = {0x00};

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

void pad_hdd_file(FILE *file, size_t target_size) {
    size_t current_size = ftell(file);
    if (current_size < target_size) {
        size_t padding_size = target_size - current_size;
        char *buffer = (char *)calloc(padding_size, 1);
        fwrite(buffer, 1, padding_size, file);
        free(buffer);
    }
}

void create_hdd_file(void){
    int name_offset = 0x200;
    int ata_offset = 0x300;
    FILE* fp = fopen(fake_hdd_file_path,"wb");
    
    fseek(fp,name_offset,0);
    fwrite(HDD_GAME_LABEL,sizeof(HDD_GAME_LABEL),1,fp);
    // Write ATA INFO
    fwrite(ATA_INFO_BLOCK+0x14,20,1,fp);
    fwrite(ATA_INFO_BLOCK+0x2E,8,1,fp);
    fwrite(ATA_INFO_BLOCK+0x36,36,1,fp);

    // Pad file out to certain size
    size_t padded_size = 0x2180000;
    if(strstr(HDD_GAME_LABEL,"Fiesta") || strstr(HDD_GAME_LABEL,"Prime")){
        pad_hdd_file(fp,padded_size);
    }
    fclose(fp);
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
        }else if(strcmp(name,"hdd_label") == 0){
            strncpy(HDD_GAME_LABEL,value,sizeof(HDD_GAME_LABEL));
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}

    // Add Mount Entry for HDD
    PIUTools_Filesystem_AddMountEntry("/dev/hda1","/mnt/hd");

    // Create Fake Writable HDD File
    piutools_resolve_path("${SAVE_ROOT_PATH}/hdd.bin",fake_hdd_file_path);
    create_hdd_file();
    // Add Redirect Entries
    PIUTools_Filesystem_Add("/dev/hda",fake_hdd_file_path);
    PIUTools_Filesystem_Add("/dev/sda",fake_hdd_file_path);
    return entries;
}