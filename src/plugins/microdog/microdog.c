// Plugin for RainbowChina/SafeNET MicroDOG API Version 3.4
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "microdog/microdog.h"

#include <PIUTools_SDK.h>

static char dongle_file_path[1024] = {0x00};
static char fake_dongle_dev_path[1024] = {0x00};
static int protocol_version = 0;
int md40_last_sock_fd = 0;
unsigned char md40_last_packet[MD40_PACKET_SIZE] = {0x00};

typedef int (*ioctl_func_t)(int fd, int request, void* data);
static ioctl_func_t next_ioctl;


static int md34_ioctl(int fd, int request, void* data) {

    if(request == MD33_IOCTL || request == MD34_IOCTL){
        // This is somewhat of an issue with the MicroDog 3.x Stuff
        // The parameter is an unsigned long* which has to be dereferenced.        
        MicroDog_HandlePacket((unsigned char*)*(unsigned long*)data);
        return 0;
    }
    return next_ioctl(fd, request, data);
}

typedef int (*select_func_t)(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
select_func_t next_select;
int md40_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
    // Bit on the hacky side, but we're waiting until 'select' to make the emulator do its thing.
    if(nfds-1 == md40_last_sock_fd){
        MicroDog_HandlePacket(md40_last_packet);
        return 1;
    }
    return next_select(nfds, readfds, writefds, exceptfds, timeout);
}

typedef ssize_t (*recvfrom_func_t)(int sockfd, void *buf, size_t len, int flags, void *src_addr, void *addrlen);
recvfrom_func_t next_recvfrom;
ssize_t md40_recvfrom(int sockfd, void *buf, size_t len, int flags, void *src_addr, void *addrlen) {
   // printf("md40_recvfrom\n");
    // If this is our MicroDog USBDaemon Response, do the thing!
    if(src_addr != NULL){
        if(!memcmp(src_addr+2,MD40_DAEMON_ADDR,strlen(MD40_DAEMON_ADDR))){
            memcpy(buf,md40_last_packet,MD40_PACKET_SIZE);
            memset(md40_last_packet,0x00,MD40_PACKET_SIZE);
            md40_last_sock_fd = -1;
            return MD40_PACKET_SIZE;
        }    
    }

    // Otherwise, Next!
    return next_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

typedef ssize_t (*sendto_func_t)(int sockfd, const void *buf, size_t len, int flags, void *dest_addr, unsigned int addrlen);
sendto_func_t next_sendto;
ssize_t md40_sendto(int sockfd, const void *buf, size_t len, int flags, void *dest_addr, unsigned int addrlen) {
   // printf("md40_sendto\n");
    // If this is our MicroDog USBDaemon, do the thing pls!
    if(dest_addr != NULL){    
        if(!memcmp(dest_addr+2,MD40_DAEMON_ADDR,strlen(MD40_DAEMON_ADDR))){
            md40_last_sock_fd = sockfd;
            memcpy(md40_last_packet,buf,len);
            return len;
        }
    }
    // Otherwise, we don't care.
    return next_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","ioctl", md34_ioctl, &next_ioctl, 0),    
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","select", md40_select, &next_select, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","recvfrom", md40_recvfrom, &next_recvfrom, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","sendto", md40_sendto, &next_sendto, 0),
    {}    
};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("MICRODOG","version",CONFIG_TYPE_INT,&protocol_version,sizeof(protocol_version)),
  {}
};

const PHookEntry plugin_init(void){
    PIUTools_Config_Read(plugin_config);    

    PIUTools_Path_Resolve("${GAME_ROM_PATH}/microdog.dongle",dongle_file_path);
    // Add a fallback to the dongle path if we can't resolve it from ROMPATH
    if(!PIUTools_Filesystem_Path_Exist(dongle_file_path)){
        strcpy(dongle_file_path,"./microdog.dongle");
    }

    // At this point, we'll only continue this plugin if the dongle file exists.
    if(!PIUTools_Filesystem_Path_Exist(dongle_file_path)){ 
        DBG_printf("[%s] Fail: Microdog File not found: %s", __FILE__,dongle_file_path);
        return NULL;
    }

    DBG_printf("[%s] MicroDog Dongle File Loaded: %s",__FILE__,dongle_file_path);  
    MicroDog_Init(dongle_file_path);
       
    
    // Based on the API version of the dongle, we have to enable different hooks.   
    FILE* fp;    
    switch(protocol_version){
        case 34:
            entries[0].hook_enabled = 1;
            PIUTools_Path_Resolve("${TMP_ROOT_PATH}/usbdog",fake_dongle_dev_path);
            fp = fopen(fake_dongle_dev_path,"wb");
            fwrite("wat",3,1,fp);
            fclose(fp);
            chmod(fake_dongle_dev_path, 0666);
            PIUTools_Filesystem_AddRedirect(MD34_PATH_USB,fake_dongle_dev_path);
            break;
        case 40:
            entries[1].hook_enabled = 1;
            entries[2].hook_enabled = 1;                    
            entries[3].hook_enabled = 1;                    
            break;
        default:
            DBG_printf("[%s] MicroDog Protocol: %d Invalid - Options are 34 or 40",__FILE__,protocol_version);   
            break;
    }    
    DBG_printf("[%s] MicroDog Protocol: %d Initialized!",__FILE__,protocol_version);
    return entries;
}
