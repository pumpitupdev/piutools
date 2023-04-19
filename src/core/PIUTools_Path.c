// Core Logic for Handling PIUTools Specific Paths
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int module_initialized = 0;

static char PIUTools_Path_Game_ROM[1024] = {0x00};
static char PIUTools_Path_Game_Config[1024] = {0x00};
static char PIUTools_Path_Game_Save[1024] = {0x00};
static char PIUTools_Path_Plugin[1024] = {0x00};
static char PIUTools_Path_Root[1024] = {0x00};
static char PIUTools_Path_Tmp[1024] = {0x00};

// Path resolution for tool-specific wildcards and paths.
#define GAME_ROM_PATH_TAG "${GAME_ROM_PATH}"
#define GAME_ROM_VERSION_PATH_TAG "${GAME_ROM_VERSION_PATH}"
#define GAME_CONFIG_PATH "${GAME_CONFIG_PATH}"
#define SAVE_ROOT_PATH_TAG "${SAVE_ROOT_PATH}"
#define TMP_ROOT_PATH_TAG "${TMP_ROOT_PATH}"
#define PIUTOOLS_ROOT_PATH_TAG "${PIUTOOLS_ROOT_PATH}"
#define PLUGIN_PATH_TAG "${PLUGIN_ROOT_PATH}"


static void init_paths(void){
    if(module_initialized){return;}
    module_initialized=1;
    sprintf(PIUTools_Path_Root,"%s",getenv("PIUTOOLS_PATH"));
    sprintf(PIUTools_Path_Game_ROM,"%s",getenv("PIUTOOLS_ROM_PATH"));
    sprintf(PIUTools_Path_Game_Config,"%s",getenv("PIUTOOLS_CONFIG_PATH"));
    sprintf(PIUTools_Path_Game_Save,"%s",getenv("PIUTOOLS_SAVE_PATH"));
    sprintf(PIUTools_Path_Plugin,"%s",getenv("PIUTOOLS_PLUGIN_PATH"));
    sprintf(PIUTools_Path_Tmp,"%s",getenv("PIUTOOLS_TMP_PATH"));

    // Check SDK Paths
    int fail_init = 0;

    if(strlen(PIUTools_Path_Root) < 1){fail_init = 1;}
    if(strlen(PIUTools_Path_Game_ROM) < 1){fail_init = 1;}
    if(strlen(PIUTools_Path_Game_Config) < 1){fail_init = 1;}
    if(strlen(PIUTools_Path_Game_Save) < 1){fail_init = 1;}
    if(strlen(PIUTools_Path_Plugin) < 1){fail_init = 1;}
    if(strlen(PIUTools_Path_Tmp) < 1){fail_init = 1;}
    if(fail_init){
        printf("Failed to Resolve One of the Paths\n");
        printf("Root Path: %s\n",PIUTools_Path_Root);
        printf("Game Rom Path: %s\n",PIUTools_Path_Game_ROM);
        printf("Game Config Path: %s\n",PIUTools_Path_Game_Config);
        printf("Game Save Path: %s\n",PIUTools_Path_Game_Save);
        printf("Plugin Path: %s\n",PIUTools_Path_Plugin);        
        printf("Temp Path: %s\n",PIUTools_Path_Tmp);
        exit(-1);
    }    
}

/* normalizes redundant slashes */
char* fix_slashes_in_place(const char *path, char *buf, size_t nbuf) {
    int pos = 0;
    for (int i = 0; i < strlen(path); i++) {
        if (i > 0 && path[i-1] == '/' && path[i] == '/') {
            continue;
        }
        buf[pos++] = path[i];
        if (pos >= nbuf) {
            return buf;
        }
    }
    return NULL;
}

void PIUTools_Path_Resolve(const char* in_path, char* out_path){
    if(!module_initialized){init_paths();}
    char normalized_path[PATH_MAX+1] = {0x00};
    char tmp_path[1024] = {0x00};

    fix_slashes_in_place(in_path, normalized_path, PATH_MAX);

    // Handle GAME_ROM_PATH
    if(strncmp(normalized_path,GAME_ROM_PATH_TAG,strlen(GAME_ROM_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Game_ROM,normalized_path+strlen(GAME_ROM_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }
    // Handle TMP_ROOT_PATH
    if(strncmp(normalized_path,TMP_ROOT_PATH_TAG,strlen(TMP_ROOT_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Tmp,normalized_path+strlen(TMP_ROOT_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }    
    // Handle PIUTOOLS_ROOT_PATH
    if(strncmp(normalized_path,PIUTOOLS_ROOT_PATH_TAG,strlen(PIUTOOLS_ROOT_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Root,normalized_path+strlen(PIUTOOLS_ROOT_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }       
    // Handle Save Root Path
    if(strncmp(normalized_path,SAVE_ROOT_PATH_TAG,strlen(SAVE_ROOT_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Game_Save,normalized_path+strlen(SAVE_ROOT_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }  
    // Handle Plugin Path
    if(strncmp(normalized_path,PLUGIN_PATH_TAG,strlen(PLUGIN_PATH_TAG)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Plugin,normalized_path+strlen(PLUGIN_PATH_TAG));
        strcpy(out_path,tmp_path);
        return;        
    }    
    // Handle Game Config Path
    if(strncmp(normalized_path,GAME_CONFIG_PATH,strlen(GAME_CONFIG_PATH)) == 0){
        sprintf(tmp_path,"%s%s",PIUTools_Path_Game_Config,normalized_path+strlen(GAME_CONFIG_PATH));
        strcpy(out_path,tmp_path);
        return;        
    }       
    if(in_path[0] == '.'){
        realpath(normalized_path,tmp_path);
        strcpy(out_path,tmp_path);
        return;
    }      
}
