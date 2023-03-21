// Plugin to Support the AM030614 IO Board
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

#include "am030614.h"

// Signal Handler for CAT702
#define IOPORT_PIUIO_P1_OUT       0x2A0
#define IOPORT_PIUIO_P2_OUT       0x2A2
#define IOPORT_PIUIO_P1_IN        0x2A4
#define IOPORT_PIUIO_P2_IN        0x2A6

struct sigaction old_action_sigill;
struct sigaction old_action_sigsegv;

void am030614_handler(int signum, siginfo_t *info, void* ctx){      

    ucontext_t* context = (ucontext_t*)ctx;
    switch(context->uc_mcontext.gregs[REG_EDX] & 0xFFFF){
          case IOPORT_PIUIO_P1_OUT:
              PIUIO_HandleOutput_1(context->uc_mcontext.gregs[REG_EAX] & 0xFFFF);
              context->uc_mcontext.gregs[REG_EIP]+=2;
              return;
          case IOPORT_PIUIO_P2_OUT:
              PIUIO_HandleOutput_2(context->uc_mcontext.gregs[REG_EAX] & 0xFFFF);
              context->uc_mcontext.gregs[REG_EIP]+=2;
              return;   
          case IOPORT_PIUIO_P1_IN:
              context->uc_mcontext.gregs[REG_EAX] = PIUIO_HandleInput_1();
              context->uc_mcontext.gregs[REG_EIP]+=2;
              return;          
          case IOPORT_PIUIO_P2_IN:
              context->uc_mcontext.gregs[REG_EAX] = PIUIO_HandleInput_2();
              context->uc_mcontext.gregs[REG_EIP]+=2;
              return;  
        default:
            break;
    }
 
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

static char eeprom_path[1024] = {0x00};

void hex_string_to_8_byte_array(const char *hex_string, unsigned char *byte_array) {
    if (!hex_string || !byte_array) {
        return;
    }

    size_t hex_length = strlen(hex_string);
    size_t num_bytes = (hex_length > 16) ? 8 : (hex_length + 1) / 2;

    for (size_t i = 0; i < num_bytes; ++i) {
        sscanf(hex_string + 2 * i, "%2hhx", &byte_array[i]);
    }
}

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if (strcmp(section, "IO_AM030614") == 0) {
        if (value == NULL) {
            return 0;
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){  
    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    PIUIO_Init();
    ioperm(IOPORT_PIUIO_P1_OUT, 1, 0);
    ioperm(IOPORT_PIUIO_P2_OUT, 1, 0);
    ioperm(IOPORT_PIUIO_P1_IN, 1, 0);
    ioperm(IOPORT_PIUIO_P2_IN, 1, 0);
    struct sigaction action;
    action.sa_sigaction = am030614_handler;
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
