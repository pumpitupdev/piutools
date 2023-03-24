#pragma once


enum _PIUTOOLS_INPUT{
    // System Helpers - Non Game
    PINPUT_APP_EXIT,

    // System Switches
    PINPUT_SERVICE_SWITCH,
    PINPUT_TEST_SWITCH,
    PINPUT_CLEAR_SWITCH,
    PINPUT_COIN_1,
    PINPUT_COIN_2,

    // Player 1 Controls - Pad
    PINPUT_P1_PAD_UL,
    PINPUT_P1_PAD_UR,
    PINPUT_P1_PAD_CENTER,
    PINPUT_P1_PAD_DL,
    PINPUT_P1_PAD_DR,

    // Player 1 Controls - ButtonBoard
    PINPUT_P1_BB_START,
    PINPUT_P1_BB_BACK,
    PINPUT_P1_BB_RIGHT,
    PINPUT_P1_BB_LEFT,
    PINPUT_P1_BB_UPLEFT,
    PINPUT_P1_BB_UPRIGHT,

    // Player 1 Controls - Profile
    PINPUT_P1_USB_IN,
    PINPUT_P1_NFC_READ,

    // Player 2 Controls - Pad
    PINPUT_P2_PAD_UL,
    PINPUT_P2_PAD_UR,
    PINPUT_P2_PAD_CENTER,
    PINPUT_P2_PAD_DL,
    PINPUT_P2_PAD_DR,

    // Player 2 Controls - ButtonBoard
    PINPUT_P2_BB_START,
    PINPUT_P2_BB_BACK,
    PINPUT_P2_BB_RIGHT,
    PINPUT_P2_BB_LEFT,
    PINPUT_P2_BB_UPLEFT,
    PINPUT_P2_BB_UPRIGHT,

    // Player 2 Controls - Profile
    PINPUT_P2_USB_IN,
    PINPUT_P2_NFC_READ,

    // Size Sentinel
    PINPUT_MAX
};

enum _PIUTOOLS_OUTPUT{
    // Cabinet Lights
    POUTPUT_LAMP_BASS,
    POUTPUT_LAMP_R1,
    POUTPUT_LAMP_R2,
    POUTPUT_LAMP_L1,
    POUTPUT_LAMP_L2,    
    
    // System Sensors
    POUTPUT_COIN_COUNTER_P1,
    POUTPUT_COIN_COUNTER_P2,
    POUTPUT_TICKET_P1,
    POUTPUT_TICKET_P2,    
    POUTPUT_NFC_LAMP_P1,
    POUTPUT_NFC_LAMP_P2,
    POUTPUT_NFC_SPEAKER_P1,
    POUTPUT_NFC_SPEAKER_P2,

    // Player 1 - Pad Lights
    POUTPUT_P1_PAD_UL,
    POUTPUT_P1_PAD_UR,
    POUTPUT_P1_PAD_CENTER,
    POUTPUT_P1_PAD_DL,
    POUTPUT_P1_PAD_DR,    

    // Player 1 - ButtonBoard Lights
    POUTPUT_P1_BB_START,
    POUTPUT_P1_BB_BACK,
    POUTPUT_P1_BB_RIGHT,
    POUTPUT_P1_BB_LEFT,
    POUTPUT_P1_BB_UPLEFT,
    POUTPUT_P1_BB_UPRIGHT,

    // Player 2 - Pad Lights
    POUTPUT_P2_PAD_UL,
    POUTPUT_P2_PAD_UR,
    POUTPUT_P2_PAD_CENTER,
    POUTPUT_P2_PAD_DL,
    POUTPUT_P2_PAD_DR,  

    // Player 2 - ButtonBoard Lights
    POUTPUT_P2_BB_START,
    POUTPUT_P2_BB_BACK,
    POUTPUT_P2_BB_RIGHT,
    POUTPUT_P2_BB_LEFT,
    POUTPUT_P2_BB_UPLEFT,
    POUTPUT_P2_BB_UPRIGHT,

    // Size Sentinel
    POUTPUT_MAX 
};

extern unsigned char PIUTools_IO_OUT[POUTPUT_MAX];
extern unsigned char PIUTools_IO_IN[PINPUT_MAX];

void PIUTools_Input_Reset(void);