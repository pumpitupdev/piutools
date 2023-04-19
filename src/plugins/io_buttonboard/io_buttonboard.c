// Compatibility Framework for Andamiro ButtonBoard IO
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include <PIUTools_SDK.h>

#include "am_buttonboard.h"



#define USB_READ 0xC0
#define USB_WRITE 0x40
static char fake_device_path[1024];

int piuinputbb_usb_control_msg(void *hdev, int requesttype, int request, int value, int index, uint8_t *bytes, int nbytes, int timeout) {
    switch(requesttype){
        case USB_READ:
            piubtn_read_input(bytes);
            break;
        case USB_WRITE:
            piubtn_write_output(bytes);
        default:
            break;
    }   
    return nbytes;
}

const PHookEntry plugin_init(void){   
    PUSBDevice nd = PIUTools_USB_Add_Device(USB_1_FULL_SPEED,0,PIUBTN_DRV_VID,PIUBTN_DRV_PID,"PIUBTN",(void*)piuinputbb_usb_control_msg,NULL,NULL);
    PIUTools_USB_Connect_Device(nd->dev);
    sprintf(fake_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
    printf("[%s] Created Fake ButtonBoard Device At: %s\n",__FILE__,fake_device_path);
    return NULL;
}
