// Plugin for Ticket Dispenser Support
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <PIUTools_SDK.h>

#include "ticket.h"

#ifndef USBDEVFS_CONTROL
#define USBDEVFS_CONTROL 0xc0185501
#endif


static char generated_ticket_device_path[1024] = {0x00};
static char fake_ticket_device_path[1024] = {0x00};

typedef int (*ioctl_func_t)(int fd, int request, void* data);
static ioctl_func_t next_ioctl;

typedef int (*open_func_t)(const char *, int);
static open_func_t next_open;

static int dispenser_enabled;
static int starting_tickets = 100;

static int ticket_ioctl(int fd, int request, void* data) {   
    if(request == USBDEVFS_CONTROL){
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

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","ioctl", ticket_ioctl, &next_ioctl, 1),    
    {}    
};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("TICKET_DISPENSER","starting_tickets",CONFIG_TYPE_INT,&starting_tickets,sizeof(starting_tickets)),
  {}
};

const PHookEntry plugin_init(void) {
    PIUTools_Config_Read(plugin_config);
    init_ticket_state(starting_tickets);

    // Make Fake USB Device for Tickets
    PUSBDevice nd = PIUTools_USB_Add_Device(USB_2_SPEED,0,0x0d2f,0x1004,"TKT123",(void*)ticket_libusb,NULL,NULL);
    PIUTools_USB_Connect_Device(nd->dev);
    sprintf(generated_ticket_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);

    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/fake_ticket_device",fake_ticket_device_path);
    FILE* fp = fopen(fake_ticket_device_path,"wb");
    fwrite("TKT",3,1,fp);
    fclose(fp);
    chmod(fake_ticket_device_path, 0666);

    PIUTools_Filesystem_AddRedirect(generated_ticket_device_path,fake_ticket_device_path);

    DBG_printf("[%s] Created Fake Ticket Device At: %s",__FILE__,fake_ticket_device_path);
    return entries;
}