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

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>

#include "fake_smbios.h"

static char motherboard_vendor[128]={0x00};
static char motherboard_product[128]={0x00};
static char cpuinfo[128]={0x00};
static int cpu_mhz = 0;

static unsigned int ram_mb = 0;

static char gpu_name[1024] = {0x00};
static char fake_dev_mem_path[1024];
static char fake_cpuinfo_path[1024];
static char fake_board_vendor_path[1024];
static char fake_board_name_path[1024];
static char fake_systab_path[1024];

typedef int (*sysinfo_t)(struct sysinfo *info);
sysinfo_t next_sysinfo;
typedef int (*open_func_t)(const char *, int);
open_func_t next_open;
typedef const GLubyte *(*glGetString_func)(GLenum name);
glGetString_func next_glGetString;

// Redirection of /dev/mem in the game binary (yes I know)
// This points at our fake smbios block data now.
int redirect_dev_mem_open(const char *pathname, int flags) {
    if(strcmp(pathname,"/dev/mem") == 0){
        return next_open(fake_dev_mem_path,flags);
    }
    return next_open(pathname,flags);
}

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
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", redirect_dev_mem_open, &next_open, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "sysinfo", fake_sysinfo, &next_sysinfo, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libGL.so.1","glGetString", fake_glGetString, &next_glGetString, 1), 
    {}
};


static int parse_config(void* user, const char* section, const char* name, const char* value){
    char* ptr;
    if (strcmp(section, "SYSTEM_INFO") == 0) {
        if (value == NULL) {
            return 0;
        }

        if(strcmp(name,"ram_mb") == 0){
            ram_mb = strtoul(value,&ptr,10);
        }
        if(strcmp(name,"cpu_mhz") == 0){
            cpu_mhz = strtoul(value,&ptr,10);
        }
        if(strncmp(name,"cpu_name",128) == 0){
            strcpy(cpuinfo,value);
        } 

        if(strncmp(name,"motherboard_vendor",128) == 0){
            strcpy(motherboard_vendor,value);
        } 

        if(strncmp(name,"motherboard_product",128) == 0){
            strcpy(motherboard_product,value);
        } 
        if(strcmp(name,"gpu_name") == 0){
            strcpy(gpu_name,value);
        }       
    }
    return 1;
}

 
const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}

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
    sprintf(fake_dev_mem_path,"${SAVE_ROOT_PATH}/%s","fake_smbios_table");
    piutools_resolve_path(fake_dev_mem_path,fake_dev_mem_path);
    generate_fake_smbios(motherboard_vendor,motherboard_product,cpuinfo,cpu_mhz,fake_dev_mem_path);
    
    // Create a Fake Systab 
    sprintf(fake_systab_path,"${SAVE_ROOT_PATH}/%s","fake_systab");
    piutools_resolve_path(fake_systab_path,fake_systab_path);
    FILE* fp = fopen(fake_systab_path,"wb");
    const char* systab_line = "SMBIOS=0x00000000\n";
    fwrite(systab_line,strlen(systab_line),1,fp);
    fclose(fp);

    // Redirect systab 
    PIUTools_Filesystem_Add("/proc/efi/systab",fake_systab_path);    
    PIUTools_Filesystem_Add("/sys/firmware/efi/systab",fake_systab_path);  

    // Create a Fake cpuinfo file
    sprintf(fake_cpuinfo_path,"${SAVE_ROOT_PATH}/%s","fake_cpuinfo");
    piutools_resolve_path(fake_cpuinfo_path,fake_cpuinfo_path);
    fp = fopen(fake_cpuinfo_path,"wb");
    fwrite(cpuinfo,strlen(cpuinfo),1,fp);
    fclose(fp);
    PIUTools_Filesystem_Add("/proc/cpuinfo",fake_cpuinfo_path);   

    // Create a Fake Board Vendor File
    sprintf(fake_board_vendor_path,"${SAVE_ROOT_PATH}/%s","fake_board_vendor");
    piutools_resolve_path(fake_board_vendor_path,fake_board_vendor_path);
    fp = fopen(fake_board_vendor_path,"wb");
    fwrite(motherboard_vendor,strlen(motherboard_vendor),1,fp);
    fclose(fp);
    PIUTools_Filesystem_Add("/sys/class/dmi/id/board_vendor",fake_board_vendor_path); 

    // Create a Fake Board Product File
    sprintf(fake_board_name_path,"${SAVE_ROOT_PATH}/%s","fake_board_name");
    piutools_resolve_path(fake_board_name_path,fake_board_name_path);
    fp = fopen(fake_board_name_path,"wb");
    fwrite(motherboard_product,strlen(motherboard_product),1,fp);
    fclose(fp);
    PIUTools_Filesystem_Add("/sys/class/dmi/id/board_name",fake_board_name_path); 

    return entries;
}