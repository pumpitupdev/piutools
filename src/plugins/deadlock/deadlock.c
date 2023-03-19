/*
 * Deadlock Patch for Exceed->NX (Linux Kernel > 2.4.27)
 * Bunch of stuff, SIGALRM might get double-called where they
 * have it, moved it to XPending in main() to fix it.
 * Tried giving it its own thread, game runs too fast.
 * Description: 
 * AM decided to poll the CPIU::Update function which contains updating
 * the render state as well as decoding sound data, using the signal SIGALRM.
 * This signal is triggered periodically and drives the whole engine.
 * But, starting kernel versions newer than 2.4.27 something changed which
 * causes the game to deadlock an _ALL_ kernels newer than said version. Trying
 * to fix the deadlock issue makes things even worse and only introduces
 * segfaults with random crashes. So this is not the right approach.
 * Starting NX2 AM used a newer kernel and apparently got the same problem
 * with their OS. They fixed it by moving music_send_data from CPIU::Update()
 * to the IO thread. Splitting CPIU::Update that way, this seems to work fine.
 * However, trying this with the old games lacking the patch, we are getting
 * segfaults (again).
 * It looks like there is an issue with firing and/or handling of SIGALRM in
 * another thread than the main thread (don't ask me why). This leads to the
 * solution of the problem: moving the call of CPIU::Update()to the main
 * thread which is only pumping XEvents (i.e. nothing).
 * So let's give the main thread some work: Detouring XPending and calling
 * the registered signal handler for SIGALRM of the game in there and muting
 * signal handling for SIGALRM. This solves all deadlocking and segfault
 * issues which are related to audio playback in combination with decoding a
 * video.
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// For Signal Stuff
#include <signal.h>

// For XPending
#include <X11/Xlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>





typedef int (*XPending_t)(Display *);
typedef void (*sigaction_t)(int signum, struct sigaction *act, struct sigaction *oldact);
typedef void (*sighandler_t)(int);

static XPending_t next_XPending;
static sigaction_t next_sigaction;

static sighandler_t timer_driver = NULL;


// Replacement signal handler to keep the sigcall happy.
static void empty_sigalrm_handler(int mysignal, siginfo_t *si, void* arg){}

// Deadlock Patches - Sigaction Swap
void deadlock_sigalrm(int signum, struct sigaction *act, struct sigaction *oldact) {
    // Block SIGALRM Calls after replacing our timer driver.
    if (signum == SIGALRM) {
        if(timer_driver == NULL){
            if(act != NULL && act->sa_handler != NULL){
                timer_driver = act->sa_handler;
                act->sa_handler = (sighandler_t)empty_sigalrm_handler;
            }
        }
    }
    // Block SIGSEGV
    if(signum == SIGSEGV){return;}
    if(signum == (SIGHAX | SIGSEGV)){
        signum = SIGSEGV;
    }
    // Allow a replacement bypass of the empty handler in case we want to use this later.
    if(signum == (SIGHAX | SIGALRM)){signum = SIGALRM;}    
    return next_sigaction(signum,act,oldact);    
}

// Deadlock Patches - Calling timer driver from Main thread
int deadlock_XPending(Display *dpy){
    if(timer_driver!=NULL){timer_driver(SIGALRM);}
    return next_XPending(dpy);
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6", "XPending", deadlock_XPending, &next_XPending, 1),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "sigaction", deadlock_sigalrm, &next_sigaction, 1),
    {}    
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"DEADLOCK") == 0){
        if(strcmp(name,"sigalrm_fix") == 0){
            char *ptr;            
            if(value != NULL){
              entries[0].hook_enabled = (strtoul(value,&ptr,10) == 1);
              entries[1].hook_enabled = entries[0].hook_enabled;
            }
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    // We'll set up our own handler 
    if(entries[0].hook_enabled){    
        DBG_printf("[%s] Linux 2.4.x Deadlock SIGALRM Fix Enabled.",__FILE__);  
    }
    return entries;
}