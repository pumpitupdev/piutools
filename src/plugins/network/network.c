// Network Redirect Plugin
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <time.h>

#include <PIUTools_SDK.h>

#define MAX_REDIRECT_ENTRIES 16

typedef struct _NET_REDIRECT_ENTRY{
    char src[128];
    char dest[128];
    unsigned short src_port;
    unsigned short dest_port;
}NetRedirectEntry,*PNetRedirectEntry;


static int redirect_entry_count = 0;
static NetRedirectEntry NetRedirectEntries[MAX_REDIRECT_ENTRIES] = {{0}};
static int g_block_network = 0;
static int g_log_network = 0;
static char g_network_redirect_str[4096]={0x00};

typedef int (*connect_func)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
static connect_func next_connect;

typedef struct hostent* (*gethostbyname_func)(const char* name);
static gethostbyname_func next_gethostbyname;

static int send_hook_enabled = 0;
static int recv_hook_enabled = 0;
static char log_path[4096] = {0x00};
typedef ssize_t (*recv_t)(int socket, void *buffer, size_t length, int flags);
typedef ssize_t (*send_t)(int socket, const void *buffer, size_t length, int flags);
static recv_t next_recv;
static send_t next_send;
static int log_lock = 0;

static FILE* open_log_file(){
    FILE* fp;
    while(log_lock){}
    log_lock = 1;
    fp = fopen(log_path,"ab");
    if(!fp){
        printf("Open Log File Error occurred: %s!\n",log_path);
        exit(-1);
    }
    return fp;
}

static void close_log_file(FILE* fp){
    fclose(fp);
    log_lock = 0;
}

static void LogData(const char* prefix, const void* buffer, size_t count){    
    FILE* fp = open_log_file();
    fwrite(prefix,strlen(prefix),1,fp);
    fwrite(buffer,count,1,fp);    
    fwrite("-END-",strlen("-END-"),1,fp);    
    close_log_file(fp);
}

static unsigned int is_ip_address(char *ipAddress){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

static ssize_t network_recv(int socket, void *buffer, size_t length, int flags){
    ssize_t res =  next_recv(socket,buffer,length,flags);
    // We'll skip heartbeat packets.
    if(length !=16){    
        LogData("RECV",buffer,res);
    }
    return res;
}

 static ssize_t network_send(int socket, const void *buffer, size_t length, int flags){
    // We'll skip heartbeat packets.
    if(length !=16){
        LogData("SEND",buffer,length);
    }
    return next_send(socket,buffer,length,flags);
 }

static struct hostent* network_gethostbyname(const char* name) {

    if(g_block_network) { return NULL; }
    struct hostent* h = next_gethostbyname(name);
    if(h == NULL) { return NULL; } // If gethostbyname failed
    for(int i = 0; i < redirect_entry_count; i++) {
        if(!strcmp(NetRedirectEntries[i].src, name)) {
            if(!is_ip_address(NetRedirectEntries[i].dest)) {
                return next_gethostbyname(NetRedirectEntries[i].dest);            
            }
            // If it's an IP address, we'll update it here.
            struct in_addr dest_addr;
            inet_pton(AF_INET, NetRedirectEntries[i].dest, &dest_addr);
            if(h->h_addr_list[0] == NULL){
                h->h_addr_list[0] = (char*) malloc(h->h_length);
            }
            memcpy(h->h_addr_list[0],&dest_addr,h->h_length);
            break;
        }
    }
    return h;
}

static int network_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    if(g_block_network){return -1;}

    // We're only dealing with IPV4 at the moment...
    if (addr->sa_family == AF_INET) {
        char ipstr[64];
        const void *src;
        src = &((struct sockaddr_in *)addr)->sin_addr;
        inet_ntop(addr->sa_family, src, ipstr, sizeof(ipstr));
        unsigned short src_port = ntohs(((struct sockaddr_in *)addr)->sin_port); 
        DBG_printf("[%s] Connecting to: %s:%d\n",__FUNCTION__,ipstr,src_port);
        for(int i=0;i<redirect_entry_count;i++){
            // We'll skip anything that isn't an ip address.
            if(!is_ip_address(NetRedirectEntries[i].dest)){continue;}
            if(!strcmp(NetRedirectEntries[i].src,ipstr) && (NetRedirectEntries[i].src_port == 0 || NetRedirectEntries[i].src_port == src_port)){
                if(NetRedirectEntries[i].dest_port){
                    ((struct sockaddr_in *)addr)->sin_port = htons(NetRedirectEntries[i].dest_port);
                }
                DBG_printf("[%s] Redirecting to %s:%d\n",__FUNCTION__,NetRedirectEntries[i].dest,ntohs(((struct sockaddr_in *)addr)->sin_port));
                inet_pton(addr->sa_family, NetRedirectEntries[i].dest, &((struct sockaddr_in *)addr)->sin_addr);
                break;
            }
        }
        
    }    

    return next_connect(sockfd,addr,addrlen);
}



static void print_net_redirect_entries(void){
    for(int i=0;i<redirect_entry_count;i++){
        DBG_printf("Location: %s Port: %d => Location: %s Port: %d\n",NetRedirectEntries[i].src,NetRedirectEntries[i].src_port,NetRedirectEntries[i].dest,NetRedirectEntries[i].dest_port);
    }
}

static void parse_network_redirect(const char* net_redirect_src){
    char* working_str = malloc(strlen(net_redirect_src)+1);
    strcpy(working_str,net_redirect_src);
    char *pt = strtok (working_str,",");
    while (pt != NULL) {
        if(redirect_entry_count == MAX_REDIRECT_ENTRIES){break;}

        char working_src[128]={0x00};
        char working_dest[128]={0x00};
        // Find the delimiter from src->dest.
        const char* delim_offset = strstr(pt,";");
        if(!delim_offset){
            pt = strtok (NULL, ",");
            continue;
        }
        int src_len = delim_offset - pt;
        int dest_len = strlen(delim_offset)+1;
        char* port_offset = NULL;
        strncpy(working_src,pt,src_len);
        strncpy(working_dest,delim_offset+1,dest_len);

        // Parse Src Entry
        if(strstr(working_src,":") != NULL){
            port_offset = strstr(working_src,":");            
            NetRedirectEntries[redirect_entry_count].src_port = atoi(port_offset+1);
            port_offset[0] = 0x00;
        }
        
        strcpy(NetRedirectEntries[redirect_entry_count].src,working_src);
    
        // Parse Dest Entry
        if(strstr(working_dest,"${PIUTOOLS_HOST}") != NULL){
            char tmp_path[1024] = {0x00};
            char* host_ip = getenv("PIUTOOLS_HOST_IP");
            if( host_ip == NULL){
                host_ip = "127.0.0.1";
            }
            strcpy(tmp_path,host_ip);
            strcat(tmp_path,working_dest+strlen("${PIUTOOLS_HOST}"));
            strcpy(working_dest,tmp_path);
        }

        if(strstr(working_dest,":") != NULL){
            port_offset = strstr(working_dest,":");            
            NetRedirectEntries[redirect_entry_count].dest_port = atoi(port_offset+1);
            port_offset[0] = 0x00;
        }

        strcpy(NetRedirectEntries[redirect_entry_count].dest,working_dest);      
        redirect_entry_count++;
        pt = strtok (NULL, ",");
    }
    print_net_redirect_entries();
    free(working_str);
}





static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","gethostbyname", network_gethostbyname, &next_gethostbyname, 1),    
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","connect", network_connect, &next_connect, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","send", network_send, &next_send, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6","recv", network_recv, &next_recv, 0),
    {}
};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("NETWORK","redirects",CONFIG_TYPE_STRING,g_network_redirect_str,sizeof(g_network_redirect_str)),
  CONFIG_ENTRY("NETWORK","block_network",CONFIG_TYPE_BOOL,&g_block_network,sizeof(g_block_network)),
  CONFIG_ENTRY("NETWORK","log_network",CONFIG_TYPE_BOOL,&g_log_network,sizeof(g_log_network)),
  {}
};

const PHookEntry plugin_init(void){
    PIUTools_Config_Read(plugin_config);  
    // Parse Redirect urls
    if(strlen(g_network_redirect_str) > 0){
        parse_network_redirect(g_network_redirect_str);
    }  
    if(g_log_network){
        PIUTools_Path_Resolve("${PIUTOOLS_ROOT_PATH}/network_log.txt",log_path);
        entries[2].hook_enabled = 1;
        entries[3].hook_enabled = 1;
    }
    
    return entries;
}



