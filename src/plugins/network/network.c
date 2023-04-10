// Filesystem Redirect Plugin
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>


#include <arpa/inet.h>
#include <netinet/in.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>


typedef int (*connect_func)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
static connect_func next_connect;

typedef int (*curl_easy_setopt_t)(int * handle, int option, void * param);
static curl_easy_setopt_t next_curl_easy_setopt;


static int block_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    char ipstr[64];
    const void *src;
    if (addr->sa_family == AF_INET) {
        src = &((struct sockaddr_in *)addr)->sin_addr;
    } else if (addr->sa_family == AF_INET6) {
        src = &((struct sockaddr_in6 *)addr)->sin6_addr;
    } else {
        // unsupported address family
        return -1;
    }
    inet_ntop(addr->sa_family, src, ipstr, sizeof(ipstr));
    //printf("Connecting to IP address: %s\n", ipstr);
    if(!strcmp(ipstr,"115.68.108.183")){
        return -1;
    }
    
    // call the original connect function
    return next_connect(sockfd, addr, addrlen);
}

int block_curl_easy_setopt(int * handle, int option, void * param){
    int result = 0;

    if (option == 10002 && memcmp("http://update_kr", (char*)param, 16) == 0)
    {
        printf("curl_easy_setopt: blocking update download request\n");
        result = next_curl_easy_setopt(handle, option, "does-not-exist");
    }
    else
    {
        result = next_curl_easy_setopt(handle, option, param);
    }
    return result;
}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libcurl.so.4","curl_easy_setopt", block_curl_easy_setopt, &next_curl_easy_setopt, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","connect", block_connect, &next_connect, 1),
    {}
};


static int parse_config(void* user, const char* section, const char* name, const char* value){    
    if(strcmp(section,"NETWORK") == 0){
        if(value == NULL){return 1;}     
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    return entries;
}



