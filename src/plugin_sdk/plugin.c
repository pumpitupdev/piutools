#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "ini.h"
#include "PIUTools_Input.h"
#include "PIUTools_USB.h"
#include "PIUTools_Filesystem.h"

char piutools_game_rom_path[1024] = {0x00};
char piutools_game_version_path[1024] = {0x00};
char piutools_game_config_path[1024] = {0x00};
char piutools_game_save_path[1024] = {0x00};
char piutools_plugin_path[1024] = {0x00};
char piutools_root_path[1024] = {0x00};

// Path resolution for tool-specific wildcards and paths.
#define GAME_ROM_PATH_TAG "${GAME_ROM_PATH}"
#define GAME_ROM_VERSION_PATH_TAG "${GAME_ROM_VERSION_PATH}"
#define SAVE_ROOT_PATH_TAG "${SAVE_ROOT_PATH}"

void piutools_resolve_path(const char* in_path, char* out_path){
    char tmp_path[1024] = {0x00};
    // Handle GAME_ROM_PATH
    if(strncmp(in_path,GAME_ROM_PATH_TAG,strlen(GAME_ROM_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",piutools_game_rom_path,in_path+strlen(GAME_ROM_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }
    // Handle Game ROM Version Path
    if(strncmp(in_path,GAME_ROM_VERSION_PATH_TAG,strlen(GAME_ROM_VERSION_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",piutools_game_version_path,in_path+strlen(GAME_ROM_VERSION_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }   
    // Handle Save Root Path
    if(strncmp(in_path,SAVE_ROOT_PATH_TAG,strlen(SAVE_ROOT_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",piutools_game_save_path,in_path+strlen(SAVE_ROOT_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }  
    if(in_path[0] == '.'){
        realpath(in_path,tmp_path);
        strcpy(out_path,tmp_path);
        return;
    }      
}

void piutools_get_plugin_path(const char* plugin_name, char* plugin_path){
    sprintf(plugin_path,"%s/%s.plugin",piutools_plugin_path,plugin_name);
}

void piutools_init_sdk(void){
    

    sprintf(piutools_root_path,"%s",getenv("PIUTOOLS_PATH"));
    sprintf(piutools_game_rom_path,"%s/%s",getenv("PIUTOOLS_ROM_PATH"),getenv("PIUTOOLS_GAME_NAME"));
    sprintf(piutools_game_version_path,"%s/version/%s",piutools_game_rom_path,getenv("PIUTOOLS_GAME_VERSION"));
    sprintf(piutools_game_config_path,"%s/%s/%s.conf",getenv("PIUTOOLS_CONFIG_PATH"),getenv("PIUTOOLS_GAME_NAME"),getenv("PIUTOOLS_GAME_VERSION"));
    sprintf(piutools_game_save_path,"%s/%s/%s",getenv("PIUTOOLS_SAVE_PATH"),getenv("PIUTOOLS_GAME_NAME"),getenv("PIUTOOLS_GAME_VERSION"));
    sprintf(piutools_plugin_path,"%s",getenv("PIUTOOLS_PLUGIN_PATH"));
    // Check SDK Paths
    int fail_init = 0;

    if(strlen(piutools_root_path) < 1){fail_init = 1;}
    if(strlen(piutools_game_rom_path) < 1){fail_init = 1;}
    if(strlen(piutools_game_version_path) < 1){fail_init = 1;}
    if(strlen(piutools_game_config_path) < 1){fail_init = 1;}
    if(strlen(piutools_game_save_path) < 1){fail_init = 1;}
    if(strlen(piutools_plugin_path) < 1){fail_init = 1;}

    if(fail_init){
        printf("Failed to Resolve One of the Paths\n");
        printf("Root Path: %s\n",piutools_root_path);
        printf("Game Rom Path: %s\n",piutools_game_rom_path);
        printf("Game Version Path: %s\n",piutools_game_version_path);
        printf("Game Config Path: %s\n",piutools_game_config_path);
        printf("Game Save Path: %s\n",piutools_game_save_path);
        printf("Plugin Path: %s\n",piutools_plugin_path);        
        exit(-1);
    }
    // Set up Initial PIUTools Config
    PIUTools_Filesystem_Init();
    PIUTools_Input_Reset();
    PIUTools_USB_Init(piutools_game_save_path);
}