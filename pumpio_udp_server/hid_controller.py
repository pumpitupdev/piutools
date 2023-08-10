import hid
import threading
VID = 0x03eb
PID = 0x8041
HID_ENABLED = True

"""
L-TEK Pad Layout
byte[1]
0 - UL
2 - UR
4 - DL
8 - DR
16 - CENTER
"""

BUTTON_0 = 0
BUTTON_1 = 1
BUTTON_2 = 2
BUTTON_3 = 3
BUTTON_4 = 4

BUTTON_STATES = 0

def open_hid(vid,pid):
    h = hid.device()
    try:
        h.open(vid,pid)
    except:
        return None
    h.set_nonblocking(1)
    return h

def process_hid_input():
    global BUTTON_STATES
    hhid = open_hid(VID,PID)
    while True:
        data = hhid.read(64)
        if data:
            # Extract button states
            BUTTON_STATES = data[1]                          
    hhid.close()

def is_button_down(button_val):
    global BUTTON_STATES
    return (BUTTON_STATES >> button_val) & 1


def init():
    threading.Thread(target=process_hid_input, daemon=True).start()