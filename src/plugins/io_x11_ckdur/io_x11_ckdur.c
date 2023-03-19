// A Super Simple MK6IO/PIUIO/ButtonBoard Keyboard X11 Backend for Testing
// X11 includes
#include <string.h>
#include <stdio.h>
#include <stdarg.h>


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>


#include <usb.h>

#include <plugin_sdk/dbg.h>
#include <plugin_sdk/ini.h>
#include <plugin_sdk/plugin.h>


/* THE FOLLOWING IS THE BUTTON-DESCRIPTION*/
int btn_dscr[] = {6, 7, 2, 4, 5,
                  -1, -1, -1, -1, -1
                  -1, -1, 0, 1};
uint32_t btn_mrk[] =
{0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010,
 0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000,
 0x04000000, 0x00000200, 0x00000400, 0x00004000};

 uint32_t lst_mrk[] =
{0x00000400, 0x00000000, 0x00000004, 0x00000000, 0x00000008,
 0x00000010, 0x00000001, 0x00000002, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000};  
 
 uint32_t lsta_mrk[] =
{0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
 0x00000000, 0x00000000, 0x00000000, 0x00000000};

 int size_mrk = sizeof(lst_mrk)/sizeof(uint32_t);

#define USB_DIR_OUT 0x00
#define USB_DIR_IN 0x80

#define USB_TYPE_STANDARD (0x00 << 5)
#define USB_TYPE_CLASS (0x01 << 5)
#define USB_TYPE_VENDOR (0x02 << 5)
#define USB_TYPE_RESERVED (0x03 << 5)

#define USB_RECIP_DEVICE 0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02
#define USB_RECIP_OTHER 0x03

const short PIUIO_VENDOR_ID	= 0x0547;
const short PIUIO_PRODUCT_ID = 0x1002;

const short PIUIOBUTTON_VENDOR_ID	= 0x0D2F;
const short PIUIOBUTTON_PRODUCT_ID = 0x1010;

/* proprietary (arbitrary?) request PIUIO requires to handle I/O */
const short PIUIO_CTL_REQ = 0xAE;

/* timeout value for read/writes, in microseconds (so, 10 ms) */
const int REQ_TIMEOUT = 10000;

// CKDur objects

/* USB BUS OBJECT*/
struct usb_bus *usb_busses;

struct usb_bus *bus;

int i = 0, k, l;
static int plugin_enabled = 0;
char bCouple = 0;
char bFastFire = 0;
char bStateFastFire = 0;
int bMaxStateFastFire = 450;
char bKeyCouple = 0;
char bKeyFastFire = 0;
char bKeyMaxFastFire1 = 0;
char bKeyMaxFastFire2 = 0;

int joy_fd, *axis=NULL, num_of_axis=0, num_of_buttons=0, x;
char *button=NULL, name_of_joystick[80];

int rc=0;


int done;
FILE* hFile = NULL;
int g_init = 0;
int nKeyboards = 0;

static usb_dev_handle *x_usb_open(struct usb_device *dev){
    void* p = (void*)dev;
    usb_dev_handle* pd = (usb_dev_handle*)p;
    return pd;
}
static int x_usb_reset(usb_dev_handle *dev){return 0;}
static int x_usb_set_altinterface(usb_dev_handle *dev, int alternate){return 0;}
static int x_usb_set_configuration(usb_dev_handle *dev, int configuration){return 0;}
static struct usb_bus *x_usb_get_busses(void){return usb_busses;}
static void x_usb_init(void){return;}
int x_usb_claim_interface(usb_dev_handle *dev, int interface){return 0;}
int x_usb_close(void *hdev){return 0;}
#define keyp(keymap, key) (keymap[key/8] & (1 << (key % 8)))
char bytes_p[4];
uint32_t mem = 0xFFFFFFFF;
struct timeval  tv;
double tick, last_tick;
char bytes_b[4] = {0xFF,0xFF,0xFF,0xFF};
char bytes_w[2] = {0xFF,0xFF};


int (*next_XNextEvent)(Display *display, XEvent *event_return);
#define SETMAPKEY(byte, place, value) {if(!press) byte[place] |= value; else byte[place] &= ~value;}

int x_XNextEvent(Display *display, XEvent *event_return){	
	// Load the original function
	int nRet = next_XNextEvent(display, event_return);
	XEvent ev; int ks; char press;
	memcpy(&ev, event_return, sizeof(XEvent));

	// Do event acording to us
	switch  (ev.type) {
    case KeyPress:
	case KeyRelease:
		press = ev.type==KeyPress?1:0;
		ks = XLookupKeysym(&ev.xkey, 0);

        if((ks == XK_KP_Home) || (ks == XK_r))
			SETMAPKEY(bytes_b, 2, 0x1);
        if((ks == XK_KP_Page_Up) || (ks == XK_y))
            SETMAPKEY(bytes_b, 2, 0x2);
        if((ks == XK_KP_Begin) || (ks == XK_g))
            SETMAPKEY(bytes_b, 2, 0x4);
        if((ks == XK_KP_End) || (ks == XK_v))
            SETMAPKEY(bytes_b, 2, 0x8);
        if((ks == XK_KP_Page_Down) || (ks == XK_n))
            SETMAPKEY(bytes_b, 2, 0x10);
        if((ks == XK_u))
            SETMAPKEY(bytes_b, 2, 0x20);
        if((ks == XK_i))
            SETMAPKEY(bytes_b, 2, 0x40);
        if((ks == XK_o))
            SETMAPKEY(bytes_b, 2, 0x80);

        if((ks == XK_q))
            SETMAPKEY(bytes_b, 0, 0x1);
        if((ks == XK_e))
            SETMAPKEY(bytes_b, 0, 0x2);
        if((ks == XK_s))
            SETMAPKEY(bytes_b, 0, 0x4);
        if((ks == XK_z))
            SETMAPKEY(bytes_b, 0, 0x8);
        if((ks == XK_c))
            SETMAPKEY(bytes_b, 0, 0x10);
        if((ks == XK_p))
            SETMAPKEY(bytes_b, 0, 0x20);
        if((ks == XK_j))
            SETMAPKEY(bytes_b, 0, 0x40);
        if((ks == XK_k))
            SETMAPKEY(bytes_b, 0, 0x80);

        if((ks == XK_l))
            SETMAPKEY(bytes_b, 3, 0x1);
        if((ks == XK_m))
            SETMAPKEY(bytes_b, 3, 0x2);
        if((ks == XK_6))
            SETMAPKEY(bytes_b, 3, 0x4);
        if((ks == XK_7))
            SETMAPKEY(bytes_b, 3, 0x8);
        if((ks == XK_8))
            SETMAPKEY(bytes_b, 3, 0x10);
        if((ks == XK_9))
            SETMAPKEY(bytes_b, 3, 0x20);
        if((ks == XK_0))
            SETMAPKEY(bytes_b, 3, 0x40);
        if((ks == XK_comma))
            SETMAPKEY(bytes_b, 3, 0x80);
        
		if((ks == XK_F5))
            SETMAPKEY(bytes_b, 1, 0x1);
        if((ks == XK_F6))
            SETMAPKEY(bytes_b, 1, 0x2);
        if((ks == XK_F7))
            SETMAPKEY(bytes_b, 1, 0x4);
        if((ks == XK_F8))
            SETMAPKEY(bytes_b, 1, 0x8);
        if((ks == XK_F9))
            SETMAPKEY(bytes_b, 1, 0x10);
        if((ks == XK_F10))
            SETMAPKEY(bytes_b, 1, 0x20);
        if((ks == XK_F11))
            SETMAPKEY(bytes_b, 1, 0x40);
        if((ks == XK_F12))
            SETMAPKEY(bytes_b, 1, 0x80);

        if((ks == XK_space))
        {
			SETMAPKEY(bytes_b, 2, 0x1F);
			SETMAPKEY(bytes_b, 0, 0x1F);
        }

		if((ks == XK_BackSpace))
            SETMAPKEY(bytes_w, 0, 0x1);
        if((ks == XK_Left))
            SETMAPKEY(bytes_w, 0, 0x2);
        if((ks == XK_Right))
            SETMAPKEY(bytes_w, 0, 0x4);
        if((ks == XK_Return))
            SETMAPKEY(bytes_w, 0, 0x8);
        if((ks == XK_KP_Subtract))
            SETMAPKEY(bytes_w, 0, 0x10);
        if((ks == XK_KP_Left))
            SETMAPKEY(bytes_w, 0, 0x20);
        if((ks == XK_KP_Right))
            SETMAPKEY(bytes_w, 0, 0x40);
        if((ks == XK_KP_Enter))
            SETMAPKEY(bytes_w, 0, 0x80);
    	break;
    }
	
	return nRet;
}

struct usb_device* pec;
int x_usb_control_msg(usb_dev_handle *dev, int requesttype, int request,int value, int index, char *bytes, int size, int timeout){

	pec = (struct usb_device*)dev;
    if(pec->descriptor.idVendor == PIUIO_VENDOR_ID && pec->descriptor.idProduct == PIUIO_PRODUCT_ID &&
      (requesttype & USB_DIR_IN) && (requesttype & USB_TYPE_VENDOR) && (request & PIUIO_CTL_REQ)){
        memcpy(bytes_p, bytes_b, 4);
        bytes[0] = (mem & 0xFF) & bytes_p[0];
        bytes[1] = ((mem >> 8) & 0xFF) & bytes_p[1];
        bytes[2] = ((mem >> 16) & 0xFF) & bytes_p[2];
        bytes[3] = ((mem >> 24) & 0xFF) & bytes_p[3];
        return 8;   // ITG says that
    }
    else if(pec->descriptor.idVendor == PIUIO_VENDOR_ID && pec->descriptor.idProduct == PIUIO_PRODUCT_ID &&
           (requesttype & USB_DIR_OUT) && (requesttype & USB_TYPE_VENDOR) && (request & PIUIO_CTL_REQ)){ 
        return 8;   // ITG says that
    }else if(pec->descriptor.idVendor == PIUIOBUTTON_VENDOR_ID && pec->descriptor.idProduct == PIUIOBUTTON_PRODUCT_ID){
		memcpy(bytes, bytes_w, 2);
        return 0;
    }
    return 8;   // ITG says that
}

int g_found_busses = 0;
int x_usb_find_busses(void){
    if(!g_found_busses) bus = (struct usb_bus*)malloc(sizeof(struct usb_bus));
    memset(bus, 0, sizeof(struct usb_bus));
    bus->next=NULL;
    bus->prev=NULL;
    usb_busses = bus;
    g_found_busses = 1;
    return 1;
}


int g_usb_device1=0;
int g_usb_device2=0;
struct usb_device* g_dev = NULL;
struct usb_device* g_dev2 = NULL; // For button
int x_usb_find_devices(void){    
    if(!g_usb_device1)
    {
        g_dev = (struct usb_device*)malloc(sizeof(struct usb_device));
        memset(g_dev, 0, sizeof(struct usb_device));
        g_dev->bus = bus;    // Same bus... LOL
        // A new configfor PIUIO
        g_dev->config = (struct usb_config_descriptor*)malloc(sizeof(struct usb_config_descriptor));
        memset(g_dev->config, 0, sizeof(struct usb_config_descriptor));
        g_dev->config->interface = (struct usb_interface*)malloc(sizeof(struct usb_interface));
        memset(g_dev->config->interface, 0, sizeof(struct usb_interface));
        g_dev->config->interface->altsetting = (struct usb_interface_descriptor*)malloc(sizeof(struct usb_interface_descriptor));
        memset(g_dev->config->interface->altsetting, 0, sizeof(struct usb_interface_descriptor));
        g_dev->config->interface->altsetting->endpoint = (struct usb_endpoint_descriptor*)malloc(sizeof(struct usb_endpoint_descriptor));
        memset(g_dev->config->interface->altsetting->endpoint, 0, sizeof(struct usb_endpoint_descriptor));
        // Set the bitches for PIUIO
        g_dev->descriptor.idVendor = PIUIO_VENDOR_ID;
        g_dev->descriptor.idProduct = PIUIO_PRODUCT_ID;
        g_usb_device1=1;
    }
    if(!g_usb_device2)
    {
        g_dev2 = (struct usb_device*)malloc(sizeof(struct usb_device));
        memset(g_dev2, 0, sizeof(struct usb_device));
        g_dev2->bus = bus;    // Same bus... LOL
        // A new config for PIUIOBUTTON
        g_dev2->config = (struct usb_config_descriptor*)malloc(sizeof(struct usb_config_descriptor));
        memset(g_dev2->config, 0, sizeof(struct usb_config_descriptor));
        g_dev2->config->interface = (struct usb_interface*)malloc(sizeof(struct usb_interface));
        memset(g_dev2->config->interface, 0, sizeof(struct usb_interface));
        g_dev2->config->interface->altsetting = (struct usb_interface_descriptor*)malloc(sizeof(struct usb_interface_descriptor));
        memset(g_dev2->config->interface->altsetting, 0, sizeof(struct usb_interface_descriptor));
        g_dev2->config->interface->altsetting->endpoint = (struct usb_endpoint_descriptor*)malloc(sizeof(struct usb_endpoint_descriptor));
        memset(g_dev2->config->interface->altsetting->endpoint, 0, sizeof(struct usb_endpoint_descriptor));
        // Set the bitches for PIUIOBUTTON
        g_dev2->descriptor.idVendor = PIUIOBUTTON_VENDOR_ID;
        g_dev2->descriptor.idProduct = PIUIOBUTTON_PRODUCT_ID;
        g_usb_device2=1;
    }
    //Next connected to PIUIO is PIUIOBUTTON
    //This is ONLY if PIUIOBUTTON exists
    //if(bIsButton)
    {
        g_dev->next = g_dev2;
        g_dev2->prev = g_dev;
    }
    /*else
    {
        g_dev->next = NULL;
    }*/

    // Finally, ret
    bus->devices = g_dev;
    bus->root_dev = g_dev;
	return 1;
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_claim_interface", x_usb_claim_interface, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_close", x_usb_close, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_control_msg", x_usb_control_msg, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_busses", x_usb_find_busses, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_find_devices", x_usb_find_devices, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_init", x_usb_init, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_open", x_usb_open, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_reset", x_usb_reset, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_altinterface", x_usb_set_altinterface, NULL, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libusb-0.1.so.4","usb_set_configuration", x_usb_set_configuration, NULL, 1),                    
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6","XNextEvent", x_XNextEvent, &next_XNextEvent, 1),       
    {}
};

const PHookEntry plugin_init(const char* config_path){    
    return entries;
}

