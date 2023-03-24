// Compatibility Framework for Andamiro MK6 IO
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>


#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_USB.h>
#include "apug0pcb007.h"



#define USB_READ 0xC0
#define USB_WRITE 0x40
static char fake_mk6io_device_path[1024];

int piuinput_usb_control_msg(void *hdev, int requesttype, int request, int value, int index, uint8_t *bytes, int nbytes, int timeout) {
    switch(requesttype){
        case USB_READ:
            *((uint32_t *) bytes) = mk6io_read_input();
            break;
        case USB_WRITE:
            mk6io_write_output(bytes);
        default:
            break;
    }   
    return nbytes;
}


const PHookEntry plugin_init(const char* config_path){   
    PUSBDevice nd = PIUTools_USB_Add_Device(USB_1_FULL_SPEED,0,PIUIO_VID,PIUIO_PID,"MK6IO",(void*)piuinput_usb_control_msg,NULL,NULL);
    PIUTools_USB_Connect_Device(nd->dev);
    sprintf(fake_mk6io_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
    printf("[%s] Created Fake MK6IO Device At: %s\n",__FILE__,fake_mk6io_device_path);
    return NULL;
}
