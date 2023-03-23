// LibUSB Emulation based on PIUTools_USB
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_USB.h>

#include "usb_0.h"



#define FAKE_DT 0xFF

#pragma pack(1)
struct usb_dev_fake_handle {
  int fd;

  struct usb_bus *bus;
  struct usb_device *device;

  int config;
  int interface;
  int altsetting;

  /* Added by RMT so implementations can store other per-open-device data */
  void *impl_info;
  PUSBDevice fake_device;
};
typedef struct usb_dev_fake_handle usb_dev_fakehandle;
// --- libusb0.1 Hooks ---

typedef int (*usb_close_t)(struct usb_dev_handle *);
usb_close_t next_usb_close;
int fake_usb_close(struct usb_dev_handle * hdev){
    printf("Entering [%s]\n",__FUNCTION__);
    if(hdev->fd == ID_FAKE_USB_DEVICE){
        free(hdev); 
        return 0;
    }
    return next_usb_close(hdev);
}

typedef int (*usb_bulk_write_t)(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
usb_bulk_write_t next_usb_bulk_write;
int fake_usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout){
    printf("Entering [%s]\n",__FUNCTION__);    
    if(dev->fd == ID_FAKE_USB_DEVICE){
        struct usb_dev_fake_handle* fdev = (struct usb_dev_fake_handle*)dev;
        usb_bulk_write_t nfunc = (usb_bulk_write_t)fdev->fake_device->bulk_write_handler;
        return nfunc(dev,ep,bytes,size,timeout);
    }
    return next_usb_bulk_write(dev,ep,bytes,size,timeout);
}

typedef int (*usb_bulk_read_t)(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
usb_bulk_read_t next_usb_bulk_read;
int fake_usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout){
    printf("Entering [%s]\n",__FUNCTION__);    
    if(dev->fd == ID_FAKE_USB_DEVICE){
        struct usb_dev_fake_handle* fdev = (struct usb_dev_fake_handle*)dev;
        usb_bulk_read_t nfunc = (usb_bulk_read_t)fdev->fake_device->bulk_read_handler;
        return nfunc(dev,ep,bytes,size,timeout);
    }   
    return next_usb_bulk_read(dev,ep,bytes,size,timeout);
}


typedef int (*usb_control_msg_t)(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);
static usb_control_msg_t next_usb_control_msg;
int fake_usb_control_msg(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout){
    //printf("Entering [%s]\n",__FUNCTION__);
    if(dev->fd == ID_FAKE_USB_DEVICE){
        struct usb_dev_fake_handle* fdev = (struct usb_dev_fake_handle*)dev;
        usb_control_msg_t nfunc = (usb_control_msg_t)fdev->fake_device->ctrl_msg_handler;
        return nfunc(dev,requesttype,request,value,index,bytes,size,timeout);
    }
    return next_usb_control_msg(dev,requesttype,request,value,index,bytes,size,timeout);
}

typedef int (*usb_set_configuration_t)(usb_dev_handle *dev, int configuration);
usb_set_configuration_t next_usb_set_configuration;
int fake_usb_set_configuration(struct usb_dev_handle * dev, int config){
    printf("Entering [%s]\n",__FUNCTION__);
    if(dev->fd == ID_FAKE_USB_DEVICE){
        dev->config = config;
        return 0;
    }
    return next_usb_set_configuration(dev,config);
}

typedef int (*usb_claim_interface_t)(usb_dev_handle *dev, int interface);
usb_claim_interface_t next_usb_claim_interface;
int fake_usb_claim_interface(struct usb_dev_handle * dev,int interface){    
    printf("Entering [%s]\n",__FUNCTION__);
    if(dev->fd == ID_FAKE_USB_DEVICE){
        dev->interface = interface;
        return 0;
    }
    return next_usb_claim_interface(dev,interface);
}

typedef int (*usb_release_interface_t)(usb_dev_handle *dev, int interface);
usb_release_interface_t next_usb_release_interface;
int fake_usb_release_interface(struct usb_dev_handle * dev,int interface){
    printf("Entering [%s]\n",__FUNCTION__);
    if(dev->fd == ID_FAKE_USB_DEVICE){
        dev->interface = -1;
        return 0;
    }
    return next_usb_release_interface(dev,interface);
}

typedef int (*usb_set_altinterface_t)(usb_dev_handle *dev, int alternate);
usb_set_altinterface_t next_usb_set_altinterface;
int fake_usb_set_altinterface(struct usb_dev_handle * dev,int alternate){
    printf("Entering [%s]\n",__FUNCTION__);
    if(dev->fd == ID_FAKE_USB_DEVICE){
         dev->altsetting = alternate;
        return 0;
    }
    return next_usb_set_altinterface(dev,alternate);
}

typedef int (*usb_resetep_t)(usb_dev_handle *dev, unsigned int ep);
usb_resetep_t next_usb_resetep;
int fake_usb_resetep (usb_dev_handle *dev, unsigned int ep){
    if(dev->fd == ID_FAKE_USB_DEVICE){         
        return 0;
    }
    return next_usb_resetep(dev,ep);
}

typedef int (*usb_clear_halt_t)(usb_dev_handle *dev, unsigned int ep);
usb_clear_halt_t next_usb_clear_halt;
int fake_usb_clear_halt(usb_dev_handle *dev, unsigned int ep){
    if(dev->fd == ID_FAKE_USB_DEVICE){         
        return 0;
    }
    return next_usb_clear_halt(dev,ep);
}

typedef int (*usb_reset_t)(usb_dev_handle *dev);
usb_reset_t next_usb_reset;
int fake_usb_reset (usb_dev_handle *dev){
    if(dev->fd == ID_FAKE_USB_DEVICE){         
        return 0;
    }
    return next_usb_reset(dev);
}

typedef int (*usb_init_t)(void);
usb_init_t next_usb_init;
static int libusb_initialized = 0;
int fake_usb_init(void){
    if(libusb_initialized){return 0;}
    return next_usb_init();
}


typedef usb_dev_handle * (*usb_open_ptr)(struct usb_device*);
usb_open_ptr next_usb_open;

usb_dev_handle * fake_usb_open(struct usb_device* dev){    
    printf("Entering [%s]\n",__FUNCTION__); 
    if(dev->descriptor.bDescriptorType == FAKE_DT){
        usb_dev_fakehandle *udev;
        udev = malloc(sizeof(*udev));
        if (!udev){return NULL;}
        udev->fd = ID_FAKE_USB_DEVICE;
        udev->device = dev;
        udev->bus = dev->bus;
        udev->config = udev->interface = udev->altsetting = -1;               
        udev->fake_device = NULL;        
        // Get the right fake device.
        for(int i=0;i<MAX_USB_DEVICES;i++){
            if(PIUTools_USB_Devices[i].enabled == 0){continue;}
            if(PIUTools_USB_Devices[i].vid == dev->descriptor.idVendor && PIUTools_USB_Devices[i].pid == dev->descriptor.idProduct){
                udev->fake_device = &PIUTools_USB_Devices[i];
            }
        }
        if(udev->fake_device == NULL){
            printf("Warning - Unable to Find Fake Device VID/PID: %04X %04x\n",dev->descriptor.idVendor, dev->descriptor.idProduct);
            return next_usb_open(dev);
        }
        return (usb_dev_handle*)udev;
    }
    return next_usb_open(dev);
}



typedef int (*usb_find_busses_t)(void);
usb_find_busses_t next_usb_find_busses;
int fake_usb_find_busses(void){
    // First - Get Real Busses
    int num_real_busses = next_usb_find_busses();

    // Next - Build our own Fake Bus
    struct usb_bus *bus = malloc(sizeof(*bus));
    memset((void *)bus, 0, sizeof(*bus));
    // Set Fake Name
    strncpy(bus->dirname, FAKE_USB_BUS_STR, sizeof(bus->dirname) - 1);
    bus->dirname[sizeof(bus->dirname) - 1] = 0;

    if (usb_busses) {
      bus->next = usb_busses;
      bus->next->prev = bus;
    }
    usb_busses = bus;

    return num_real_busses + 1;
}


typedef int (*usb_find_devices_t)(void);
usb_find_devices_t next_usb_find_devices;
void populate_fake_usb_device_descriptor(unsigned short vid, unsigned short pid, char usb_rev, struct usb_device* fake_device){
    printf("Entering [%s]\n",__FUNCTION__);
    fake_device->descriptor.bLength = sizeof(struct usb_device_descriptor);
    fake_device->descriptor.bDescriptorType = FAKE_DT;
    int spdval = 0;
    switch(usb_rev){
        case USB_1_LOW_SPEED:
            spdval = 0x100;
            break;
        case USB_1_FULL_SPEED:
            spdval = 0x110;
            break;        
        case USB_2_SPEED:
            spdval = 0x200;
            break;               
        case USB_3_SPEED:
            spdval = 0x300;
            break;            
        case USB_3_1_SPEED:
            spdval = 0x310;
            break;                                 
        default:
            spdval = 0x200;
            break;
    }
    fake_device->descriptor.bcdUSB = spdval;
    fake_device->descriptor.idVendor = vid;
    fake_device->descriptor.idProduct = pid;
    fake_device->descriptor.bcdDevice = 0x0001;
    fake_device->descriptor.iManufacturer = 0x01;
    fake_device->descriptor.iProduct = 0x02;
    fake_device->descriptor.iSerialNumber = 0x03;
    fake_device->descriptor.bNumConfigurations = 0x01;
    fake_device->config = malloc(sizeof(struct usb_config_descriptor));
    fake_device->config->bNumInterfaces = 1;
    fake_device->config->bLength = sizeof(struct usb_config_descriptor);
    fake_device->config->wTotalLength = sizeof(struct usb_config_descriptor) + sizeof(struct usb_interface_descriptor) + sizeof(struct usb_endpoint_descriptor);
    fake_device->config->interface = malloc(sizeof(struct usb_interface));
    fake_device->config->interface->altsetting = malloc(sizeof(struct usb_interface_descriptor));
    fake_device->config->interface->altsetting->bLength = sizeof(struct usb_interface_descriptor);
    fake_device->config->interface->altsetting->bInterfaceNumber = 0;
    fake_device->config->interface->altsetting->bAlternateSetting = 0;
    fake_device->config->interface->altsetting->bNumEndpoints = 1;
    fake_device->config->interface->altsetting->bInterfaceClass = 0xff;
    fake_device->config->interface->altsetting->bInterfaceSubClass = 0xff;
    fake_device->config->interface->altsetting->bInterfaceProtocol = 0xff;
    fake_device->config->interface->altsetting->iInterface = 0;
    fake_device->config->interface->altsetting->endpoint = malloc(sizeof(struct usb_endpoint_descriptor));
    fake_device->config->interface->altsetting->endpoint->bLength = sizeof(struct usb_endpoint_descriptor);
    fake_device->config->interface->altsetting->endpoint->bDescriptorType = 0x05;
    fake_device->config->interface->altsetting->endpoint->bEndpointAddress = 0x81;
    fake_device->config->interface->altsetting->endpoint->bmAttributes = 0;

}

// dev->config->interface->altsetting->bInterfaceNumber

int fake_usb_find_devices(void){
    printf("Entering [%s]\n",__FUNCTION__);
    // First - Call the real find devices.
    int num_real_devices = next_usb_find_devices();

    // Next - Look for our fake bus, return as normal if we don't find it.
    struct usb_bus *fake_bus;
    for (fake_bus = usb_busses; fake_bus != NULL; fake_bus = fake_bus->next) {
        if(strcmp(fake_bus->dirname,FAKE_USB_BUS_STR) == 0){break;}
    }
    if(fake_bus == NULL){
        printf("[%s] Error: Couldn't Find Fake BUS\n");
        return num_real_devices;
    }

    int num_fake_devices = 0;
    for(int i = 0; i < MAX_USB_DEVICES; i++){
        if(PIUTools_USB_Devices[i].enabled == 0){continue;}
        num_fake_devices++;
        // Allocate memory for your fake device
        struct usb_device *fake_device = calloc(1, sizeof(struct usb_device));
        fake_device->bus = fake_bus;
        sprintf(fake_device->filename,"/proc/bus/usb/%03d/%03d",PIUTools_USB_Devices[i].bus,PIUTools_USB_Devices[i].dev);        
        populate_fake_usb_device_descriptor(PIUTools_USB_Devices[i].vid,PIUTools_USB_Devices[i].pid,PIUTools_USB_Devices[i].spd_enum,fake_device);

        // Set our fake device to the first device in the list
        if (fake_bus->devices) {
            fake_device->next = fake_bus->devices;
            fake_device->next->prev = fake_device;
        }
        fake_bus->devices = fake_device;        
    }
    if(num_fake_devices > 0 && num_real_devices < 0){
        num_real_devices = 0;
    }
    printf("USB Find Devices Result: %d Real Devices, %d Fake Devices\n",num_real_devices,num_fake_devices);
    return num_real_devices+num_fake_devices;
}




static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_close", fake_usb_close, &next_usb_close, 1),      
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_bulk_write", fake_usb_bulk_write, &next_usb_bulk_write, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_bulk_read", fake_usb_bulk_read, &next_usb_bulk_read, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_control_msg", fake_usb_control_msg, &next_usb_control_msg, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_configuration", fake_usb_set_configuration, &next_usb_set_configuration, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_claim_interface", fake_usb_claim_interface, &next_usb_claim_interface, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_release_interface", fake_usb_release_interface, &next_usb_release_interface, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_altinterface", fake_usb_set_altinterface, &next_usb_set_altinterface, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_resetep", fake_usb_resetep, &next_usb_resetep, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_clear_halt", fake_usb_clear_halt, &next_usb_clear_halt, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_reset", fake_usb_reset, &next_usb_reset, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_init", fake_usb_init, &next_usb_init, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_open", fake_usb_open, &next_usb_open, 1),    
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_busses", fake_usb_find_busses, &next_usb_find_busses, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_devices", fake_usb_find_devices, &next_usb_find_devices, 1),
    {}    
};

const PHookEntry plugin_init(const char* config_path){
    printf("[%s] Starting Fake libusb0...\n", __FILE__);
  return entries;
}