// Logic for Ticket Dispenser
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#define OLD_TICKET_ENDPOINT "usb/013/038"
#define FAKE_TICKET_FD 0x1338
#define FAKE_TICKET_DEVICE_PATH "./fake_devices"
#define REAL_DEVICES_PATH "/sys/kernel/debug/usb/devices"


enum {
    ticketIDLE,
    ticketSTARTREQ,
    ticketWAITSTART,
    ticketWAITEND,
    ticketERROR
};

struct usbdevfs_ctrltransfer {
    unsigned char bRequestType;
    unsigned char bRequest;
    unsigned short wValue;
    unsigned short wIndex;
    unsigned short wLength;
    unsigned int timeout;  /* in milliseconds */
    void *data;
};

#define USB_READ 0xC0
#define USB_WRITE 0x40

typedef struct TState{
    bool set[2];
    bool last[2];
    bool sensor[2];
    unsigned int ticket_total[2];
}TicketControllerState;

static const unsigned char reqtbl[2] = {1, 2};
static const unsigned char sensortbl[2] = {0x10, 0x04};

static TicketControllerState tstate = {false,false,false,false,false,false,0,0};


#define SetSensor(n) out[5] |= sensortbl[n]
#define ClearSensor(n) out[5] &= ~sensortbl[n]

void TicketControllerRead(unsigned char* out){
    /*
     TODO: Processing
     Eventually determine if the output sensors are set or not.
     */
    for(int n=0;n<2;n++){
        if(!tstate.set[n] && !tstate.last[n]){
            // We're probably in ticketIDLE or ticketERROR
        }else if(!tstate.set[n] && tstate.last[n]){
            // We're unset but used to be set - we are probably dispensing or errored.
            // Reset the counter and do some song and dance.
            //printf("[TicketController::Write] Dispenser: %d CLEAR\n",n);
            if(tstate.ticket_total[n]){
                printf("[TicketController] P%d Dispensing Ticket\n",n+1);
            }

            tstate.ticket_total[n] = 0;
        }else if(tstate.set[n] && !tstate.last[n]){ // Definitely ticketSTARTREQ


        }else{ // tstate.set[n] && tstate.last[n]
            tstate.sensor[n] = !tstate.sensor[n];
        }

        // Regardless, at the end, we're going to flip the sensor or not.
        if(tstate.sensor[n]){
            SetSensor(n);
        }else{
            ClearSensor(n);
        }
    }
}

#define CheckReq(n) in[2] & reqtbl[n]
void TicketControllerWrite(unsigned char* in){
    for(int n=0;n<2;n++) {
        tstate.last[n] = tstate.set[n];
        tstate.set[n] = (CheckReq(n) > 0);
        if(tstate.last[n] != tstate.set[n]){
            printf("Ticket Controller State Change: %d %d => %d\n",n,tstate.last[n],tstate.set[n]);
        }
    }
    // TODO: This might be where a "print" happens but I'm not sure yet...
}


void parse_ticketcmd(void* data) {
    struct usbdevfs_ctrltransfer *ctrl = data;
    switch(ctrl->bRequestType){
        case USB_READ:
            TicketControllerRead((unsigned char*)ctrl->data);
            break;
        case USB_WRITE:
            TicketControllerWrite((unsigned char*)ctrl->data);
            break;
        default:
            break;
    }
}


void make_faketicketdevices(char* devices_path){
    char line[512]={0x00};
    FILE* f = fopen(REAL_DEVICES_PATH,"rt");
    if(!f){return;}
    FILE* g = fopen(FAKE_TICKET_DEVICE_PATH,"wt");
    while (!feof(f)) {
        fgets(line, sizeof(line), f);
        fputs(line,g);
    }
    fputs("T:  Bus=13 Lev=00 Prnt=00 Port=00 Cnt=00 Dev#= 38 Spd=480  MxCh= 6\nP:  Vendor=0d2f ProdID=1004 Rev= 13.38 \n",g);
    fclose(f);
    fclose(g);
    strcpy(devices_path,"./fake_devices");
}