// Logic for Ticket Dispenser
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ticket.h"

#define USB_READ 0xC0
#define USB_WRITE 0x40

enum {
    ticketIDLE,
    ticketSTARTREQ,
    ticketWAITSTART,
    ticketWAITEND,
    ticketERROR
};

static int Stat[2];

typedef struct TState {
    char set[2];
    char last[2];
    char sensor[2];
    unsigned int ticket_total[2];
} TicketControllerState;

static const unsigned char reqtbl[2] = {1, 2};
static const unsigned char sensortbl[2] = {0x10, 0x04};

static TicketControllerState tstate = {0, 0, 0, 0, 0, 0, 0, 0};

#define SetSensor(n) out[5] |= sensortbl[n]
#define ClearSensor(n) out[5] &= ~sensortbl[n]
#define ClearOut(n) out[2] &= ~sensortbl[n]

const char* get_ticket_state_str(int state) {
    switch (state) {
        case ticketIDLE:
            return "Ticket: IDLE";
        case ticketSTARTREQ:
            return "Ticket: STARTREQ";
        case ticketWAITSTART:
            return "Ticket: WAITSTART";
        case ticketWAITEND:
            return "Ticket: WAITEND";
        case ticketERROR:
            return  "Ticket: ERROR";
        default:
            return "UNKNOWN";
    }
}


void update_ticket_stat(int n, int state) {
    if (Stat[n] != state) {
        //printf("Player %d: %s => %s\n", n + 1, get_ticket_state_str(Stat[n]), get_ticket_state_str(state));
        Stat[n] = state;
    }
}

void TicketControllerRead(unsigned char* out) {
    for (int n = 0; n < 2; n++) {
        if (!tstate.set[n]) {
            if (tstate.ticket_total[n] > 0) {
                tstate.sensor[n] = true;
                update_ticket_stat(n, ticketWAITSTART);
            } else {
                tstate.sensor[n] = false;
                update_ticket_stat(n, ticketIDLE);
            }
        } else {
            if (!tstate.sensor[n]) {
                update_ticket_stat(n, ticketWAITEND);
            } else {
                update_ticket_stat(n, ticketSTARTREQ);
            }
            tstate.sensor[n] = !tstate.sensor[n];
        }

        if (tstate.sensor[n]) {
            SetSensor(n);
        } else {
            ClearSensor(n);
        }
    }
}

#define CheckReq(n) in[2] & reqtbl[n]
void TicketControllerWrite(unsigned char* in) {
    for (int n = 0; n < 2; n++) {
        bool prev_set = tstate.set[n];
        tstate.set[n] = (CheckReq(n) > 0);

        if (prev_set != tstate.set[n]) {
            if (tstate.set[n]) {
                tstate.ticket_total[n]--;
                printf("[TICKET DISPENSER] Player %d: PRINTING Ticket\n", n + 1);
            }
        }
    }
}


void parse_ticketcmd(void* data) {
    struct ticket_usbdevfs_ctrltransfer *ctrl = data;
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

void init_ticket_state(unsigned int ticket_total){
    tstate.ticket_total[0] = ticket_total;
    tstate.ticket_total[1] = ticket_total;
}