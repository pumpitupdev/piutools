// Compatibility Framework for Nexcade DLS IO
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>



#include <PIUTools_SDK.h>

#define DLSIO_PID 0x0010
#define DLSIO_VID 0x0905

struct DLSIO {
  uint8_t HiHat;
  uint8_t Snare;
  uint8_t Tom1;
  uint8_t Tom2;
  uint8_t Cymbal;
  uint8_t Bass;
  uint16_t Unk0; // service, coin/test in here but not sure which bits
};

unsigned char tv = 0;
#define SET_IO_STATE(state, byte, offset, val) ((state) ? (byte[offset] |= val) : (byte[offset] &= ~val))
void dlsio_read_input(uint8_t* input_state){
  /* Player 1 */
memset(input_state,0x00,8);

SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UL],    input_state,0,0xFF);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UR],    input_state,1,0xFF);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_CENTER],    input_state,5,0xFF);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_DL],input_state,2,0xFF);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_DR],    input_state,3,0xFF);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_UL],    input_state,4,0xFF);


SET_IO_STATE(PIUTools_IO_IN[PINPUT_COIN_1],          input_state,7,0x02);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_TEST_SWITCH],     input_state,7,0x10);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_SERVICE_SWITCH],  input_state,7,0x20);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_CLEAR_SWITCH],    input_state,7,0x40);

// Bit Invert bytes 6 and 7
input_state[6] = ~input_state[6];
input_state[7] = ~input_state[7];

  return;
}

void dlsio_write_output(uint8_t* out_state){
    // Write the 8 byte buffer of out_state as a hexstring.
   
    /* The layout of the lamp state is 
    RED_LAMP_L 0 40
    RED_LAMP_R 0 10
    BLUE_LAMP_L 0 80
    BLUE_LAMP_R 0 20
    BILLBOARD_LAMP 2 02
    BOTTOM_LAMP 0 3
    */

   // Print a pretty printout of the current illumination state.
   /*
    printf("RED_LAMP_L: %s\n",out_state[0] & 0x40 ? "ON" : "OFF");
    printf("RED_LAMP_R: %s\n",out_state[0] & 0x10 ? "ON" : "OFF");
    printf("BLUE_LAMP_L: %s\n",out_state[0] & 0x80 ? "ON" : "OFF");
    printf("BLUE_LAMP_R: %s\n",out_state[0] & 0x20 ? "ON" : "OFF");
    printf("BILLBOARD_LAMP: %s\n",out_state[2] & 0x02 ? "ON" : "OFF");
    printf("BOTTOM_LAMP: %s\n",out_state[0] & 0x03 ? "ON" : "OFF");
    */
    return;
}


#define USB_READ 0xC0
#define USB_WRITE 0x40
static char fake_dlsio_device_path[1024];

int piuinput_usb_control_msg(void *hdev, int requesttype, int request, int value, int index, uint8_t *bytes, int nbytes, int timeout) {
    switch(requesttype){
        case USB_READ:
            dlsio_read_input(bytes);
            break;
        case USB_WRITE:
            dlsio_write_output(bytes);
            break;
        default:
            break;
    }   
    return nbytes;
}

const PHookEntry plugin_init(void){   
    PUSBDevice nd = PIUTools_USB_Add_Device(USB_1_FULL_SPEED,0,DLSIO_VID,DLSIO_PID,"DLSIO",(void*)piuinput_usb_control_msg,NULL,NULL);
    PIUTools_USB_Connect_Device(nd->dev);
    sprintf(fake_dlsio_device_path,"/proc/bus/usb/%03d/%03d",nd->bus,nd->dev);
    printf("[%s] Created Fake DLSIO Device At: %s\n",__FILE__,fake_dlsio_device_path);
    return NULL;
}
