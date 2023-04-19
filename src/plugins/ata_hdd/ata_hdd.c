// ATA Info Hook for KPUMP Prior to Prime2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <PIUTools_SDK.h>

static unsigned char ATA_INFO_BLOCK[512] = {0x00};
static char HDD_GAME_LABEL[256] = {0x00};
static char fake_hdd_file_path[1024] = {0x00};
static char atainfo_serial[21];
static char atainfo_model[37];
static char atainfo_firmware[9];

// Handle Our HDD Security Calls
typedef int (*ioctl_func_t)(int, unsigned long, ...);
static ioctl_func_t next_ioctl;

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

static void pad_hdd_file(FILE *file, size_t target_size) {
    size_t current_size = ftell(file);
    if (current_size < target_size) {
        size_t padding_size = target_size - current_size;
        char *buffer = (char *)calloc(padding_size, 1);
        fwrite(buffer, 1, padding_size, file);
        free(buffer);
    }
}

static void create_hdd_file(void){
    int name_offset = 0x200;
    int ata_offset = 0x300;
    FILE* fp = fopen(fake_hdd_file_path,"wb");
    
    fseek(fp,name_offset,0);
    fwrite(HDD_GAME_LABEL,sizeof(HDD_GAME_LABEL),1,fp);
    // Write ATA INFO
    fwrite(ATA_INFO_BLOCK+0x14,20,1,fp);
    fwrite(ATA_INFO_BLOCK+0x2E,8,1,fp);
    fwrite(ATA_INFO_BLOCK+0x36,36,1,fp);

    // Pad file out to a reasonable size
    size_t padded_size = 0x2180000;
    if(strstr(HDD_GAME_LABEL,"Fiesta") || strstr(HDD_GAME_LABEL,"PRIME")){
        pad_hdd_file(fp,padded_size);
    }else{
        pad_hdd_file(fp,2048);
    }
    fclose(fp);
}

#ifndef HDIO_GET_IDENTITY
#define HDIO_GET_IDENTITY 0x030D
#endif

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

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("ATA_HDD","atainfo_serial",CONFIG_TYPE_STRING,atainfo_serial,sizeof(atainfo_serial)),
  CONFIG_ENTRY("ATA_HDD","atainfo_firmware",CONFIG_TYPE_STRING,atainfo_firmware,sizeof(atainfo_firmware)),
  CONFIG_ENTRY("ATA_HDD","atainfo_model",CONFIG_TYPE_STRING,atainfo_model,sizeof(atainfo_model)),
  CONFIG_ENTRY("ATA_HDD","hdd_label",CONFIG_TYPE_STRING,HDD_GAME_LABEL,sizeof(HDD_GAME_LABEL)),      
  {}
};

const PHookEntry plugin_init(void){
    PIUTools_Config_Read(plugin_config);

    DBG_printf("[%s] Loaded HDD Serial: %s", __FILE__, atainfo_serial);
    DBG_printf("[%s] Loaded HDD Firmware: %s", __FILE__, atainfo_firmware);
    DBG_printf("[%s] Loaded HDD Model: %s", __FILE__, atainfo_model);
    DBG_printf("[%s] Loaded HDD Label: %s", __FILE__, HDD_GAME_LABEL);

    // Create our ATA Data
    update_ata_info_block(atainfo_serial, 0x14, 0x14);
    update_ata_info_block(atainfo_firmware, 0x2E, 8);
    update_ata_info_block(atainfo_model, 0x36, 0x24);
    
    // Add Mount Entry for HDD
    PIUTools_Mount_AddEntry("/dev/hda1","/mnt/hd");
    // Redirect Direct Access Requests
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/hdd.bin",fake_hdd_file_path);
    PIUTools_Filesystem_AddRedirect("/dev/hda",fake_hdd_file_path);
    PIUTools_Filesystem_AddRedirect("/dev/sda",fake_hdd_file_path);

    // Create Fake Writable HDD File
    DBG_printf("[%s] FAKE HDD FILE PATH:%s",__FILE__, fake_hdd_file_path);
    create_hdd_file();

    return entries;
}