// USB Abstraction Layer for PIUTools
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <PIUTools_SDK.h>




char fake_devices_list_path[1024] = {0x00};

static int module_initialized = 0;
int current_device_count = 0;
USBDevice PIUTools_USB_Devices[MAX_USB_DEVICES];

static void piutools_usb_init(void){
    if(module_initialized){return;}
    module_initialized = 1;
    char usb_dev_path[1024] = {0x00};
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/usb",usb_dev_path);
    PIUTools_Filesystem_Create_Directory(usb_dev_path);
    PIUTools_Path_Resolve("${TMP_ROOT_PATH}/usb/fake_usb_devices",fake_devices_list_path);

    // We'll add this programmatically to our redirect.
    // For some systems, we might not have usbfs, so we don't have 
    // the necessary directories to do our thing.
    if(PIUTools_Filesystem_Path_Exist("/proc/bus/usb") == 0){
        PIUTools_Filesystem_AddRedirect("{FULL}/proc/bus/usb",usb_dev_path);
    }
    if(PIUTools_Filesystem_Path_Exist("/dev/bus/usb") == 0){
        char usb_fakedev_path[1024] = {0x00};
        sprintf(usb_fakedev_path,"%s/001",usb_dev_path);
        PIUTools_Filesystem_Create_Directory(usb_fakedev_path);
        PIUTools_Filesystem_AddRedirect("/dev/bus/usb",usb_dev_path);
    }
    PIUTools_Filesystem_AddRedirect("/proc/bus/usb/devices",fake_devices_list_path);
    PIUTools_Filesystem_AddRedirect("/sys/kernel/debug/usb/devices",fake_devices_list_path);
    printf("[%s] Fake USB Driver Initialized: %s\n",__FILE__,fake_devices_list_path);
    for(int i=0;i<MAX_USB_DEVICES;i++){
        PIUTools_USB_Devices[i].enabled = 0;
    }
}

static void update_fake_devices(void){
    FILE* fp = fopen(fake_devices_list_path,"wb");
    if(fp == NULL){
        printf("[%s:%s] Error - Unable to Open USB Devices List Path: %s - %s\n",__FILE__,__FUNCTION__,fake_devices_list_path, strerror(errno));
        exit(-1);
    }
    for(int i=0;i<MAX_USB_DEVICES;i++){
        char t_line[256];
        char p_line[256];
        char s_line[256];
        if(PIUTools_USB_Devices[i].enabled == 0){continue;}
        sprintf(t_line, "T:  Bus=%2d Lev=%2d Prnt=%2d Port=%2d Cnt=%2d Dev#=%3d Spd=%s  MxCh= 6\n", PIUTools_USB_Devices[i].bus,PIUTools_USB_Devices[i].lev,PIUTools_USB_Devices[i].prnt, PIUTools_USB_Devices[i].port,PIUTools_USB_Devices[i].cnt, PIUTools_USB_Devices[i].dev, PIUTools_USB_Devices[i].spd);
        fwrite(t_line,strlen(t_line),1,fp);
        sprintf(p_line, "P:  Vendor=%04x ProdID=%04x\n", PIUTools_USB_Devices[i].vid, PIUTools_USB_Devices[i].pid);
        fwrite(p_line,strlen(p_line),1,fp);
        sprintf(s_line, "S:  SerialNumber=%s\n", PIUTools_USB_Devices[i].serial);
        fwrite(s_line,strlen(s_line),1,fp);
        if(PIUTools_USB_Devices[i].cls == USB_CLASS_MASS_STORAGE){
            char i_line[256] = {0x00};
            sprintf(i_line,"I:  If#=%2d Alt=%2d #EPs=%2d Cls=%02x\n",0,0,2,USB_CLASS_MASS_STORAGE);
            fwrite(i_line,strlen(i_line),1,fp);
            const char* e_line = "E:  Ad=82(I)\n";
            fwrite(e_line,strlen(e_line),1,fp);            
        }
        fwrite("\n",1,1,fp);
    }

    fclose(fp);
    chmod(fake_devices_list_path, 0666);
}

PUSBDevice PIUTools_USB_Add_Device(unsigned char usb_speed, unsigned char cls, unsigned short vid, unsigned short pid, char* serial, void* ctrl_msg_handler,void* bulk_read_handler,void* bulk_write_handler){
    if(!module_initialized){piutools_usb_init();}
    // Find the first free device
    int offset = current_device_count;
    current_device_count++;
    PIUTools_USB_Devices[offset].enabled = 0;
    PIUTools_USB_Devices[offset].bus = FAKE_USB_BUS;
    PIUTools_USB_Devices[offset].lev = 0;
    PIUTools_USB_Devices[offset].prnt = 0;
    PIUTools_USB_Devices[offset].port = offset+1;
    PIUTools_USB_Devices[offset].cnt = 0;
    PIUTools_USB_Devices[offset].dev = offset+1;
    PIUTools_USB_Devices[offset].mxch = 0;
    PIUTools_USB_Devices[offset].spd_enum = usb_speed;
    PIUTools_USB_Devices[offset].cls = cls;
    const char* spd_s;
    switch(usb_speed){
        case USB_1_LOW_SPEED:
            spd_s = "1.5";
            break;
        case USB_1_FULL_SPEED:
            spd_s = "12";
            break;        
        case USB_2_SPEED:
            spd_s = "480";
            break;  
        case USB_3_SPEED:
            spd_s = "5000";
            break;     
        case USB_3_1_SPEED:
            spd_s = "10000";
            break; 
        default:
            spd_s = "480";
            break;                        
    }
    strcpy(PIUTools_USB_Devices[offset].spd,spd_s);
    PIUTools_USB_Devices[offset].vid = vid;
    PIUTools_USB_Devices[offset].pid = pid;
    strcpy(PIUTools_USB_Devices[offset].serial,serial);
    PIUTools_USB_Devices[offset].ctrl_msg_handler = ctrl_msg_handler;
    PIUTools_USB_Devices[offset].bulk_read_handler = bulk_read_handler;
    PIUTools_USB_Devices[offset].bulk_write_handler = bulk_write_handler;    
    return &PIUTools_USB_Devices[offset];
}

void PIUTools_USB_Connect_Device(unsigned char dev){
    if(!module_initialized){piutools_usb_init();}
    PIUTools_USB_Devices[dev - 1].enabled = 1;
    update_fake_devices();
}

void PIUTools_USB_Disconnect_Device(unsigned char dev){
    if(!module_initialized){piutools_usb_init();}
    PIUTools_USB_Devices[dev - 1].enabled = 0;
    update_fake_devices();
}

