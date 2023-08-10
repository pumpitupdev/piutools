# Handler Module for all Outputs (e.g. Lights, speaker, etc.)
import struct
from enum import Enum

class PIUOutput(Enum):

    # Cabinet Lights
    LAMP_BASS = 0
    LAMP_R1 = 1
    LAMP_R2 = 2
    LAMP_L1 = 3
    LAMP_L2 = 4

    # System Sensors
    COIN_COUNTER_P1 = 5
    COIN_COUNTER_P2 = 6
    TICKET_P1 = 7
    TICKET_P2 = 8
    NFC_LAMP_P1 = 9
    NFC_LAMP_P2 = 10
    NFC_SPEAKER_P1 = 11 
    NFC_SPEAKER_P2 = 12

    # Player 1 - Pad Lights  
    P1_PAD_UL = 13
    P1_PAD_UR = 14
    P1_PAD_CENTER = 15
    P1_PAD_DL = 16
    P1_PAD_DR = 17

    # Player 1 - Button Board Lights
    P1_BB_START = 18
    P1_BB_BACK = 19
    P1_BB_RIGHT = 20
    P1_BB_LEFT = 21
    P1_BB_UPLEFT = 22
    P1_BB_UPRIGHT = 23

    # Player 2 - Pad Lights
    P2_PAD_UL = 24
    P2_PAD_UR = 25
    P2_PAD_CENTER = 26
    P2_PAD_DL = 27  
    P2_PAD_DR = 28

    # Player 2 - Button Board Lights
    P2_BB_START = 29
    P2_BB_BACK = 30
    P2_BB_RIGHT = 31
    P2_BB_LEFT = 32
    P2_BB_UPLEFT = 33
    P2_BB_UPRIGHT = 34

    # Sentinel
    MAX = 35


def process_output(data):
    value = struct.unpack("<Q",data)[0]
    #print("New Output: %08X" % value)
    pass