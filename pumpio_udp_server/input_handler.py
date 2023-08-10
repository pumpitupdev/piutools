# MetaHandler for All Pump Inputs
import struct
import win_keyboard
import hid_controller
from enum import Enum

hid_controller.init()

class PIUInput(Enum):

    # System Helpers 
    APP_EXIT = 0

    # System Switches
    SERVICE_SWITCH = 1
    TEST_SWITCH = 2
    CLEAR_SWITCH = 3  
    COIN_1 = 4
    COIN_2 = 5

    # Player 1 Controls - Pad 
    P1_PAD_UL = 6
    P1_PAD_UR = 7    
    P1_PAD_CENTER = 8
    P1_PAD_DL = 9
    P1_PAD_DR = 10

    # Player 1 Controls - Button Board
    P1_BB_START = 11
    P1_BB_BACK = 12
    P1_BB_RIGHT = 13
    P1_BB_LEFT = 14  
    P1_BB_UPLEFT = 15
    P1_BB_UPRIGHT = 16

    # Player 1 Controls - Profile
    P1_USB_IN = 17
    P1_NFC_READ = 18

    # Player 2 Controls - Pad
    P2_PAD_UL = 19
    P2_PAD_UR = 20
    P2_PAD_CENTER = 21
    P2_PAD_DL = 22
    P2_PAD_DR = 23

    # Player 2 Controls - Button Board 
    P2_BB_START = 24
    P2_BB_BACK = 25
    P2_BB_RIGHT = 26
    P2_BB_LEFT = 27
    P2_BB_UPLEFT = 28
    P2_BB_UPRIGHT = 29

    # Player 2 Controls - Profile
    P2_USB_IN = 30
    P2_NFC_READ = 31

    # Sentinel 
    MAX = 32

INPUT_STATE = [
    # System Switches
    {'input':PIUInput.APP_EXIT.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_F12},
    {'input':PIUInput.TEST_SWITCH.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_1},
    {'input':PIUInput.SERVICE_SWITCH.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_2},    
    {'input':PIUInput.CLEAR_SWITCH.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_3},
    {'input':PIUInput.COIN_1.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_4},
    {'input':PIUInput.COIN_2.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_5},
    # Player 1 Switches    
    #{'input':PIUInput.P1_PAD_UL.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_Q},
    #{'input':PIUInput.P1_PAD_UR.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_E},
    #{'input':PIUInput.P1_PAD_CENTER.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_S},
    #{'input':PIUInput.P1_PAD_DL.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_Z},
    #{'input':PIUInput.P1_PAD_DR.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_C},
    {'input':PIUInput.P1_PAD_UL.value,'device_handler':hid_controller.is_button_down,'device_value':hid_controller.BUTTON_0},
    {'input':PIUInput.P1_PAD_UR.value,'device_handler':hid_controller.is_button_down,'device_value':hid_controller.BUTTON_1},
    {'input':PIUInput.P1_PAD_CENTER.value,'device_handler':hid_controller.is_button_down,'device_value':hid_controller.BUTTON_4},
    {'input':PIUInput.P1_PAD_DL.value,'device_handler':hid_controller.is_button_down,'device_value':hid_controller.BUTTON_2},
    {'input':PIUInput.P1_PAD_DR.value,'device_handler':hid_controller.is_button_down,'device_value':hid_controller.BUTTON_3},    
    # Player 1 BB -- TODO

    # Player 1 Profile 
    {'input':PIUInput.P1_USB_IN.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_7},
    {'input':PIUInput.P1_NFC_READ.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_9},

    # Player 2 Switches
    {'input':PIUInput.P2_PAD_UL.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_R},
    {'input':PIUInput.P2_PAD_UR.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_Y},
    {'input':PIUInput.P2_PAD_CENTER.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_G},
    {'input':PIUInput.P2_PAD_DL.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_V},
    {'input':PIUInput.P2_PAD_DR.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_N},
    # Player 2 BB -- TODO

    # Player 2 Profile 
    {'input':PIUInput.P2_USB_IN.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_8},
    {'input':PIUInput.P2_NFC_READ.value,'device_handler':win_keyboard.is_key_down,'device_value':win_keyboard.KEY_0},    
]

def get_input_state():
    state = 0
    for ck in INPUT_STATE:
        state |= ck['device_handler'](ck['device_value']) << ck['input']
    return struct.pack("<I",state)


