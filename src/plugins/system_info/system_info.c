// Plugin to Spoof System Info for Prime+
// Example Setting based on an old machine:
/*
[SYSTEM_INFO]
ram_mb=1024
cpu_mhz=2600
cpu_name="Intel(R) Celeron(R) CPU     E3400"
gpu_name="GeForce 210/PCIe/SSE2"
motherboard_vendor=AsRock
motherboard_product=G41C-GS
*/

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <GL/gl.h>

#include <PIUTools_SDK.h>

#include "fake_smbios.h"

static char motherboard_vendor[128]={0x00};
static char motherboard_product[128]={0x00};
static char mac_address[128]={0x00};
static char cpuinfo[128]={0x00};
static int cpu_mhz = 0;

static unsigned int ram_mb = 0;

static char gpu_name[1024] = {0x00};
static char fake_dev_mem_path[1024];
static char fake_cpuinfo_path[1024];
static char fake_board_vendor_path[1024];
static char fake_board_name_path[1024];
static char fake_mac_address_path[1024];
static char fake_systab_path[1024];

typedef int (*sysinfo_t)(struct sysinfo *info);
sysinfo_t next_sysinfo;
typedef const GLubyte *(*glGetString_func)(GLenum name);
glGetString_func next_glGetString;


// Capture sysinfo and modify the amount of reported available RAM.
// The game displays this information in KB,  info.totalram / (1024 * info.mem_unit);
// By default, unit is a page size now so this no longer works right.
// The patch also sets the unit to 1 and standardizes the memory amount.
int fake_sysinfo(struct sysinfo *info){
    int res = next_sysinfo(info);
    size_t ram_bytes = ram_mb * (1024*1024);
    info->totalram = ram_bytes;
    info->mem_unit = 1;
    return 0;
}


//glGetString(GL_RENDERER) to spoof the GPU Name
const GLubyte *fake_glGetString(GLenum name) {
    if (name == GL_RENDERER && strlen(gpu_name) > 0) {
        return (const GLubyte *)gpu_name;
    }

    // Call the original glGetString function for other cases
    return next_glGetString(name);
}

// We only need a few hooks here.
static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "sysinfo", fake_sysinfo, &next_sysinfo, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glGetString", fake_glGetString, &next_glGetString, 1), 
    {}
};




 static HookConfigEntry plugin_config[] = {
    CONFIG_ENTRY("SYSTEM_INFO","motherboard_vendor",CONFIG_TYPE_STRING,motherboard_vendor,sizeof(motherboard_vendor)),
    CONFIG_ENTRY("SYSTEM_INFO","motherboard_product",CONFIG_TYPE_STRING,motherboard_product,sizeof(motherboard_product)),  
    CONFIG_ENTRY("SYSTEM_INFO","cpu_name",CONFIG_TYPE_STRING,cpuinfo,sizeof(cpuinfo)),
    CONFIG_ENTRY("SYSTEM_INFO","cpu_mhz",CONFIG_TYPE_INT,&cpu_mhz,sizeof(cpu_mhz)),
    CONFIG_ENTRY("SYSTEM_INFO","gpu_name",CONFIG_TYPE_STRING,gpu_name,sizeof(gpu_name)),      
    CONFIG_ENTRY("SYSTEM_INFO","mac_address",CONFIG_TYPE_STRING,mac_address,sizeof(mac_address)),  
    CONFIG_ENTRY("SYSTEM_INFO","ram_mb",CONFIG_TYPE_INT,&ram_mb,sizeof(ram_mb)),
  {}
};

const PHookEntry plugin_init(void){
    PIUTools_Config_Read(plugin_config);
    // Set some Defaults
    if(strlen(motherboard_vendor) < 1){
        strcpy(motherboard_vendor,"PIUTools");
    }
    if(strlen(motherboard_product) < 1){
        strcpy(motherboard_product,"Pump it Up PC");
    }
    if(ram_mb == 0){
        ram_mb = 1024;
    }
    if(cpu_mhz < 1){
        cpu_mhz = 1800;
    }
    if(strlen(cpuinfo) < 1){
        strcpy(cpuinfo,"Intel Celeron 430");
    }    

    // Create Save Path for fake /dev/mem
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_smbios_table",fake_dev_mem_path);
    generate_fake_smbios(motherboard_vendor,motherboard_product,cpuinfo,cpu_mhz,fake_dev_mem_path);
    PIUTools_Filesystem_AddRedirect("/dev/mem",fake_dev_mem_path);

    // Create a Fake Systab 
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_systab",fake_systab_path);
    FILE* fp = fopen(fake_systab_path,"wb");
    const char* systab_line = "SMBIOS=0x00000000\n";
    fwrite(systab_line,strlen(systab_line),1,fp);
    fclose(fp);

    // Redirect systab 
    PIUTools_Filesystem_AddRedirect("/proc/efi/systab",fake_systab_path);    
    PIUTools_Filesystem_AddRedirect("/sys/firmware/efi/systab",fake_systab_path);  

    // Create a Fake cpuinfo file
    char cpu_info_str[1024] = {0x00};
    sprintf(cpu_info_str,"model name      : %s",cpuinfo);
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_cpuinfo",fake_cpuinfo_path);
    fp = fopen(fake_cpuinfo_path,"wb");
    fwrite(cpu_info_str,strlen(cpu_info_str),1,fp);
    fclose(fp);
    PIUTools_Filesystem_AddRedirect("/proc/cpuinfo",fake_cpuinfo_path);   

    // Create a Fake Board Vendor File
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_board_vendor",fake_board_vendor_path);
    fp = fopen(fake_board_vendor_path,"wb");
    fwrite(motherboard_vendor,strlen(motherboard_vendor),1,fp);
    fclose(fp);
    PIUTools_Filesystem_AddRedirect("/sys/class/dmi/id/board_vendor",fake_board_vendor_path); 

    // Create a Fake Board Product File
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_board_name",fake_board_name_path);
    fp = fopen(fake_board_name_path,"wb");
    fwrite(motherboard_product,strlen(motherboard_product),1,fp);
    fclose(fp);
    PIUTools_Filesystem_AddRedirect("/sys/class/dmi/id/board_name",fake_board_name_path); 

    // Create a Fake MAC Address if it exists
    if(strlen(mac_address) > 1){
        PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_eth0_address",fake_mac_address_path);
    fp = fopen(fake_mac_address_path,"wb");
    fwrite(mac_address,strlen(mac_address),1,fp);
    fclose(fp);
    PIUTools_Filesystem_AddRedirect("/sys/class/net/eth0/address",fake_mac_address_path);  
    }


    return entries;
}