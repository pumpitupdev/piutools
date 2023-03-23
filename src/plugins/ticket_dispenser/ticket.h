#pragma once

struct ticket_usbdevfs_ctrltransfer {
    unsigned char bRequestType;
    unsigned char bRequest;
    unsigned short wValue;
    unsigned short wIndex;
    unsigned short wLength;
    unsigned int timeout;  /* in milliseconds */
    void *data;
};

void parse_ticketcmd(void* data);
void init_ticket_state(unsigned int ticket_total);