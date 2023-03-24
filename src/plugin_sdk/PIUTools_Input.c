#include <stdio.h>
#include <string.h>

#include "PIUTools_Input.h"


unsigned char PIUTools_IO_OUT[POUTPUT_MAX] = {0x00};
unsigned char PIUTools_IO_IN[PINPUT_MAX] = {0x00};


void PIUTools_Input_Reset(void){
    memset(PIUTools_IO_OUT,0x00,POUTPUT_MAX);
    memset(PIUTools_IO_IN,0x00,PINPUT_MAX);
}