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
}

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"LOCALE") == 0){
        if(strcmp(name,"locale_fix") == 0){
            if(value == NULL){return 0;}
            char *ptr; 
            unsigned long lv = strtoul(value,&ptr,10);
            if(lv == 1){
                hook_core_piu_utils_fix_locale();
            }                       
        }                 
    }
    return 1;
}

int plugin_init(const char* config_path, PHookEntry *hook_entry_table){
    ini_parse(config_path,parse_config,NULL);
    return 0;
}
