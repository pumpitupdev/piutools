// An X11 Keyboard Input Plugin for PIUTools
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>


// X11 includes
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <PIUTools_SDK.h>


int (*next_XGrabKeyboard)(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time);

int block_XGrabKeyboard(Display *display, Window grab_window, Bool owner_events,int pointer_mode, int keyboard_mode, Time time){
    return 0;
}

int (*next_XNextEvent)(Display *display, XEvent *event_return);

int x11ki_XNextEvent(Display *display, XEvent *event){	
	// Load the original function
	int nRet = next_XNextEvent(display, event);
	int ksym;


	// Do event acording to us
        if (event->type == KeyPress || event->type == KeyRelease){
            ksym = XLookupKeysym(&event->xkey, 0);   
            // TODO: Remove This      
            if(ksym == XK_F12){
                printf("[%s] User Exited.\n",__FUNCTION__);
                exit(0);
            }
            switch(ksym){            
                // System Switches
                case XK_F12:
                    PIUTools_IO_IN[PINPUT_APP_EXIT] = (event->type == KeyPress) ? 1 : 0;
                    break; 
                case XK_1:
                    PIUTools_IO_IN[PINPUT_TEST_SWITCH] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_2:
                    PIUTools_IO_IN[PINPUT_SERVICE_SWITCH] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_3:
                    PIUTools_IO_IN[PINPUT_CLEAR_SWITCH] = (event->type == KeyPress) ? 1:0;
                    break;
                case XK_5:
                    PIUTools_IO_IN[PINPUT_COIN_1] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_6:
                    PIUTools_IO_IN[PINPUT_COIN_2] = (event->type == KeyPress) ? 1 : 0;
                    break;        
                // USB IN Will Be a Toggle                              
                case XK_7:
                    PIUTools_IO_IN[PINPUT_P1_USB_IN] = (event->type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_8:
                    PIUTools_IO_IN[PINPUT_P2_USB_IN] = (event->type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_9:
                    PIUTools_IO_IN[PINPUT_P1_NFC_READ] = (event->type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_0:
                    PIUTools_IO_IN[PINPUT_P2_NFC_READ] = (event->type == KeyPress) ? 1 : 0;
                    break;                                                                                                                      
                // Player 1 - Pad
                case XK_q:
                    PIUTools_IO_IN[PINPUT_P1_PAD_UL] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P1_BB_BACK] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_e:
                    PIUTools_IO_IN[PINPUT_P1_PAD_UR] = (event->type == KeyPress) ? 1 : 0;
                    break;                    
                case XK_s:
                    PIUTools_IO_IN[PINPUT_P1_PAD_CENTER] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P1_BB_START] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_z:
                    PIUTools_IO_IN[PINPUT_P1_PAD_DL] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P1_BB_LEFT] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_c:
                    PIUTools_IO_IN[PINPUT_P1_PAD_DR] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P1_BB_RIGHT] = (event->type == KeyPress) ? 1 : 0;
                    break;
                // Player 1 - ButtonBoard TODO

                // Player 2 - Pad
                case XK_r:
                    PIUTools_IO_IN[PINPUT_P2_PAD_UL] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P2_BB_BACK] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_y:
                    PIUTools_IO_IN[PINPUT_P2_PAD_UR] = (event->type == KeyPress) ? 1 : 0;
                    break;                    
                case XK_g:
                    PIUTools_IO_IN[PINPUT_P2_PAD_CENTER] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P2_BB_START] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_v:
                    PIUTools_IO_IN[PINPUT_P2_PAD_DL] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P2_BB_LEFT] = (event->type == KeyPress) ? 1 : 0;
                    break;
                case XK_n:
                    PIUTools_IO_IN[PINPUT_P2_PAD_DR] = (event->type == KeyPress) ? 1 : 0;
                    PIUTools_IO_IN[PINPUT_P2_BB_RIGHT] = (event->type == KeyPress) ? 1 : 0;
                    break;    

                // Player 2 - ButtonBoard TODO

                default:
                break;
            }            
        }
	
	return nRet;
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6","XNextEvent", x11ki_XNextEvent, &next_XNextEvent, 1),                      
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6","XGrabKeyboard", block_XGrabKeyboard, &next_XGrabKeyboard, 1),                      
    {}
};

const PHookEntry plugin_init(void){    
    return entries;
}
