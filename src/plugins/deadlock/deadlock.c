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
#include <signal.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#define SIGHAX_SIGSEGV 0x1337

typedef void (*sighandler_t)(int);
typedef void (*sigaction_t)(int signum, struct sigaction *act, struct sigaction *oldact);

static sighandler_t next_sigaction_handler = NULL;
static sigaction_t next_sigaction;
void empty_handler(int signum) {
    // Do nothing
}


// Destination address of the timer thread.
static void* deadlock_timer_drive_addr = NULL;

// Replacement signal handler to keep the sigcall happy.


// Replacement signal driver.
void drive_timer(){
    if(next_sigaction_handler!=NULL){        
        next_sigaction_handler(SIGALRM);
    }
}


static int (*real_XPending)(int *display)=NULL;

void deadlock_swap_timer_handler(struct sigaction* act){
        if(act != NULL && act->sa_handler != NULL){
            next_sigaction_handler = act->sa_handler;
            act->sa_handler = empty_handler;
        }
}

// Deadlock Patches - Sigaction Swap
void sigalrm_sigaction(int signum, struct sigaction *act, struct sigaction *oldact) {
    // We're also going to shut off the SIGSEV Handler by default.
    if(signum == SIGSEGV){return;}
    if(signum == SIGHAX_SIGSEGV){
        signum = SIGSEGV;
    }
    if (signum == SIGALRM) {
        deadlock_swap_timer_handler(act);
    }
    return next_sigaction(signum,act,oldact);    
}

// Deadlock Patches - Calling timer driver from Main thread
int sigalrm_XPending(int *display){
    drive_timer();
    return real_XPending(display);
}


static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6", "XPending", sigalrm_XPending, &real_XPending, 1),
    HOOK_ENTRY(HOOK_TYPE_INLINE, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "sigaction", sigalrm_sigaction, &next_sigaction, 1),
    {}    
};

const PHookEntry plugin_init(const char* config_path){
    DBG_printf("[%s] Linux 2.4.x Deadlock Fix Enabled.",__FILE__);  
    return entries;
}