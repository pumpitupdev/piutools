// Compatibility Layer for APUG0PCB007 - Andamiro MK6 I/O [USB IO]
#include <stdio.h>
#include <stdint.h>

#include <PIUTools_SDK.h>
#include "apug0pcb007.h"

#define SET_IO_STATE(state, byte, offset, val) ((state) ? (byte[offset] |= val) : (byte[offset] &= ~val))
unsigned int mk6io_read_input(void){
  /* Player 1 */
unsigned char input_state[4] = {0};

SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UL],    input_state,0,1);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_UR],    input_state,0,2);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_CENTER],input_state,0,4);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_DL],    input_state,0,8);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P1_PAD_DR],    input_state,0,0x10);

/* Player 2 */
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_UL],    input_state,2,1);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_UR],    input_state,2,2);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_CENTER],input_state,2,4);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_DL],    input_state,2,8);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_P2_PAD_DR],    input_state,2,0x10);

/* System */
SET_IO_STATE(PIUTools_IO_IN[PINPUT_TEST_SWITCH],     input_state,1,0x02);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_COIN_1],          input_state,1,0x04);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_SERVICE_SWITCH],  input_state,1,0x40);
SET_IO_STATE(PIUTools_IO_IN[PINPUT_CLEAR_SWITCH],    input_state,1,0x80);


/*  Apparently, "touching" byte 3 as a whole causes some random and weird input
    triggering which results in the service menu popping up. Don't clear the
    whole byte, touch coin2 only to avoid this 
*/
SET_IO_STATE(PIUTools_IO_IN[PINPUT_COIN_2],    input_state,3,0x04);     

  return ~*(unsigned int*)input_state;
}

/*
  Output IO
  byte[0] bits:
  02 P1_CCFL_UL
  03 P1_CCFL_UR
  04 P1_CCFL_CENTER
  05 P1_CCFL_DL
  06 P1_CCFL_DR
  10 NEON

  byte[1] bits:
  02 P2_CCFL_UL
  03 P2_CCFL_UR
  04 P2_CCFL_CENTER
  05 P2_CCFL_DL
  06 P2_CCFL_DR  
  07 HALO_R2
  08 HALO_R1
  09 HALO_L2
  10 HALO_L1
*/
void mk6io_write_output(unsigned char* bytes){
    PIUTools_IO_OUT[POUTPUT_P1_PAD_UL] = (bytes[0] & (1 << 2)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P1_PAD_UR] = (bytes[0] & (1 << 3)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P1_PAD_CENTER] = (bytes[0] & (1 << 4)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P1_PAD_DL] = (bytes[0] & (1 << 5)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P1_PAD_DR] = (bytes[0] & (1 << 6)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_LAMP_BASS] = (bytes[0] & (1 << 10)) ? 1 : 0;

    PIUTools_IO_OUT[POUTPUT_P2_PAD_UL] = (bytes[0] & (1 << 2)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P2_PAD_UR] = (bytes[0] & (1 << 3)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P2_PAD_CENTER] = (bytes[0] & (1 << 4)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P2_PAD_DL] = (bytes[0] & (1 << 5)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_P2_PAD_DR] = (bytes[0] & (1 << 6)) ? 1 : 0;

    PIUTools_IO_OUT[POUTPUT_LAMP_R2] = (bytes[0] & (1 << 7)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_LAMP_R1] = (bytes[0] & (1 << 8)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_LAMP_L2] = (bytes[0] & (1 << 9)) ? 1 : 0;
    PIUTools_IO_OUT[POUTPUT_LAMP_L1] = (bytes[0] & (1 << 10)) ? 1 : 0;
    return;
}
