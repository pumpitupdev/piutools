#include <stdio.h>
#include <string.h>

#include <PIUTools_SDK.h>

#include "am_buttonboard.h"

static unsigned short btn_input[4];

void piubtn_read_input(unsigned char* input_buffer){

  memset(input_buffer, 0, PIUBTN_DRV_BUFFER_SIZE);

  /*
     byte 0:
   bit 0: P1 back (red)
   bit 1: P1 left
   bit 2: P1 right
   bit 3: P1 start (green)
   bit 4: P2 back (red)
   bit 5: P2 left
   bit 6: P2 right
   bit 7: P2 start (green)
  */

  /* Player 1 */
  if (PIUTools_IO_IN[PINPUT_P1_BB_BACK]) {
    input_buffer[0] |= (1 << 0);
  }

  if (PIUTools_IO_IN[PINPUT_P1_BB_LEFT]) {
    input_buffer[0] |= (1 << 1);
  }

  if (PIUTools_IO_IN[PINPUT_P1_BB_RIGHT]) {
    input_buffer[0] |= (1 << 2);
  }

  if (PIUTools_IO_IN[PINPUT_P1_BB_START]) {
    input_buffer[0] |= (1 << 3);
  }

  /* Player 2 */
  if (PIUTools_IO_IN[PINPUT_P2_BB_BACK]) {
    input_buffer[0] |= (1 << 4);
  }

  if (PIUTools_IO_IN[PINPUT_P2_BB_LEFT]) {
    input_buffer[0] |= (1 << 5);
  }

  if (PIUTools_IO_IN[PINPUT_P2_BB_RIGHT]) {
    input_buffer[0] |= (1 << 6);
  }

  if (PIUTools_IO_IN[PINPUT_P2_BB_START]) {
    input_buffer[0] |= (1 << 7);
  }

  /* xor inputs because pullup active */
  for (int i = 0; i < PIUBTN_DRV_BUFFER_SIZE; i++) {
    input_buffer[i] ^= 0xFF;
  }
}

void piubtn_write_output(unsigned char* output_buffer){


  /*
   byte 0:
   bit 0: p2 start (green)
   bit 1: p2 right
   bit 2: p2 left
   bit 3: p2 back (red)
   bit 4: p1 start (green)
   bit 5: p1 right
   bit 6: p1 left
   bit 7: p1 back (red)
  */

  /* Player 2 */
PIUTools_IO_OUT[POUTPUT_P2_BB_START] = (output_buffer[0] & (1 << 0)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P2_BB_RIGHT] = (output_buffer[0] & (1 << 1)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P2_BB_LEFT] = (output_buffer[0] & (1 << 2)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P2_BB_BACK] = (output_buffer[0] & (1 << 3)) ? 1 : 0;


  /* Player 1 */
PIUTools_IO_OUT[POUTPUT_P1_BB_START] = (output_buffer[0] & (1 << 4)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P1_BB_RIGHT] = (output_buffer[0] & (1 << 5)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P1_BB_LEFT] = (output_buffer[0] & (1 << 6)) ? 1 : 0;
PIUTools_IO_OUT[POUTPUT_P1_BB_BACK] = (output_buffer[0] & (1 << 7)) ? 1 : 0;

}