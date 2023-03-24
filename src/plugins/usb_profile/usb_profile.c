// USB Profile Plugin for NX2 and NXA
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_USB.h>
#include <plugin_sdk/PIUTools_Filesystem.h>
#include <plugin_sdk/PIUTools_Input.h>

#include "nx2/nx2_profile.h"

#define USB_SERIAL_P1 "1NBJZ4PL"
#define USB_SERIAL_P2 "1FGAT0YX"
#define USB_DEFAULT_DEV_P1 "/dev/sdb1"
#define USB_DEFAULT_DEV_P2 "/dev/sdc1"
#define USB_DEFAULT_MNT_P1 "/mnt/0"
#define USB_DEFAULT_MNT_P2 "/mnt/1"

#define USB_VID 0x58F
#define USB_PID 0x6387

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

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if (strcmp(section, "USB_PROFILE") == 0) {
        if (value == NULL) {
            return 0;
        }
        if (strcmp(name, "game_type") == 0) {
            if(strcmp(value,"nx2") == 0){
                PlayerUSB.profile_type = USB_PROFILE_NX2;
            }else if(strcmp(value,"nxa") == 0){
                PlayerUSB.profile_type = USB_PROFILE_NXA;
            }else if(strcmp(value,"fiesta") == 0){
                PlayerUSB.profile_type = USB_PROFILE_FIESTA;
            }else if(strcmp(value,"fiestaex") == 0){
                PlayerUSB.profile_type = USB_PROFILE_FIESTAEX;
            }else if(strcmp(value,"fiesta2") == 0){
                PlayerUSB.profile_type = USB_PROFILE_FIESTA2;
            }else if(strcmp(value,"prime") == 0){
                PlayerUSB.profile_type = USB_PROFILE_PRIME;
            }
        }
        if (strcmp(name, "p1_name") == 0) {
            strncpy(PlayerUSB.player[0].name,value,sizeof(PlayerUSB.player[0].name));
            PlayerUSB.player[0].enabled = 1;
        }
        if (strcmp(name, "p1_avatar") == 0) {
            char *ptr;            
            if(value != NULL){
              PlayerUSB.player[0].avatar_id = strtoul(value,&ptr,10);
            }
        }  
        if (strcmp(name, "p1_fakedev") == 0) {
            strcpy(PlayerUSB.player[0].fake_dev_path,value);
        }
        if (strcmp(name, "p2_name") == 0) {
            strncpy(PlayerUSB.player[1].name,value,sizeof(PlayerUSB.player[1].name));
            PlayerUSB.player[1].enabled = 1;
        }  
        if (strcmp(name, "p2_avatar") == 0) {
            char *ptr;            
            if(value != NULL){
              PlayerUSB.player[1].avatar_id = strtoul(value,&ptr,10);
            }
        }
        if (strcmp(name, "p2_fakedev") == 0) {
            strcpy(PlayerUSB.player[1].fake_dev_path,value);
        }                                
    }
    return 1;
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

            printf("[USB_PROFILE] Player 1 USB Profile %s\n",state_str);
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

const PHookEntry plugin_init(const char* config_path){
    PlayerUSB_Init();
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}

    // We'll set up any prep work for each player now.
    for(int i=0;i < 2; i++){
        if(PlayerUSB.player[i].enabled == 0){continue;}
        // Resolve Save Folder Path and Create if it Doesn't Exist
        sprintf(PlayerUSB.player[i].save_profile_folder_path,"${SAVE_ROOT_PATH}/%s",PlayerUSB.player[i].name);
        piutools_resolve_path(PlayerUSB.player[i].save_profile_folder_path,PlayerUSB.player[i].save_profile_folder_path);
        // Create The Save Folder Paths if They Don't Exist for Enabled Profiles.
        PIUTools_Filesystem_Create_Directory(PlayerUSB.player[i].save_profile_folder_path);
        // Create Block Device Path
        sprintf(PlayerUSB.player[i].fake_block_device_path,"${SAVE_ROOT_PATH}/blockdev.%d",i);
        piutools_resolve_path(PlayerUSB.player[i].fake_block_device_path,PlayerUSB.player[i].fake_block_device_path);
        PlayerUSB.player[i].usb_device = PIUTools_USB_Add_Device(USB_2_SPEED,USB_CLASS_MASS_STORAGE,USB_VID,USB_PID,PlayerUSB.player[i].serial,NULL,NULL,NULL);



        switch(PlayerUSB.profile_type){
            case USB_PROFILE_NX2:
                USB_Profile_Generate_NX2(PlayerUSB.player[i].save_profile_folder_path, PlayerUSB.player[i].name, PlayerUSB.player[i].serial, PlayerUSB.player[i].avatar_id);
                write_scsi_blockdev_file(PlayerUSB.player[i].serial,PlayerUSB.player[i].fake_block_device_path,PlayerUSB.player[i].connected);
                // I don't know about these - I have to look into this.
                PIUTools_Filesystem_Add(PlayerUSB.player[i].scsi_path,PlayerUSB.player[i].fake_block_device_path);
                char lun_path[128] = {0x00};
                sprintf(lun_path,"/dev/scsi/host%d/bus0/target0/lun0/part1",i);
                PIUTools_Filesystem_Add(lun_path,PlayerUSB.player[i].fake_block_device_path);                
                break;
            default:
                break;
        }
        // This Sets the Mount Entry in /proc/mounts
        PIUTools_Filesystem_AddMountEntry(PlayerUSB.player[i].fake_dev_path,PlayerUSB.player[i].mount_path);
        // This redirects the /mnt/n to our save profile
        PIUTools_Filesystem_Add(PlayerUSB.player[i].mount_path,PlayerUSB.player[i].save_profile_folder_path);        

    }
    pthread_t button_check_thread;
    pthread_create(&button_check_thread, NULL, check_button_states, NULL);
  return NULL;
}