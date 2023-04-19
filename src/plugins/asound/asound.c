// Alsa Sound Handler Patches
#include <alsa/asoundlib.h>

#include <PIUTools_SDK.h>

typedef int (*snd_pcm_open_t)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
static snd_pcm_open_t next_snd_pcm_open;

typedef int (*snd_pcm_drop_t)(snd_pcm_t *pcm);
static snd_pcm_drop_t next_snd_pcm_drop;

static char patch_sound_dev_name[256];
static int pcm_drop_fix=0;

static int asound2_snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode) {
    int ret = 0;
    int retry_count = 5;
    // Replace the Sound Device Name
    if (strlen(patch_sound_dev_name) > 1) {
        name = patch_sound_dev_name;
    }

    /*
    Notes about exceed and how it messed up using ALSA:
    - Opening the same (hw:0) device multiple times without closing
    - Opening and closing the device on each effect play
    - No proper error handling on snd_pcm_open return

    This was their first foray into linux sound engines and based on beta source we've seen,
    they had a few misfires before even getting here.
    */

    /* Using dmix instead of hw:0 on exceed, snd_pcm_open sometimes returns
       with a bad fd but after another retry, everything's fine */
    while (retry_count > 0) {
        ret = next_snd_pcm_open(pcmp, name, stream, mode);

        if (ret < 0) {
            retry_count--;
        } else {
            break;
        }
    }

    if (ret < 0) {
        DBG_printf("[%s] snd_pcm_open failed", __FILE__);
    }

    return ret;
}

/*
  Exceed and Exceed 2 call snd_pcm_drop in StopAllEffects which is not used in later games.
  This (kinda obviously) kills all the sound effect channels, but doesn't reuse them.
  We'll disable this in those cases...
*/
static int asound2_snd_pcm_drop(snd_pcm_t *pcm) {return 0;}

static HookEntry entries[] = {
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libasound.so.2", "snd_pcm_open", asound2_snd_pcm_open, &next_snd_pcm_open, 0),
    HOOK_ENTRY(HOOK_TYPE_IMPORT, HOOK_TARGET_BASE_EXECUTABLE, "libasound.so.2", "snd_pcm_drop", asound2_snd_pcm_drop, &next_snd_pcm_drop, 0),
    {}
};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("ASOUND","device_name",CONFIG_TYPE_STRING,patch_sound_dev_name,sizeof(patch_sound_dev_name)),
  CONFIG_ENTRY("ASOUND","pcm_drop_fix",CONFIG_TYPE_BOOL,&pcm_drop_fix,sizeof(pcm_drop_fix)),
  {}
};

const PHookEntry plugin_init(void) {
  PIUTools_Config_Read(plugin_config);
  // If we didn't specify a device name, default to pulse.
  if (strlen(patch_sound_dev_name) < 1) {
    strcpy(patch_sound_dev_name, "pulse");
  }

  DBG_printf("[%s] Custom Device Name: %s", __FILE__, patch_sound_dev_name);
  entries[0].hook_enabled = 1;

  if(pcm_drop_fix){
    DBG_printf("[%s] PCM Drop Fix Enabled", __FILE__);
    entries[1].hook_enabled = 1;
  }
  return entries;
}