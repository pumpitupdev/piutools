// USB Profile Plugin for NX2 and NXA
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_USB.h>

#include "nx2_rank.h"
#include "nx2_save.h"

#define USB_SERIAL_P1 "12345678"
#define USB_SERIAL_P2 "87654321"

typedef struct _PLAYER_USB_INFO{
    char enabled;
    char name[32];
    char serial[64];
}PlayerUSBInfo;

enum _USB_PROFILE_TYPE{
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


static HookEntry entries[] = {
    //HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","ioctl", ticket_ioctl, &next_ioctl, 1),    
    //HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", ticket_open, &next_open, 1),
    {}    
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if (strcmp(section, "USB_PROFILE") == 0) {
        if (value == NULL) {
            return 0;
        }

        if (strcmp(name, "p1_name") == 0) {
            int diff = strlen(value);
            if(diff > sizeof(p1_name)){
                diff = sizeof(p1_name);
            }
            memcpy(p1_name,value,diff);
        } 
        if (strcmp(name, "p2_name") == 0) {
            int diff = strlen(value);
            if(diff > sizeof(p2_name)){
                diff = sizeof(p2_name);
            }
            memcpy(p2_name,value,diff);
        }   
        if (strcmp(name, "game") == 0) {
            int diff = strlen(value);
            if(diff > sizeof(p2_name)){
                diff = sizeof(p2_name);
            }
            strncpy(game_type,value,sizeof(game_type));
        }               
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  p1_enabled = strlen(p1_name) > 0;
  p2_enabled = strlen(p2_name) > 0;

  //init_ticket_state(starting_tickets);
  // Make Fake USB Device for Tickets
  PUSBDevice nd = PIUTools_USB_Add_Device(USB_2_SPEED,0,0x58F,0x1004,"TKT123",NULL,NULL,NULL);
  //sprintf(fake_ticket_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
  //printf("[%s] Created Fake Ticket Device At: %s\n",__FILE__,fake_ticket_device_path);
  return entries;
}