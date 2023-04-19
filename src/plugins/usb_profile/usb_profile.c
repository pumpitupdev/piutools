// USB Profile Plugin for NX2 and NXA
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <glob.h>

#include <PIUTools_SDK.h>

#include "nx2/nx2_profile.h"
#include "nxa/nxa_profile.h"
#include "fiesta/fiesta_profile.h"
#include "fex/fex_profile.h"
#include "fiesta2/fiesta2_profile.h"
#include "prime/prime_profile.h"

#define USB_SERIAL_P1 "1NBJZ4PL"
#define USB_SERIAL_P2 "1FGAT0YX"
#define USB_DEFAULT_DEV_P1 "/dev/sdb1"
#define USB_DEFAULT_DEV_P2 "/dev/sdc1"
#define USB_DEFAULT_MNT_P1 "/mnt/0"
#define USB_DEFAULT_MNT_P2 "/mnt/1"

#define USB_VID 0x58F
#define USB_PID 0x6387


typedef int (*glob_func_t)(const char *, int, int (*)(const char *, int), glob_t *);
glob_func_t next_glob = NULL;



typedef struct _PLAYER_USB_INFO{
    PUSBDevice usb_device;
    char enabled;
    char connected;
    int avatar_id;
    char name[32];
    char serial[64];
    char save_profile_folder_path[1024];
    char fake_dev_path[64];
    char fake_block_device_path[128];
    char scsi_path[64];
    char mount_path[64];
}PlayerUSBInfo;

enum _USB_PROFILE_TYPE{
    USB_PROFILE_NONE,
    USB_PROFILE_NX2,
    USB_PROFILE_NXA,
    USB_PROFILE_FIESTA,
    USB_PROFILE_FIESTAEX,
    USB_PROFILE_FIESTA2,
    USB_PROFILE_PRIME
};

static struct _PLAYER_USB{
    int profile_type;
    PlayerUSBInfo player[2];
}PlayerUSB;

// Setup both player structures with defaults.
void PlayerUSB_Init(void){
    PlayerUSB.profile_type = USB_PROFILE_NONE;
    PlayerUSB.player[0].enabled = 0;
    PlayerUSB.player[0].connected = 0;
    strcpy(PlayerUSB.player[0].scsi_path,"/proc/scsi/usb-storage-0/0");
    strcpy(PlayerUSB.player[0].serial,USB_SERIAL_P1);
    strcpy(PlayerUSB.player[0].fake_dev_path,USB_DEFAULT_DEV_P1);
    strcpy(PlayerUSB.player[0].mount_path,USB_DEFAULT_MNT_P1);
    PlayerUSB.player[1].enabled = 0;
    PlayerUSB.player[1].connected = 0;
    strcpy(PlayerUSB.player[1].serial,USB_SERIAL_P2);  
    strcpy(PlayerUSB.player[1].fake_dev_path,USB_DEFAULT_DEV_P2);
    strcpy(PlayerUSB.player[1].mount_path,USB_DEFAULT_MNT_P2);
    strcpy(PlayerUSB.player[1].scsi_path,"/proc/scsi/usb-storage-1/1");    
}


void write_scsi_blockdev_file(const char* serial_number, const char* scsi_path, int state){
    FILE* fp = fopen(scsi_path,"wb");
    const char* attached_str = "     Attached: ";
    char serial_str[1024] = {0x00};
    sprintf(serial_str,"Serial Number: %s\n",serial_number);
    fwrite(serial_str,strlen(serial_str),1,fp);
    fwrite(attached_str,strlen(attached_str),1,fp);
    const char* str_yes = "Yes\n";
    const char* str_no = "No\n";
    if(state){
        fwrite(str_yes,strlen(str_yes),1,fp);
    }else{
        fwrite(str_no,strlen(str_no),1,fp);
    }
    fclose(fp);
}

void update_blkdev_file(int player_index){
    if(PlayerUSB.profile_type == USB_PROFILE_NX2){        
        write_scsi_blockdev_file(PlayerUSB.player[player_index].serial,PlayerUSB.player[player_index].fake_block_device_path,PlayerUSB.player[player_index].connected);
    }
}

// Our USB State Thread
void *check_button_states(void *arg) {
    int prev_p1_usb_state = 0;
    int prev_p2_usb_state = 0;

    while (1) {
        int p1_usb_state = PIUTools_IO_IN[PINPUT_P1_USB_IN];
        int p2_usb_state = PIUTools_IO_IN[PINPUT_P2_USB_IN];

        if (p1_usb_state && !prev_p1_usb_state) {
            // Toggle
            PlayerUSB.player[0].connected ^= 1;
            const char* state_str = "IN";
            if(!PlayerUSB.player[0].connected){
                state_str = "OUT";
            }

            DBG_printf("[USB_PROFILE] Player 1 USB Profile %s\n",state_str);
            if(PlayerUSB.player[0].connected){
                PIUTools_USB_Connect_Device(PlayerUSB.player[0].usb_device->dev);  
            }else{
                PIUTools_USB_Disconnect_Device(PlayerUSB.player[0].usb_device->dev);
            }
            write_scsi_blockdev_file(PlayerUSB.player[0].serial,PlayerUSB.player[0].fake_block_device_path,PlayerUSB.player[0].connected);
        }

        if (p2_usb_state && !prev_p2_usb_state) {
            // Toggle
            PlayerUSB.player[1].connected ^= 1;
            const char* state_str = "IN";
            if(!PlayerUSB.player[1].connected){
                state_str = "OUT";
            }

            printf("[USB_PROFILE] Player 2 USB Profile %s\n",state_str);
            if(PlayerUSB.player[1].connected){
                PIUTools_USB_Connect_Device(PlayerUSB.player[1].usb_device->dev);  
            }else{
                PIUTools_USB_Disconnect_Device(PlayerUSB.player[1].usb_device->dev);
            }
            write_scsi_blockdev_file(PlayerUSB.player[1].serial,PlayerUSB.player[1].fake_block_device_path,PlayerUSB.player[1].connected);
        }

        prev_p1_usb_state = p1_usb_state;
        prev_p2_usb_state = p2_usb_state;
        usleep(500 * 1000);
    }

    return NULL;
}

int usb_profile_glob(const char *pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob) {
    // Check if the pattern matches your desired pattern
    //printf("GLOB: %s\n",pattern);

    if (strncmp(pattern, "/sys/bus/usb/devices",strlen("/sys/bus/usb/devices")) == 0) {
        // Clear the glob_t structure
        memset(pglob, 0, sizeof(glob_t));
        char dev_path[32] = {0x00};
        if(PlayerUSB.player[0].enabled){
            char sys_usb_path_p1[64];
            sprintf(sys_usb_path_p1,"/sys/bus/usb/devices/99-%d",PlayerUSB.player[0].usb_device->dev+1);
            if (strncmp(pattern, sys_usb_path_p1,strlen(sys_usb_path_p1)) == 0) {
                sscanf(PlayerUSB.player[0].fake_dev_path, "/dev/%[^1-9]", dev_path);
            }
        }
        if(PlayerUSB.player[1].enabled){
            char sys_usb_path_p2[64];
            sprintf(sys_usb_path_p2,"/sys/bus/usb/devices/99-%d",PlayerUSB.player[1].usb_device->dev+1);
            if (strncmp(pattern, sys_usb_path_p2,strlen(sys_usb_path_p2)) == 0) {
                sscanf(PlayerUSB.player[1].fake_dev_path, "/dev/%[^1-9]", dev_path);
            }
        }


        // Set the hardcoded path as the result
        pglob->gl_pathc = 1;
        pglob->gl_pathv = malloc(2 * sizeof(char *));
        pglob->gl_pathv[0] = strdup(dev_path);
        pglob->gl_pathv[1] = NULL;

        // Return success (0)
        return 0;
    }

    // If the pattern doesn't match, call the original glob function
    int result = next_glob(pattern, flags, errfunc, pglob);
    return result;
}

typedef int (*mount_func_t)(const char *source, const char *target,
                            const char *filesystemtype, unsigned long mountflags,
                            const void *data);
mount_func_t next_mount;
typedef int (*umount_func_t)(const char *target);
umount_func_t next_umount;

int usb_profile_mount(const char *source, const char *target,
                            const char *filesystemtype, unsigned long mountflags,
                            const void *data){

                                printf("[%s] Block Mount %s %s\n",__FUNCTION__,source,target);
                                return 0;
}

int usb_profile_umount(const char* target){
                                printf("[%s] Block Umount: %s\n",__FUNCTION__,target);
                                return 0;
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "glob", usb_profile_glob, &next_glob, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "mount", usb_profile_mount, &next_mount, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "umount", usb_profile_umount, &next_umount, 1),
    {}
};

static char config_game_type[32] = {0x00};
static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("USB_PROFILE","game_type",CONFIG_TYPE_STRING,config_game_type,sizeof(config_game_type)),
  CONFIG_ENTRY("USB_PROFILE","p1_name",CONFIG_TYPE_STRING,PlayerUSB.player[0].name,sizeof(PlayerUSB.player[0].name)),
  CONFIG_ENTRY("USB_PROFILE","p2_name",CONFIG_TYPE_STRING,PlayerUSB.player[1].name,sizeof(PlayerUSB.player[1].name)),
  CONFIG_ENTRY("USB_PROFILE","p1_fakedev",CONFIG_TYPE_STRING,PlayerUSB.player[0].fake_dev_path,sizeof(PlayerUSB.player[0].fake_dev_path)),
  CONFIG_ENTRY("USB_PROFILE","p2_fakedev",CONFIG_TYPE_STRING,PlayerUSB.player[1].fake_dev_path,sizeof(PlayerUSB.player[1].fake_dev_path)),
  CONFIG_ENTRY("USB_PROFILE","p1_avatar",CONFIG_TYPE_INT,&PlayerUSB.player[0].avatar_id,sizeof(PlayerUSB.player[0].avatar_id)),  
  CONFIG_ENTRY("USB_PROFILE","p2_avatar",CONFIG_TYPE_INT,&PlayerUSB.player[1].avatar_id,sizeof(PlayerUSB.player[1].avatar_id)),
  {}
};

const PHookEntry plugin_init(void){
    PlayerUSB_Init();
    PIUTools_Config_Read(plugin_config);    
    if(strcmp(config_game_type,"nx2") == 0){
        PlayerUSB.profile_type = USB_PROFILE_NX2;
    }else if(strcmp(config_game_type,"nxa") == 0){
        PlayerUSB.profile_type = USB_PROFILE_NXA;
    }else if(strcmp(config_game_type,"fiesta") == 0){
        PlayerUSB.profile_type = USB_PROFILE_FIESTA;
    }else if(strcmp(config_game_type,"fiestaex") == 0){
        PlayerUSB.profile_type = USB_PROFILE_FIESTAEX;
    }else if(strcmp(config_game_type,"fiesta2") == 0){
        PlayerUSB.profile_type = USB_PROFILE_FIESTA2;
    }else if(strcmp(config_game_type,"prime") == 0){
        PlayerUSB.profile_type = USB_PROFILE_PRIME;
    }

    if(strlen(PlayerUSB.player[0].name)){
        PlayerUSB.player[0].enabled = 1;
    }

    if(strlen(PlayerUSB.player[1].name)){
        PlayerUSB.player[1].enabled = 1;
    }

    // We'll set up any prep work for each player now.
    for(int i=0;i < 2; i++){
        if(PlayerUSB.player[i].enabled == 0){continue;}
        // Resolve Save Folder Path and Create if it Doesn't Exist
        sprintf(PlayerUSB.player[i].save_profile_folder_path,"${SAVE_ROOT_PATH}/%s",PlayerUSB.player[i].name);
        PIUTools_Path_Resolve(PlayerUSB.player[i].save_profile_folder_path,PlayerUSB.player[i].save_profile_folder_path);
        // Create The Save Folder Paths if They Don't Exist for Enabled Profiles.
        PIUTools_Filesystem_Create_Directory(PlayerUSB.player[i].save_profile_folder_path);
        // Create Block Device Path
        sprintf(PlayerUSB.player[i].fake_block_device_path,"${TMP_ROOT_PATH}/blockdev.%d",i);
        PIUTools_Path_Resolve(PlayerUSB.player[i].fake_block_device_path,PlayerUSB.player[i].fake_block_device_path);
        PlayerUSB.player[i].usb_device = PIUTools_USB_Add_Device(USB_2_SPEED,USB_CLASS_MASS_STORAGE,USB_VID,USB_PID,PlayerUSB.player[i].serial,NULL,NULL,NULL);


        
        switch(PlayerUSB.profile_type){
            case USB_PROFILE_NX2:
                USB_Profile_Generate_NX2(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                break;
            case USB_PROFILE_NXA:
                USB_Profile_Generate_NXA(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                break;        
            case USB_PROFILE_FIESTA:
                USB_Profile_Generate_Fiesta(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                break;
            case USB_PROFILE_FIESTAEX:            
                USB_Profile_Generate_FiestaEX(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
            break;        
            case USB_PROFILE_FIESTA2:
                USB_Profile_Generate_Fiesta2(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                break;
            case USB_PROFILE_PRIME:
                USB_Profile_Generate_PRIME(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                // TODO: Add Fake Gameserver Profile Registration Here
                break;
            default:
                break;
        }

        // Set up SCSI Path/Block Stuff
        char lun_path[128] = {0x00};
        sprintf(lun_path,"/dev/scsi/host%d/bus0/target0/lun0/part1",i);
        PIUTools_Filesystem_AddRedirect(lun_path,PlayerUSB.player[i].fake_block_device_path);                
        write_scsi_blockdev_file(PlayerUSB.player[i].serial,PlayerUSB.player[i].fake_block_device_path,PlayerUSB.player[i].connected);
        PIUTools_Filesystem_AddRedirect(PlayerUSB.player[i].scsi_path,PlayerUSB.player[i].fake_block_device_path);

        // This Sets the Mount Entry in /proc/mounts
        PIUTools_Mount_AddEntry(PlayerUSB.player[i].fake_dev_path,PlayerUSB.player[i].mount_path);
        PIUTools_Filesystem_AddRedirect(PlayerUSB.player[i].fake_dev_path,PlayerUSB.player[i].fake_block_device_path);
        // This redirects the /mnt/n to our save profile
        PIUTools_Filesystem_AddRedirect(PlayerUSB.player[i].mount_path,PlayerUSB.player[i].save_profile_folder_path); 
        // This redirects /mnt/n/ to our save profile because wtf
        char mnt_slash[1024];
        char save_slash[1024];
        sprintf(mnt_slash,"%s/",PlayerUSB.player[i].mount_path);
        sprintf(save_slash,"%s/",PlayerUSB.player[i].save_profile_folder_path);
        PIUTools_Filesystem_AddRedirect(mnt_slash,save_slash);        

    }
    pthread_t button_check_thread;
    pthread_create(&button_check_thread, NULL, check_button_states, NULL);
  return entries;
}