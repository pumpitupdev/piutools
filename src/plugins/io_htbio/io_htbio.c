// Compatibility Framework for Nexcade DLS IO
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>



#include <PIUTools_SDK.h>

#define HTBIO_PID 0x8020
#define HTBIO_VID 0x0D2F
#define PIUBTN_DRV_BUFFER_SIZE 8

#define SET_IO_STATE(state, byte, offset, val) ((state) ? (byte[offset] |= val) : (byte[offset] &= ~val))

void HTBIO_read_input(unsigned char* input_buffer){

  memset(input_buffer, 0, PIUBTN_DRV_BUFFER_SIZE);

 


 
  
// SW1
SET_IO_STATE(PIUTools_IO_IN[PINPUT_TEST_SWITCH],     input_buffer,4,0x01);
// SW2
SET_IO_STATE(PIUTools_IO_IN[PINPUT_SERVICE_SWITCH],  input_buffer,4,0x02);
// SERVICE
SET_IO_STATE(PIUTools_IO_IN[PINPUT_CLEAR_SWITCH],    input_buffer,4,0x04);
// COIN 1
SET_IO_STATE(PIUTools_IO_IN[PINPUT_COIN_1],          input_buffer,3,0x01);
// COIN 2
SET_IO_STATE(PIUTools_IO_IN[PINPUT_COIN_2],          input_buffer,3,0x02);


// P1 LEFT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UL],          input_buffer,4,0x40);
// P1 RIGHT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UR],          input_buffer,4,0x20);
// P1 SELECT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_CENTER],          input_buffer,4,0x80);
// P1 DRUM SENSOR
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_DL] || PIUTools_IO_IN[PINPUT_P1_PAD_DR], input_buffer,2,0x04);

// P2 LEFT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_UL],          input_buffer,2,0x10);
// P2 RIGHT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_UR],          input_buffer,2,0x20);
// P2 SELECT
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_CENTER],          input_buffer,2,0x40);
// P2 DRUM SENSOR
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_DL],          input_buffer,2,0x08);



// Bit Invert bytes
for(int i = 0; i <PIUBTN_DRV_BUFFER_SIZE; i++){
    input_buffer[i] = ~input_buffer[i];
}
}
static unsigned char last_output_buffer[PIUBTN_DRV_BUFFER_SIZE];
void HTBIO_write_output(unsigned char* output_buffer){

// TODO: Figure out the Light outputs when we care to figure them out
// Write Ouput Buffer Values to Console, only if they differ from the last state
/*

if((memcmp(output_buffer, last_output_buffer, 4) != 0) || (memcmp(output_buffer+6, last_output_buffer+6, 3) != 0)){
printf("Output Buffer: ");
for(int i = 0; i < PIUBTN_DRV_BUFFER_SIZE; i++){
    printf("%02X ", output_buffer[i]);
}
printf("\n");

}
memcpy(last_output_buffer, output_buffer, PIUBTN_DRV_BUFFER_SIZE);
*/

/*
output_buffer[2] 0x01 when force tickets on 2p ticket counter
output_buffer[2] 0x02 (2P Ticket Button Lamp)
[5] color for drum
0x70 is white
0x60 is blue
0x38 is pattern 1
0x20 is pattern 2
off is 0x78
0x68 is red
0x58 is green

p2 drum lamps
0x08 red
0x48 is green
0x28 is blue
0x10 is white
0x30 is pattern 1
0x18 is pattern 2

*/
}

#define USB_READ 0xC0
#define USB_WRITE 0x40
static char fake_HTBIO_device_path[1024];

int piuinput_usb_control_msg(void *hdev, int requesttype, int request, int value, int index, uint8_t *bytes, int nbytes, int timeout) {
    switch(requesttype){
        case USB_READ:
            HTBIO_read_input(bytes);
            break;
        case USB_WRITE:
            HTBIO_write_output(bytes);
            break;
        default:
            break;
    }   
    return nbytes;
}

const PHookEntry plugin_init(void){   
    PUSBDevice nd = PIUTools_USB_Add_Device(USB_1_FULL_SPEED,0,HTBIO_VID,HTBIO_PID,"HTBIO",(void*)piuinput_usb_control_msg,NULL,NULL);
    PIUTools_USB_Connect_Device(nd->dev);
    sprintf(fake_HTBIO_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
    printf("[%s] Created Fake HTBIO Device At: %s\n",__FILE__,fake_HTBIO_device_path);
    return NULL;
}
