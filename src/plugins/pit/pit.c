#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/io.h>
#include <sys/mman.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


#define IOPORT_SIGDRIVER_OUT 0x80


struct sigaction old_action_sigill;
struct sigaction old_action_sigsegv;

/* 
  Handler to Detect OUT 0x80,AL write to the Programmable Interrupt Timer (PIT)
  This is used as a timer/delay mechanism which we don't care a whole lot about now.
  As a result, we'll patch the instruction when we find it.
*/
static void pit_patch(int signum, siginfo_t *info, void* ctx){  
  ucontext_t *context = (ucontext_t *)ctx;    
  unsigned char* eip_data = (unsigned char*)context->uc_mcontext.gregs[REG_EIP];
  if(eip_data[0] == 0xE6 && eip_data[1] == 0x80){
    context->uc_mcontext.gregs[REG_EIP]+=2;
    return;
  }
  if(eip_data[0] == 0xEE && (context->uc_mcontext.gregs[REG_EDX] & 0xFF) == IOPORT_SIGDRIVER_OUT){
    context->uc_mcontext.gregs[REG_EIP]++;
    return;    
  }
  //printf("EIP: %04X\n",context->uc_mcontext.gregs[REG_EIP]);
  struct sigaction* next_action = (signum == SIGILL) ? &old_action_sigill : &old_action_sigsegv;

  if (next_action->sa_handler != NULL && next_action->sa_handler != SIG_IGN) {
      if (next_action->sa_flags & SA_SIGINFO) {
          // Call the old signal handler with three arguments
          next_action->sa_sigaction(signum, info, ctx);
      } else {
          // Call the old signal handler with one argument
          next_action->sa_handler(signum);
      }
  }
}

const PHookEntry plugin_init(const char* config_path){  
    DBG_printf("[%s] IO PIT Patch Enabled",__FILE__);   
    ioperm(0x80,1, 0);
    struct sigaction action;
    action.sa_sigaction = pit_patch;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    
    if(sigaction(SIGHAX | SIGSEGV,&action,&old_action_sigsegv) == -1){
        sigaction(SIGSEGV,&action,&old_action_sigsegv);
    }
    if(sigaction(SIGHAX | SIGILL,&action,&old_action_sigill) == -1){
        sigaction(SIGILL,&action,&old_action_sigill);
    }    
    return NULL;     
}
