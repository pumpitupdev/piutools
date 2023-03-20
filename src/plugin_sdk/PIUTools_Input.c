#include <stdio.h>
#include <string.h>

#include "PIUTools_Input.h"


unsigned char PIUTools_IO_OUT[sizeof(POUTPUT_MAX)] = {0x00};
unsigned char PIUTools_IO_IN[sizeof(PINPUT_MAX)] = {0x00};


void PIUTools_Input_Reset(void){
    memset(PIUTools_IO_OUT,0x00,sizeof(POUTPUT_MAX));
    memset(PIUTools_IO_IN,0x00,sizeof(PINPUT_MAX));
}