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

static char HDD_FILE_PATH[1024] = {0x00};
static char ATA_FILE_PATH[1024] = {0x00};
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

typedef int (*open_func_t)(const char*, int, ...);
static open_func_t next_open;
static int ata_hdd_open(const char *pathname, int flags) {   
    if(pathname != NULL && strcmp(pathname,"/dev/hda") == 0){
        return next_open(HDD_FILE_PATH,flags);
    }       
    return next_open(pathname, flags);
}

// Load HDD Info from HDD ATA File
static int load_hdd_info(void){
    FILE* fp = fopen(ATA_FILE_PATH,"rb");
    if(fp == NULL){
        DBG_printf("[%s:%s] Error Loading Fake ATA File: %s",__FILE__,__FUNCTION__,ATA_FILE_PATH);
        return 0;
    }
    memset(ATA_INFO_BLOCK,0x00,512);
    fread(ATA_INFO_BLOCK,512,1,fp);
    fclose(fp);    
    // Read in Model
    DBG_printf("[%s:%s] Loaded HDD Model: %s",__FILE__,__FUNCTION__,(char*)ATA_INFO_BLOCK+0x14);
    // Read in Firmware
    DBG_printf("[%s:%s] Loaded HDD Firmware: %s",__FILE__,__FUNCTION__,(char*)ATA_INFO_BLOCK+0x2E);
    // Read in Serial
    DBG_printf("[%s:%s] Loaded HDD Serial: %s",__FILE__,__FUNCTION__,(char*)ATA_INFO_BLOCK+0x36);
    return 1;
}

/* 
    NX had a couple of images (v1.05 so far) that, for whatever reason, 
    used a HDD where the model number had leading spaces which isn't ATA spec.
    This is clearly how the reporting was supposed to work considering the drive info
    on disk is the same, yet the game's hdd check routine fails on reading this for whatever reason.
    Instead, for... who knows why, if the leading space is swapped for trailing space and you start
    the buffer with the model number, it validates... wtf...
*/
void do_nx_hdd_fix(void) {
    unsigned char new_model[0x14] = {0x20};
    for (int i = 0; i < sizeof(new_model); i++) {
        if (ATA_INFO_BLOCK[i + 0x14] != ' ') {
            int remaining_bytes = sizeof(new_model) - i;
            memcpy(new_model, ATA_INFO_BLOCK + i + 0x14, remaining_bytes);
            memcpy(ATA_INFO_BLOCK + 0x14, new_model, sizeof(new_model));
            return;
        }
    }
}

static HookEntry entries[] = {
    {"libc.so.6","ioctl",(void*)ata_hdd_ioctl,(void*)&next_ioctl,1},
    {"libc.so.6","open",(void*)ata_hdd_open,(void*)&next_open,1}  
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"ATA_HDD") == 0){
        if(strcmp(name,"hdd_file") == 0){
            if(value == NULL){return 0;}
            realpath(value, HDD_FILE_PATH);          
        }
        if(strcmp(name,"ata_file") == 0){
            if(value == NULL){return 0;}
            realpath(value, ATA_FILE_PATH);          
            return load_hdd_info();
        }  

        if(strcmp(name,"nx_hdd_fix") == 0){
            if(value == NULL){return 0;}
            char *ptr; 
            unsigned long lv = strtoul(value,&ptr,10);
            if(lv == 1){
                do_nx_hdd_fix();
            }                       
        }                 
    }
    return 1;
}

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    if(ini_parse(config_path,parse_config,NULL) != 0){return 0;}
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}

