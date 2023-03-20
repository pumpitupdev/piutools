#ifdef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>



// X11 includes
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "PIUTools_Input.h"
#include "x11_keyio.h"

static pthread_t hthread;


static void *x11_input_loop(void* args){
    int ksym;
    Display *display = NULL;
    XEvent event;

    while(display == NULL){
        display = XOpenDisplay(NULL);
    }
    
    XGrabKeyboard(display, DefaultRootWindow(display),
                 True, GrabModeAsync, GrabModeAsync, CurrentTime);
    while (1){
        XNextEvent(display, &event);
        if (event.type == KeyPress || event.type == KeyRelease){
            ksym = XLookupKeysym(&event.xkey, 0);   
            // TODO: Remove This      
            if(ksym == XK_F12){
                printf("[%s] User Exited.\n",__FUNCTION__);
                exit(0);
            }
    // System Switches

            switch(ksym){            
                // System Switches
                case XK_F12:
                    PIUTools_IO_IN[PINPUT_APP_EXIT] = (event.type == KeyPress) ? 1 : 0;
                    break; 
                case XK_1:
                    PIUTools_IO_IN[PINPUT_TEST_SWITCH] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_2:
                    PIUTools_IO_IN[PINPUT_SERVICE_SWITCH] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_3:
                    PIUTools_IO_IN[PINPUT_CLEAR_SWITCH] = (event.type == KeyPress) ? 1:0;
                    break;
                case XK_5:
                    PIUTools_IO_IN[PINPUT_COIN_1] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_6:
                    PIUTools_IO_IN[PINPUT_COIN_2] = (event.type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_7:
                    PIUTools_IO_IN[PINPUT_P1_USB_IN] = (event.type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_8:
                    PIUTools_IO_IN[PINPUT_P2_USB_IN] = (event.type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_9:
                    PIUTools_IO_IN[PINPUT_P1_NFC_READ] = (event.type == KeyPress) ? 1 : 0;
                    break;                                      
                case XK_0:
                    PIUTools_IO_IN[PINPUT_P2_NFC_READ] = (event.type == KeyPress) ? 1 : 0;
                    break;                                                                                                                      
                // Player 1 - Pad
                case XK_q:
                    PIUTools_IO_IN[PINPUT_P1_PAD_UL] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_e:
                    PIUTools_IO_IN[PINPUT_P1_PAD_UR] = (event.type == KeyPress) ? 1 : 0;
                    break;                    
                case XK_s:
                    PIUTools_IO_IN[PINPUT_P1_PAD_CENTER] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_z:
                    PIUTools_IO_IN[PINPUT_P1_PAD_DL] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_c:
                    PIUTools_IO_IN[PINPUT_P1_PAD_DR] = (event.type == KeyPress) ? 1 : 0;
                    break;
                // Player 1 - ButtonBoard TODO

                // Player 2 - Pad
                case XK_r:
                    PIUTools_IO_IN[PINPUT_P2_PAD_UL] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_y:
                    PIUTools_IO_IN[PINPUT_P2_PAD_UR] = (event.type == KeyPress) ? 1 : 0;
                    break;                    
                case XK_g:
                    PIUTools_IO_IN[PINPUT_P2_PAD_CENTER] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_v:
                    PIUTools_IO_IN[PINPUT_P2_PAD_DL] = (event.type == KeyPress) ? 1 : 0;
                    break;
                case XK_n:
                    PIUTools_IO_IN[PINPUT_P2_PAD_DR] = (event.type == KeyPress) ? 1 : 0;
                    break;    

                // Player 2 - ButtonBoard TODO

                default:
                break;
            }            
        }
    }
}


void X11_KeyIO_Init(const char* path_to_config_file){    
    pthread_create(&hthread, 0, x11_input_loop, NULL);
}