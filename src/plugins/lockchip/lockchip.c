// Plugin to Support the ZNSEC Lockchip (CAT702)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <sys/io.h>
#include <sys/mman.h>

#include <PIUTools_SDK.h>

#include "cat702.h"

// Signal Handler for CAT702
#define IOPORT_CAT702_IN          0x2AA
#define IOPORT_AT93C86_CAT702_OUT 0x2AE

struct sigaction old_action_sigill;
struct sigaction old_action_sigsegv;

void cat702_handler(int signum, siginfo_t *info, void* ctx){      
    ucontext_t* context = (ucontext_t*)ctx;
    switch(context->uc_mcontext.gregs[REG_EDX] & 0xFFFF){
        case IOPORT_CAT702_IN:
            context->uc_mcontext.gregs[REG_EAX] = CAT702_HandleInput();
            context->uc_mcontext.gregs[REG_EIP]+=2;
            return;            
        case IOPORT_AT93C86_CAT702_OUT:            
            if ((context->uc_mcontext.gregs[REG_EAX] & 0xFF00)) {
                CAT702_HandleOutput(context->uc_mcontext.gregs[REG_EAX] & 0xFFFF);
                context->uc_mcontext.gregs[REG_EIP]+=2;
                return;                    
            }
            break;
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

unsigned char cat702_key[8] = {0x00};
static char cat702_strkey[32] = {0x00};
static unsigned char HexChar(char c) {
	if ('0' <= c && c <= '9') return (unsigned char)(c - '0');
	if ('A' <= c && c <= 'F') return (unsigned char)(c - 'A' + 10);
	if ('a' <= c && c <= 'f') return (unsigned char)(c - 'a' + 10);
	return 0xFF;
}

static int HexToBin(const char* s, unsigned char* buff, int length) {
	int result;
	if (!s || !buff || length <= 0) return -1;

	for (result = 0; *s; ++result)
	{
		unsigned char msn = HexChar(*s++);
		if (msn == 0xFF) return -1;
		unsigned char lsn = HexChar(*s++);
		if (lsn == 0xFF) return -1;
		unsigned char bin = (msn << 4) + lsn;

		if (length-- <= 0) return -1;
		*buff++ = bin;
	}
	return result;
}

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("LOCKCHIP","key",CONFIG_TYPE_STRING,cat702_strkey,sizeof(cat702_strkey)),
  {}
};


const PHookEntry plugin_init(void) { 
    PIUTools_Config_Read(plugin_config);
    DBG_printf("[%s] Loaded CAT702 Key: %s", __FILE__, cat702_strkey);           
    HexToBin(cat702_strkey,cat702_key,8); 
    CAT702_Initialize_Key(cat702_key);
    ioperm(IOPORT_CAT702_IN, 1, 0);
    ioperm(IOPORT_AT93C86_CAT702_OUT, 1, 0);

    struct sigaction action;
    action.sa_sigaction = cat702_handler;
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