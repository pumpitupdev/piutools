// Plugin for RainbowChina/SafeNET MicroDOG API Version 3.4
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <microdog.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#define MD34_FAKE_FD 0x1337

static char dongle_file_path[1024] = {0x00};
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

typedef int (*open_func_t)(const char *pathname, int flags);
static open_func_t next_open;
static int md34_open(const char *pathname, int flags) {
    // If this is a request to open our microdog, we're just going to return with a fake handle.     
    if(strcmp(pathname,MD34_PATH_USB) == 0){
        return MD34_FAKE_FD;
    }       

    // Call the original open function
    return next_open(pathname, flags);
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
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","open", md34_open, &next_open, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","select", md40_select, &next_select, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","recvfrom", md40_recvfrom, &next_recvfrom, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","sendto", md40_sendto, &next_sendto, 0),
    {}    
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"MICRODOG") == 0){
        if(strcmp(name,"file") == 0){
            if(value == NULL){return 0;}
            piutools_resolve_path(value,dongle_file_path);    
            DBG_printf("[%s] MicroDog Dongle File Loaded: %s",__FILE__,dongle_file_path);        
            MicroDog_Init(dongle_file_path);
        }
        if(strcmp(name,"version") == 0){
            char *ptr;            
            if(value != NULL){
                protocol_version = strtoul(value,&ptr,10);
                switch(protocol_version){
                    case 34:
                        entries[0].hook_enabled = 1;
                        entries[1].hook_enabled = 1;
                        break;
                    case 40:
                        entries[2].hook_enabled = 1;
                        entries[3].hook_enabled = 1;                    
                        entries[4].hook_enabled = 1;                    
                        break;
                    default:
                        DBG_printf("[%s] MicroDog Protocol: %d Invalid - Options are 34 or 40",__FILE__,protocol_version);   
                        break;
                }

            }
        }        
        
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  return entries;
}
