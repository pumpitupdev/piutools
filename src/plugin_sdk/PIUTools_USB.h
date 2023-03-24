#pragma once

enum _USB_SPEED{
    USB_1_LOW_SPEED,
    USB_1_FULL_SPEED,
    USB_2_SPEED,
    USB_3_SPEED,
    USB_3_1_SPEED
};

#ifndef USB_CLASS_MASS_STORAGE
#define USB_CLASS_MASS_STORAGE 8
#endif

#define MAX_USB_DEVICES 16
#define ID_FAKE_USB_DEVICE 0x9F2D3
#define FAKE_USB_BUS 99
#define FAKE_USB_BUS_STR "099"

typedef struct _USB_DEVICE_INFO{
    unsigned char enabled;
    unsigned char spd_enum;
    unsigned char bus;
    unsigned char lev;
    unsigned char prnt;
    unsigned char port;
    unsigned char cnt;
    unsigned char dev;
    unsigned char mxch;
    unsigned short vid;
    unsigned short pid;
    unsigned char cls;
    char spd[8];
    char rev[16];
    char serial[16];
    void* ctrl_msg_handler;
    void* bulk_read_handler;
    void* bulk_write_handler;
}USBDevice,*PUSBDevice;

typedef int (*transaction_handler)(void *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);

extern USBDevice PIUTools_USB_Devices[MAX_USB_DEVICES];
void PIUTools_USB_Init(const char* root_path);
PUSBDevice PIUTools_USB_Add_Device(unsigned char usb_speed, unsigned char cls, unsigned short vid, unsigned short pid, char* serial, void* ctrl_msg_handler,void* bulk_read_handler,void* bulk_write_handler);
void PIUTools_USB_Connect_Device(unsigned char dev);
void PIUTools_USB_Disconnect_Device(unsigned char dev);