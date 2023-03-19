/*
    PIU input device emulator for Linux event devices
    Based on [REDACTED] by infamouspat

    Shoutouts to bxrx I guess :)
*/

/*
	UPDATE : MAY-JUNE 2011 - Exceed2-NX are now supported (ATR4X). Dance Mats are also supported.
*/

#define LEGACY // Uncomment if you're building for Exceed2-NX
// #define DEBUG //Uncomment if you want to see the scan codes in the console. Useful for mapping new Gamepads.

//#define DANCEMAT
#define KEYS

//TODO: Dynamically determine these node names and add USB Keyboard node in here somewhere.
#ifdef DANCEMAT
#define INPUT "/dev/input/by-path/pci-0000:00:13.1-usb-0:2:1.0-event-joystick" //FOR USB DANCE MAT SUPPORT
#else
#define INPUT "/run/kbdhook" //FOR PS/2 KEYBOARD SUPPORT
#endif

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


#define USB_ENDPOINT_IN	0x80

enum piu_control
{
    CTL_P17 = 0x00000001,
    CTL_P19 = 0x00000002,
    CTL_P15 = 0x00000004,
    CTL_P11 = 0x00000008,
    CTL_P13 = 0x00000010,
    CTL_TEST = 0x04000,
    CTL_SERVICE = 0x200,
	CTL_CLEAR = 0x8000,
	CTL_COIN1 = 0x00000400,
	CTL_COIN2 = 0x04000000,
    CTL_P21 = 0x00080000,
    CTL_P23 = 0x00100000,
    CTL_P25 = 0x00040000,
    CTL_P27 = 0x00010000,
    CTL_P29 = 0x00020000
};

struct input_binding
{
    uint16_t code;
    enum piu_control control;

};

struct usb_bus 
{
    struct usb_bus *next, *prev;

    char dirname[PATH_MAX + 1];

    struct usb_device *devices;
    u_int32_t location;

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

/* Adding configurability to this hack is left as an exercise for the reader. 
   (according to gergc that's practically my catchphrase or something) 

   Pump has built-in keyboard support for TEST and SERVICE so I'm just going
   to wire up the gameplay inputs here */

static const struct input_binding bindings[] =
{
    { KEY_Q,    CTL_P17 },
    { KEY_E,    CTL_P19 },
    { KEY_S,    CTL_P15 },
    { KEY_Z,    CTL_P11 },
    { KEY_C,    CTL_P13 },
    { KEY_R,    CTL_P27 },
    { KEY_Y,    CTL_P29 },
    { KEY_G,    CTL_P25 },
    { KEY_V,    CTL_P21 },
    { KEY_N,    CTL_P23 },
    { KEY_F1,  CTL_SERVICE },
    { KEY_F2,  CTL_TEST },
    { KEY_F3,  CTL_CLEAR },
    { KEY_F5,  CTL_COIN1 },
    { KEY_F6,  CTL_COIN2 },
    { 0, 0 }
};

static const char device_path[] 
    = INPUT;
static int fd = -1;
static uint32_t pad;
static pthread_t hthread;

static struct usb_bus bus;
static struct usb_device dev;
static struct usb_config_descriptor config;
static struct usb_interface interface;
static struct usb_interface_descriptor idesc;

struct usb_bus *usb_busses;

static void *input_thread(void *arg)
{
    struct input_event ev;
    int i;

    /* Not going to use any kind of locking because I'm a lazy arse */

    while (true) {
        if (read(fd, &ev, sizeof(ev)) != sizeof(ev)) {
            if (errno == EBADF) {
                /* Main thread closed our handle, we need to exit */
                return NULL;
            } else {
                /* Something else went wrong */
                perror("Input read");
                exit(EXIT_FAILURE);
            }
        }
#ifdef DEBUG
fprintf(stderr,"SCANCODE IS: %d\n",ev.code);
#endif


        if (ev.type == EV_KEY) {

            for (i = 0 ; bindings[i].code != 0 ; i++) {
                if (ev.code == bindings[i].code) {

                    if (ev.value) {

                        pad |= bindings[i].control;
                    } else {

                        pad &= ~bindings[i].control;
                    }

                    break;
                }
            }
        }
    }
}

int piuinput_usb_claim_interface(void *hdev, int interface) 
{
	return 0;
}

int piuinput_usb_close(void *hdev)
{
    /* My aesthetic sensibilities demand at least this much of me. */
    if (hthread != 0) {
        close(fd);

        pthread_join(hthread, NULL);
        hthread = 0;
        fd = -1;
    }
    
    return 0;
}

int piuinput_usb_control_msg(void *hdev, int requesttype, int request, int value, 
    int index, uint8_t *bytes, int nbytes, int timeout) 
{
    if (requesttype & USB_ENDPOINT_IN) {
        memset(bytes, 0xFF, nbytes);
        *((uint32_t *) bytes) = ~pad;
    }

    return nbytes;
}

int piuinput_usb_find_busses(void)
{
    memset(&bus, 0, sizeof(bus));
    usb_busses = &bus;

    return 1;
}

int piuinput_usb_find_devices(void)
{
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

int piuinput_usb_init(void)
{
    return 0;
}

void *piuinput_usb_open(struct usb_device *hdev)
{
	system("ln -s /dev/input/by-path/*event-kbd /run/kbdhook");
    if (hthread != 0){
		return NULL;
	} 

    fd = open(device_path, O_RDONLY);
    if (fd == -1) {
        perror("Opening input");
        //exit(EXIT_FAILURE);
    }

    pthread_create(&hthread, NULL, input_thread, NULL);

    /* Taking some liberties here but hey as long as it's non-NULL */
    return hdev;
}

int piuinput_usb_reset(void *hdev)
{
    return 0;
}

int piuinput_usb_set_altinterface(void *hdev, int interface)
{
    return 0;
}

int piuinput_usb_set_configuration(void *hdev, int configuration) 
{
	return 0;
}


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

