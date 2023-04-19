#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ini.h"

#include <PIUTools_SDK.h>

static PHookConfigEntry cur_config;
char config_path[1024];

static int parse_config(void *user, const char *section, const char *name, const char *value) {
    // Filter For Section
    if(section == NULL){return 1;}
    if(name == NULL){return 1;}
    PHookConfigEntry cur_entry = cur_config;
    if(cur_entry != NULL){
        while(cur_entry->type != CONFIG_TYPE_END){
            if(strcmp(section,cur_entry->section) == 0 && strcmp(name,cur_entry->name) == 0){
                if(value == NULL){ return 1;}                
                if(cur_entry->type == CONFIG_TYPE_STRING){
                    strncpy((char*)cur_entry->data,value,cur_entry->max_length);
                    break;
                }else{
                    char *ptr;
                    int tval = strtoul(value, &ptr, 10);
                    if(cur_entry->type == CONFIG_TYPE_INT){
                        *(int*)cur_entry->data = tval;
                    }else{
                        *(int*)cur_entry->data = (tval == 1);
                    }
                    break;                    
                }
            }
            cur_entry++;        
        }
    }
    return 1;
}

int PIUTools_Config_Read(PHookConfigEntry config){
    if(strlen(config_path) < 1){
        sprintf(config_path,"%s",getenv("PIUTOOLS_CONFIG_PATH"));
    }
    cur_config = config;
    return ini_parse(config_path, parse_config, NULL);
}