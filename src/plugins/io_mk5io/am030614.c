// Emulation for Andamiro AM030614 MK5 Pump IO Board
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <PIUTools_SDK.h>
#include "am030614.h"


#define FP_LEFTUP		0x0001
#define FP_RIGHTUP		0x0002
#define FP_CENTER		0x0004
#define FP_LEFTDOWN		0x0008
#define FP_RIGHTDOWN	0x0010

#define BTN_COIN           	0x0400  
#define BTN_TEST            0x0200  
#define BTN_SERVICE         0x4000  
#define BTN_CLEAR         	0x8000

PIUIO_MK5_STATE piuio_state;



unsigned short PIUIO_HandleInput_1(){
    unsigned short value = 0;
    if(PIUTools_IO_IN[PINPUT_P1_PAD_UL]){value |= FP_LEFTUP;}
    if(PIUTools_IO_IN[PINPUT_P1_PAD_UR]){value |= FP_RIGHTUP;}
    if(PIUTools_IO_IN[PINPUT_P1_PAD_CENTER]){value |= FP_CENTER;}
    if(PIUTools_IO_IN[PINPUT_P1_PAD_DL]){value |= FP_LEFTDOWN;}
    if(PIUTools_IO_IN[PINPUT_P1_PAD_DR]){value |= FP_RIGHTDOWN;}
    if(PIUTools_IO_IN[PINPUT_TEST_SWITCH]){value |= BTN_TEST;}
    if(PIUTools_IO_IN[PINPUT_COIN_1]){value |= BTN_COIN;}
    if(PIUTools_IO_IN[PINPUT_SERVICE_SWITCH]){value |= BTN_SERVICE;}
    if(PIUTools_IO_IN[PINPUT_CLEAR_SWITCH]){value |= BTN_CLEAR;}
    return ~value;
}
unsigned short PIUIO_HandleInput_2(){

    unsigned short value = 0;
    if(PIUTools_IO_IN[PINPUT_P2_PAD_UL]){value |= FP_LEFTUP;}
    if(PIUTools_IO_IN[PINPUT_P2_PAD_UR]){value |= FP_RIGHTUP;}
    if(PIUTools_IO_IN[PINPUT_P2_PAD_CENTER]){value |= FP_CENTER;}
    if(PIUTools_IO_IN[PINPUT_P2_PAD_DL]){value |= FP_LEFTDOWN;}
    if(PIUTools_IO_IN[PINPUT_P2_PAD_DR]){value |= FP_RIGHTDOWN;}
    if(PIUTools_IO_IN[PINPUT_COIN_2]){value |= BTN_COIN;}
    return ~value;
}

void PIUIO_HandleOutput_1(unsigned short val){
    // TODO
}

void PIUIO_HandleOutput_2(unsigned short val){
    // TODO
}

void PIUIO_Init(){
    memset(&piuio_state,0,sizeof(PIUIO_MK5_STATE));
}