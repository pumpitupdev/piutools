// Logic for Handling Fake Mounts and Mountpoints
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <PIUTools_SDK.h>


static int module_initialized = 0;
static MountsEntry mount_entries[MAX_MOUNT_ENTRIES];
static char fake_mount_path[1024] = {0x00};

static void mount_init(void){
    if(module_initialized){return;}
    module_initialized = 1;
    // Addition for Mounts
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/mounts",fake_mount_path);
    PIUTools_Filesystem_AddRedirect("/proc/mounts",fake_mount_path);        
}

static void update_mount_file(void){
    char line[512]={0x00};
    FILE* fp = fopen(fake_mount_path,"wb");
    if(!fp){return;}
    for(int i=0;i<MAX_MOUNT_ENTRIES;i++){
        if(mount_entries[i].enabled){
            sprintf(line,"%s %s\n",mount_entries[i].from,mount_entries[i].to);
            fwrite(line,strlen(line),1,fp);
        }
    }
    fclose(fp);
}

void PIUTools_Mount_RemoveEntry(PMountsEntry entry){
    if(!module_initialized){mount_init();}    
    entry->enabled = 0;
    update_mount_file();
}

PMountsEntry PIUTools_Mount_AddEntry(const char* mount_from, const char* mount_to){
    if(!module_initialized){mount_init();}
    for(int i=0; i < MAX_MOUNT_ENTRIES; i++){
        if(mount_entries[i].enabled == 0){
            strncpy(mount_entries[i].from,mount_from,sizeof(mount_entries[i].from));
            strncpy(mount_entries[i].to,mount_to,sizeof(mount_entries[i].to));
            mount_entries[i].enabled = 1;
            update_mount_file();
            return &mount_entries[i];
        }
    }
    return NULL;
}

