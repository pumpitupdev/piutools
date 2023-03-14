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

static HookEntry entries[] = {
    {"libc.so.6","ioctl",(void*)ata_hdd_ioctl,(void*)&next_ioctl},
    {"libc.so.6","open",(void*)ata_hdd_open,(void*)&next_open}  
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
    }
    return 1;
}

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    if(ini_parse(config_path,parse_config,NULL) != 0){return 0;}
    *hook_entry_table = entries;
    return sizeof(entries) / sizeof(HookEntry);
}

