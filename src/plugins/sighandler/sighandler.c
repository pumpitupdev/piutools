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
#include <unistd.h>

// For Signal Stuff
#include <signal.h>

// For Backtrace Stuff
#include <execinfo.h>

// For XPending
#include <X11/Xlib.h>

#include <PIUTools_SDK.h>




typedef int (*XPending_t)(Display *);
typedef void (*sigaction_t)(int signum, struct sigaction *act, struct sigaction *oldact);
typedef void (*sighandler_t)(int);

static XPending_t next_XPending;
static sigaction_t next_sigaction;

static sighandler_t timer_driver = NULL;
static int better_backtrace = 0;
static int deadlock_fix = 0;


// Replacement signal handler to keep the sigcall happy.
static void empty_sigalrm_handler(int mysignal, siginfo_t *si, void* arg){}


static void better_segfault_backtrace(int signum, siginfo_t *info, void *p) {
    printf(
        "signum: %d\n"
        "si_signo: %d\n"
        "si_errno: %d\n"
        "si_code: %d\n"
        "si_value: %08X\n"
        "si_addr: %p\n"
        "p: %p\n",
        signum, info->si_signo, info->si_errno, info->si_code, info->si_value.sival_int, info->si_addr,p);

    printf("Backtrace: \n");
    void *buffer[50];
    int size = backtrace(buffer, 50);
    printf("Received signal %d (%s)\n", signum, strsignal(signum));
    printf("Fault address: %p\n", info->si_addr);
    backtrace_symbols_fd(buffer, size, STDERR_FILENO);
    exit(1);
}

// Deadlock Patches - Sigaction Swap
void deadlock_sigalrm(int signum, struct sigaction *act, struct sigaction *oldact) {
    // Block SIGALRM Calls after replacing our timer driver.
    if (signum == SIGALRM && deadlock_fix) {
        if(timer_driver == NULL){
            if(act != NULL && act->sa_handler != NULL){
                timer_driver = act->sa_handler;
                act->sa_handler = (sighandler_t)empty_sigalrm_handler;
            }
        }
    }
    // Block SIGSEGV
    if(signum == SIGSEGV){
        
        //if(better_backtrace == 0){return;}
        if(better_backtrace){
            act->sa_handler=(sighandler_t)better_segfault_backtrace;
        }else{
            return;
        }    
    }
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
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libX11.so.6", "XPending", deadlock_XPending, &next_XPending, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libc.so.6", "sigaction", deadlock_sigalrm, &next_sigaction, 0),
    {}    
};


static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("SIGHANDLER","sigalrm_deadlock_fix",CONFIG_TYPE_BOOL,&deadlock_fix,sizeof(deadlock_fix)),
  CONFIG_ENTRY("SIGHANDLER","better_backtrace",CONFIG_TYPE_BOOL,&better_backtrace,sizeof(better_backtrace)),
  {}
};

const PHookEntry plugin_init(void) {
  PIUTools_Config_Read(plugin_config);
    // We'll set up our own handler 
    if(deadlock_fix){
        entries[0].hook_enabled = 1;
        entries[1].hook_enabled = 1;
        DBG_printf("[%s] Linux 2.4.x Deadlock SIGALRM Fix Enabled.",__FILE__);  
    }
    if(better_backtrace){
        entries[0].hook_enabled = 1;
        DBG_printf("[%s] Better Backtrace Enabled.",__FILE__);  
    }
    return entries;
}

