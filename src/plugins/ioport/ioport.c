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

#define SIGHAX 0x13370000

/*
Note - This plugin involves messing with the signal handlers so it has to be enabled before the deadlock handler.
*/

struct sigaction next_action;

/* 
  Handler to Detect OUT 0x80,AL write to the Programmable Interrupt Timer (PIT)
  This is used as a timer/delay mechanism which we don't care a whole lot about now.
  As a result, we'll patch the instruction when we find it.
*/
static void pit_patch(int mysignal, siginfo_t *si, void* arg){  
  ucontext_t *context = (ucontext_t *)arg;    
  unsigned int eip_val = context->uc_mcontext.gregs[REG_EIP];
  // -- Handle OUT 0x80,AL -- 
  if((*(unsigned short*)eip_val) == 0x80E6){
      // Change memory protection to allow write access
      unsigned long page_size = 0x1000;
      unsigned long page_start = (unsigned long)eip_val & ~(page_size - 1);
      mprotect((void *)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

      // Patch the two bytes with 0x90 0x90
      *((char *)eip_val) = 0x90;
      *((char *)eip_val + 1) = 0x90;

      // Restore memory protection
      mprotect((void *)page_start, page_size, PROT_READ | PROT_EXEC);

      // Update EIP to skip the patched instruction
      context->uc_mcontext.gregs[REG_EIP]+=2;
  }

  // Call the next_action handler if it is set
  if (next_action.sa_flags & SA_SIGINFO) {
      if (next_action.sa_sigaction != NULL) {
          next_action.sa_sigaction(mysignal, si, arg);
      }
  } else {
      if (next_action.sa_handler != NULL && next_action.sa_handler != SIG_IGN && next_action.sa_handler != SIG_DFL) {
          next_action.sa_handler(mysignal);
      }
  }
}


void IOPort_Init(){
  // We have to make sure the ioports aren't permitted.
  ioperm(0x80,1, 0);
  struct sigaction action;
  action.sa_sigaction = &pit_patch;
  action.sa_flags = SA_SIGINFO;
  // Technically this should be SIGILL but whatever
  if(sigaction(SIGHAX | SIGSEGV,&action,&next_action) == -1){
    sigaction(SIGSEGV,&action,&next_action);
  }
}

const PHookEntry plugin_init(const char* config_path){  
    IOPort_Init();
    DBG_printf("[%s] IOPort Fixes Enabled",__FILE__);    
    return NULL;
}
