#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <PIUTools_SDK.h>

#include "patch.h"
#include "mem_patch.h"
#include "old_hasp.h"
static const uint8_t patch_hasp_login_sig[] = {
    0x00,
    0xB8,
    0x16,
    0x00,
    0x00,
    0x00,
    0x8B,
    0x4C,
    0x24,
    0x14,
    0x8B,
    0x74,
    0x24,
    0x10,
    0x8B,
    0x54};

static const uint8_t patch_hasp_id_sig[] = {
    0x83, 0xC4, 0x18, 0x5B, 0xC3, 0x8D, 0x74, 0x26, 0x00, 0x8D, 0xBC, 0x27, 0x00, 0x00, 0x00, 0x00, 
	0x55, 0x57, 0x56, 0x53, 0x83, 0xEC, 0x3C, 0xE8};


void patch_hasp_init(const uint8_t *key_data, size_t len, char* str_fids){
  void *func_api_login;
  void *func_api_logout;
  void *func_api_decrypt;
  void *func_api_getinfo;
    
  func_api_login = util_patch_find_signiture(
      patch_hasp_login_sig,
      sizeof(patch_hasp_login_sig),
      -16,
      (void *) 0x80a0000,
      (void *) 0x80f0000,
      16);

  if (!func_api_login) {
    DBG_printf("Could not find ApiLogin address");
    return;
  }

  DBG_printf("ApiLogin at %p\n", func_api_login);

  func_api_logout = (void *) (((size_t) func_api_login) - 0x130);
  DBG_printf("assuming ApiLogout at %p\n", func_api_logout);

  func_api_decrypt = (void *) (((size_t) func_api_login) - 0x200);
  DBG_printf("assuming ApiDecrypt at %p\n", func_api_decrypt);

  func_api_getinfo = (void *) (((size_t) func_api_login) - 0x15F0);

  if (!func_api_getinfo) {
    DBG_printf("Could not find ApiGetinfo address");
    return;
  }

  DBG_printf("ApiGetid at %p\n", func_api_getinfo);

  util_patch_function((uintptr_t) func_api_login, sec_hasp_api_login);
  util_patch_function((uintptr_t) func_api_logout, sec_hasp_api_logout);
  util_patch_function((uintptr_t) func_api_decrypt, sec_hasp_api_decrypt);
  util_patch_function((uintptr_t) func_api_getinfo, sec_hasp_get_info);

  sec_hasp_init(key_data, len,str_fids);
}

static char dongle_file_path[1024] = {0x00};
static char hasp_fids[128] = {0x00};

static HookConfigEntry plugin_config[] = {
  CONFIG_ENTRY("HASP","feature_ids",CONFIG_TYPE_STRING,hasp_fids,sizeof(hasp_fids)),
  {}
};

const PHookEntry plugin_init(void){
  PIUTools_Config_Read(plugin_config);
  PIUTools_Path_Resolve("${GAME_ROM_PATH}/hasp.dongle",dongle_file_path);

  // Read the HASP Dongle File   
  FILE* fp = fopen(dongle_file_path,"rb");
  if(fp == NULL){return NULL;}
  fseek(fp,0,SEEK_END);
  size_t hasp_file_size = ftell(fp);
  rewind(fp);
  unsigned char* hasp_data = malloc(hasp_file_size);
  fread(hasp_data,hasp_file_size,1,fp);
  fclose(fp);

  // Initialize The Emulator
  DBG_printf("[%s] HASP Dongle File Loaded: %s FIDS: %s",__FILE__,dongle_file_path,hasp_fids);      
  patch_hasp_init(hasp_data,hasp_file_size,hasp_fids);

  return NULL;
}
