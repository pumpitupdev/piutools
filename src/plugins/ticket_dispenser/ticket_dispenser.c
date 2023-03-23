// Plugin for Ticket Dispenser Support
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_USB.h>


#include "ticket.h"

#define FAKE_TICKET_FD 0x99884
char fake_ticket_device_path[1024] = {0x00};

typedef int (*ioctl_func_t)(int fd, int request, void* data);
static ioctl_func_t next_ioctl;

typedef int (*open_func_t)(const char *, int);
open_func_t next_open;

static int dispenser_enabled;
static int starting_tickets = 100;

static int ticket_ioctl(int fd, int request, void* data) {   
    if(fd == FAKE_TICKET_FD){
        parse_ticketcmd((unsigned char*)data);
        return 0;
    }
    return next_ioctl(fd, request, data);
}

static int ticket_libusb(void *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout){
    struct ticket_usbdevfs_ctrltransfer ctrl;
    ctrl.bRequestType = requesttype;
    ctrl.data = bytes;
    ctrl.timeout = timeout;
    ctrl.wIndex = index;
    ctrl.wLength = size;
    ctrl.bRequest = request;

    parse_ticketcmd(&ctrl);
    return 0;
}

int ticket_open(const char *pathname, int flags) {
    // Ticket Endpoint Handling
    if(strcmp(pathname,fake_ticket_device_path) == 0){
        return FAKE_TICKET_FD;
    }
    return next_open(pathname, flags);
}


static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"TICKET_DISPENSER") == 0){ 
        if(value == NULL){return 0;}       

        if(strcmp(name,"starting_tickets") == 0){
            char *ptr;            
            starting_tickets = strtoul(value,&ptr,10);
        }
    }
    return 1;
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","ioctl", ticket_ioctl, &next_ioctl, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", ticket_open, &next_open, 1),
    {}    
};

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  init_ticket_state(starting_tickets);
  // Make Fake USB Device for Tickets
  PUSBDevice nd = PIUTools_USB_Add_Device(USB_2_SPEED,0,0x0d2f,0x1004,"TKT123",(void*)ticket_libusb,NULL,NULL);
  sprintf(fake_ticket_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
  printf("[%s] Created Fake Ticket Device At: %s\n",__FILE__,fake_ticket_device_path);
  return entries;
}