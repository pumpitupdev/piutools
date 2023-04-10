// Settings that Have to Do With Locale Fixes


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>


void hook_core_piu_utils_fix_locale()
{
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
}


const PHookEntry plugin_init(const char* config_path){  
    hook_core_piu_utils_fix_locale();
    return NULL;
}
