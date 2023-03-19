#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/io.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#define SIGHAX_SIGSEGV 0x1337

/*
Note - This plugin involves messing with the signal handlers so it has to be enabled before the deadlock handler.
*/
static void ioport_handler(int mysignal, siginfo_t *si, void* arg){  
  ucontext_t *context = (ucontext_t *)arg;    
  unsigned int eip_val = context->uc_mcontext.gregs[REG_EIP];
  // -- Handle OUT 0x80,AL -- 
  if((*(unsigned short*)eip_val) == 0x80E6){
      context->uc_mcontext.gregs[REG_EIP]+=2;
  }
}


void IOPort_Init(){
  // We have to make sure the ioports aren't permitted.
  ioperm(0x80,1, 0);
  struct sigaction action;
  action.sa_sigaction = &ioport_handler;
  action.sa_flags = SA_SIGINFO;
  // Technically this should be SIGILL but whatever
  sigaction(SIGHAX_SIGSEGV,&action,NULL);    
}

const PHookEntry plugin_init(const char* config_path){  
    IOPort_Init();
    DBG_printf("[%s] IOPort Fixes Enabled",__FILE__);    
    return NULL;
}
