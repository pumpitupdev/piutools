// Settings that Have to Do With Locale Fixes


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <PIUTools_SDK.h>

const PHookEntry plugin_init(void){  
    DBG_printf("[%s:%s] Fixing locale settings.",__FILE__,__FUNCTION__);
    if (setenv("LC_ALL", "C", 1) != 0) {
      DBG_printf("[%s:%s] Setting locale LC_ALL failed.",__FILE__,__FUNCTION__);
    }    
    if (unsetenv("LANGUAGE") != 0) {
      DBG_printf("[%s:%s] Unsetting LANGUAGE env var failed.",__FILE__,__FUNCTION__);
    }
    if (unsetenv("LANG") != 0) {
      DBG_printf("[%s:%s] Unsetting LANGUAGE env var failed.",__FILE__,__FUNCTION__);
    }  
    return NULL;
}
