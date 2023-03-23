// Compatibility Layer for APUG0PCB007 - Andamiro MK6 I/O [USB IO]
#include <stdio.h>
#include <stdint.h>

#include <plugin_sdk/PIUTools_Input.h>
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


void mk6io_write_output(unsigned char* bytes){
    return;
    /*
    int P1_UPLEFT = *bytes & (1 << 2);
int P1_CEN = *bytes & (1 << 4); 
int P1_UPRIGHT = *bytes & (1 << 3);
int P1_DOWNLEFT  =  *bytes & (1 << 5);
int P1_DOWNRIGHT =  *bytes & (1 << 6);
int NEON = *bytes & (1 << 10);

//fprintf(stderr,"LEFT PAD:[%x][%x][%x][%x][%x]     ",P1_DOWNLEFT,P1_UPLEFT,P1_CEN,P1_UPRIGHT,P1_DOWNRIGHT);

bytes+=1;
int P2_UPLEFT = *bytes & (1 << 2);
int P2_CEN = *bytes & (1 << 4); 
int P2_UPRIGHT = *bytes & (1 << 3);
int P2_DOWNLEFT =  *bytes & (1 << 5);
int P2_DOWNRIGHT =  *bytes & (1 << 6);
//fprintf(stderr,"RIGHT PAD:[%x][%x][%x][%x][%x]   ",P2_DOWNLEFT,P2_UPLEFT,P2_CEN,P2_UPRIGHT,P2_DOWNRIGHT);
int HALOR2 = *bytes & (1 << 7);
int HALOL1 = *bytes & (1 << 10);
int HALOR1 = *bytes & (1 << 8);
int HALOL2 = *bytes & (1 << 9); 
system("clear");
char HL1,HL2,HR1,HR2,N,P1CN,P1UL,P1UR,P1DL,P1DR,P2CN,P2UL,P2UR,P2DL,P2DR;
if(HALOL1)HL1='O';
if(!HALOL1)HL1=' ';
if(HALOR1)HR1='O';
if(!HALOR1)HR1=' ';
if(HALOL2)HL2='O';
if(!HALOL2)HL2=' ';
if(HALOR2)HR2='O';
if(!HALOR2)HR2=' ';
if(NEON)N='O';
if(!NEON)N=' ';
//PADS
if(P1_CEN)P1CN='X';
if(!P1_CEN)P1CN=' ';
if(P2_CEN)P2CN='X';
if(!P2_CEN)P2CN=' ';
if(P1_UPLEFT)P1UL='X';
if(!P1_UPLEFT)P1UL=' ';
if(P2_UPLEFT)P2UL='X';
if(!P2_UPLEFT)P2UL=' ';
if(P1_UPRIGHT)P1UR='X';
if(!P1_UPRIGHT)P1UR=' ';
if(P2_UPRIGHT)P2UR='X';
if(!P2_UPRIGHT)P2UR=' ';
if(P1_DOWNLEFT)P1DL='X';
if(!P1_DOWNLEFT)P1DL=' ';
if(P2_DOWNLEFT)P2DL='X';
if(!P2_DOWNLEFT)P2DL=' ';
if(P1_DOWNRIGHT)P1DR='X';
if(!P1_DOWNRIGHT)P1DR=' ';
if(P2_DOWNRIGHT)P2DR='X';
if(!P2_DOWNRIGHT)P2DR=' ';
//fprintf(stderr,"CAB LAMPS:[%x][%x][%x][%x][%x]\n",HALOL1,HALOL2,NEON,HALOR1,HALOR2);
fprintf(stderr,"                            PUMP IT UP I/O DIAGRAM\n\n\n");
fprintf(stderr,"===========		 |-------------------------|\n");
fprintf(stderr,"|%c       %c|           (X)|   PUMP IT UP BITCHES    |(X)\n",P1UL,P1UR);
fprintf(stderr,"|	  |	      (%c)|-------------------------|(%c)\n",HL1,HR1);
fprintf(stderr,"|    %c    |LEFT	      (%c)|                         |(%c)\n",P1CN,HL2,HR2);
fprintf(stderr,"|	  |	         |                         |\n");
fprintf(stderr,"|%c	 %c|	         |        F Y G D C        |\n",P1DL,P1DR);
fprintf(stderr,"===========		 |                         |\n");
fprintf(stderr,"   		         |                         |\n");
fprintf(stderr,"===========		 |                         |\n");
fprintf(stderr,"|%c	 %c|              |-------------------------|\n",P2UL,P2UR);
fprintf(stderr,"|	  |	         |                         |\n");
fprintf(stderr,"|    %c	  |RIGHT         |    %c%c%c          %c%c%c     |\n",P2CN,N,N,N,N,N,N);
fprintf(stderr,"|	  |	         |   %c   %c        %c   %c    |\n",N,N,N,N);
fprintf(stderr,"|%c	 %c|	         |   %c   %c        %c   %c    |\n",P2DL,P2DR,N,N,N,N);
fprintf(stderr,"===========              |    %c%c%c          %c%c%c     |\n",N,N,N,N,N,N);
fprintf(stderr,"		         |                         |\n");
fprintf(stderr,"		         |=========================|\n");

int light;
if(NEON && !HALOL1 && !HALOR1) light = 4;
if(!NEON && HALOL1 && !HALOR1) light = 2;
if(!NEON && !HALOL1 && HALOR1) light = 1;
if(!NEON && !HALOL1 && !HALOR1) light = 0;
if(!NEON && HALOL1 && HALOR1) light = 3;
if(NEON && !HALOL1 && HALOR1) light = 5;
if(NEON && HALOL1 && !HALOR1) light = 6;
if(NEON && HALOL1 && HALOR1) light = 7;
    */

}
