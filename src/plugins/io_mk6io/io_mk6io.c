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

#include "apug0pcb007.h"

// TODO: Deprecate this with actual virtual USB Device Handling Like a Fucking Adult
#define USB_ENDPOINT_IN	0x80

struct usb_bus 
{
    struct usb_bus *next, *prev;

    char dirname[PATH_MAX + 1];

    struct usb_device *devices;
    uint32_t location;

    struct usb_device *root_dev;
};

struct usb_config_descriptor 
{
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t wTotalLength;
	uint8_t  bNumInterfaces;
	uint8_t  bConfigurationValue;
	uint8_t  iConfiguration;
	uint8_t  bmAttributes;
	uint8_t  MaxPower;

	struct usb_interface *interface;

	unsigned char *extra;
	int extralen;
};

struct usb_device_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
};

struct usb_device 
{
    struct usb_device *next;
    struct usb_device *prev;

    char filename[PATH_MAX + 1];

    struct usb_bus *bus;
    
    struct usb_device_descriptor descriptor;
    struct usb_config_descriptor *config;

    void *dev;

    uint8_t devnum;

    unsigned char num_children;
    struct usb_device **children;
};

struct usb_interface 
{
	struct usb_interface_descriptor *altsetting;
	int num_altsetting;
};

struct usb_interface_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bInterfaceNumber;
	uint8_t  bAlternateSetting;
	uint8_t  bNumEndpoints;
	uint8_t  bInterfaceClass;
	uint8_t  bInterfaceSubClass;
	uint8_t  bInterfaceProtocol;
	uint8_t  iInterface;

	struct usb_endpoint_descriptor *endpoint;

	unsigned char *extra;
	int extralen;
};
static struct usb_bus bus;
static struct usb_device dev;
static struct usb_config_descriptor config;
static struct usb_interface interface;
static struct usb_interface_descriptor idesc;

struct usb_bus *usb_busses;

int piuinput_usb_claim_interface(void *hdev, int interface) {return 0;}
int piuinput_usb_close(void *hdev){return 0;}

int piuinput_usb_control_msg(void *hdev, int requesttype, int request, int value, int index, uint8_t *bytes, int nbytes, int timeout) {
    if (requesttype & USB_ENDPOINT_IN) {
        memset(bytes, 0xFF, nbytes);        
        *((uint32_t *) bytes) = mk6io_read_input();
        
    }
    return nbytes;
}
int piuinput_usb_find_busses(void){
    memset(&bus, 0, sizeof(bus));
    usb_busses = &bus;
    return 1;
}

int piuinput_usb_find_devices(void){
    /* PIU accesses device->config->bConfigurationValue and
       device->config->interface->altsetting->bInterfaceNumber 

       As long as we can perform these dereferences we don't need to bother
       with anything else. Well, except for the vendor and product ID.
       Those are kind of important :) */

    usb_busses = &bus;
    bus.devices = &dev;
    dev.config = &config;
    config.interface = &interface;
    interface.altsetting = &idesc;

    dev.descriptor.idVendor = 0x0547;
    dev.descriptor.idProduct = 0x1002;

    return 1;
}

int piuinput_usb_init(void){return 0;}
void* piuinput_usb_open(struct usb_device *hdev){return hdev;}
int piuinput_usb_reset(void *hdev){return 0;}
int piuinput_usb_set_altinterface(void *hdev, int interface){return 0;}
int piuinput_usb_set_configuration(void *hdev, int configuration) {return 0;}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_claim_interface", piuinput_usb_claim_interface, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_close", piuinput_usb_close, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_control_msg", piuinput_usb_control_msg, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_busses", piuinput_usb_find_busses, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_devices", piuinput_usb_find_devices, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_init", piuinput_usb_init, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_open", piuinput_usb_open, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_reset", piuinput_usb_reset, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_altinterface", piuinput_usb_set_altinterface, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_configuration", piuinput_usb_set_configuration, NULL, 1),                    
    {}
};

const PHookEntry plugin_init(const char* config_path){    
    return entries;
}
