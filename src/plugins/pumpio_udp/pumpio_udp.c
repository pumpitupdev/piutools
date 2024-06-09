// A Network-Backed Pump IO Handler
#include <stdio.h>   // for printf() and fprintf()
#include <stdlib.h>  // for atoi() and exit()
#include <string.h>  // for memset()
#include <unistd.h>  // for close()
#include <arpa/inet.h> // for sockaddr_in and inet_ntoa()
#include <sys/socket.h> // for socket(), connect(), send(), and recv()

#include <pthread.h>
#include <PIUTools_SDK.h>


#define TEST_SERVER_IP "10.0.0.123"
#define SERVER_CONNECT_PORT 15572
#define SERVER_INPUT_PORT 15573
#define SERVER_OUTPUT_PORT 15574
static char* server_ip = TEST_SERVER_IP;
static int server_connected = 0;


void* pumpionet_output_sender(void *arg){
    int sockfd = -1;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        DBG_printf("[%s:%s] Socket Create Failed!",__FILE__,__FUNCTION__);
        return 0;
    }
    struct sockaddr_in broadcastAddr; 
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   
    broadcastAddr.sin_family = AF_INET;                 
    broadcastAddr.sin_addr.s_addr = inet_addr(server_ip);
    broadcastAddr.sin_port = htons(SERVER_OUTPUT_PORT);

    DBG_printf("[%s:%s] Connected to Server!",__FILE__,__FUNCTION__);
    // At this point, we're connected - keep sending output until you don't.    
    unsigned long long last_state = 0;
    while (1) {
        unsigned long long out_state = 0;
        for(int i = 0; i < POUTPUT_MAX; i++){            
            out_state |= (PIUTools_IO_OUT[i] << i);
        }
        if(out_state != last_state){
            sendto(sockfd, &out_state, sizeof(out_state), 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr));
            last_state = out_state;
        }
        
    }  
}

void *pumpionet_input_receiver(void *arg){
    int sockfd = -1;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        DBG_printf("[%s:%s] Socket Create Failed!",__FILE__,__FUNCTION__);
        return 0;
    }
    struct sockaddr_in broadcastAddr; 
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   
    broadcastAddr.sin_family = AF_INET;                 
    broadcastAddr.sin_addr.s_addr = inet_addr(server_ip);
    broadcastAddr.sin_port = htons(SERVER_INPUT_PORT);

    char message = 1;
    if (sendto(sockfd, &message, 1, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) != 1){
        DBG_printf("[%s:%s] Socket ConnectSend Failed!\n",__FILE__,__FUNCTION__);
        return NULL;        
    }

    DBG_printf("[%s:%s] Connected to Server!",__FILE__,__FUNCTION__);
    // At this point, we're connected - keep receiving input until you don't.
    int recvLen = 0;
    int recvBuffer; 
    while (1) {
        if ((recvLen = recvfrom(sockfd, &recvBuffer, sizeof(recvBuffer), 0, NULL, 0)) < 0){
            DBG_printf("[%s:%s] Socket RecvFrom Failed!",__FILE__,__FUNCTION__);
            break;
        }

        for(int i = 0; i < PINPUT_MAX; i++){
            PIUTools_IO_IN[i] = (recvBuffer & (1 << i)) ? 1 : 0;
        }

        if(PIUTools_IO_IN[PINPUT_APP_EXIT]){
            DBG_printf("IO Called Exit Code!");
            exit(0);
        }
    }
}

// Connect and Handshake with the Server  
unsigned char pumpionet_connect(void){
    int sockfd = -1;
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        DBG_printf("[%s:%s] Socket Create Failed!",__FILE__,__FUNCTION__);
        return 0;
    }
    struct sockaddr_in broadcastAddr; 
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   
    broadcastAddr.sin_family = AF_INET;                 
    broadcastAddr.sin_addr.s_addr = inet_addr(server_ip);
    broadcastAddr.sin_port = htons(SERVER_CONNECT_PORT);

    char message = 1;
    if (sendto(sockfd, &message, 1, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) != 1){
        DBG_printf("[%s:%s] Socket ConnectSend Failed!",__FILE__,__FUNCTION__);
        return 0;        
    }

    return 1;
}


const PHookEntry plugin_init(void){   

    if(getenv("PIUTOOLS_HOST_IP") != NULL){
        server_ip = getenv("PIUTOOLS_HOST_IP");
    }    

    if(pumpionet_connect()){
        pthread_t net_input_thread;
        pthread_t net_output_thread;    
        pthread_create(&net_input_thread, NULL, pumpionet_input_receiver, NULL);
        pthread_create(&net_output_thread, NULL, pumpionet_output_sender, NULL);         
    }

    return NULL;
}
