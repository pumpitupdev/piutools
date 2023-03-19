// Alsa Sound Handler Patches
#include <stddef.h>
#include <alsa/asoundlib.h>

#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>

typedef int (*snd_pcm_open_t)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
static snd_pcm_open_t next_snd_pcm_open;

typedef int (*snd_pcm_drop_t)(snd_pcm_t *pcm);
static snd_pcm_drop_t next_snd_pcm_drop;

static char patch_sound_dev_name[256];


static int asound2_snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode){    
  int ret = 0;
  int retry_count = 5;
  // Replace the Sound Device Name
  if (strlen(patch_sound_dev_name) > 1) {name = patch_sound_dev_name;}

  /*
     Notes about exceed and how it fucked up using alsa:
     - Opening the same (hw:0) device multiple times without closing
     - Opening and closing the device on each effect play
     - No proper error handling on snd_pcm_open return
  */

  /* Using dmix instead of hw:0 on exceed, snd_pcm_open sometimes returns
     with a bad fd but after another retry, everything's fine */ 
  return next_snd_pcm_open(pcmp, name, stream, mode);
}

static int asound2_snd_pcm_drop(snd_pcm_t*pcm){return 0;}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libasound.so.2", "snd_pcm_open", asound2_snd_pcm_open, &next_snd_pcm_open, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libasound.so.2", "snd_pcm_drop", asound2_snd_pcm_drop, &next_snd_pcm_drop, 0),
    {}
};

static int parse_config(void* user, const char* section, const char* name, const char* value){
    if(strcmp(section,"ASOUND") == 0){
        if(strcmp(name,"device_name") == 0){
            if(value == NULL){return 0;}
            strncpy(patch_sound_dev_name,value,sizeof(patch_sound_dev_name));
            DBG_printf("[%s] Custom Device Name: %s",__FILE__, patch_sound_dev_name);
            entries[0].hook_enabled = 1;
        }
        if(strcmp(name,"pcm_drop_fix") == 0){
            char *ptr;            
            if(value != NULL){
              entries[1].hook_enabled = strtoul(value,&ptr,10);
            }
        }
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
  if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
  return entries;
}