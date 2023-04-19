#pragma once


typedef enum _HOOK_CONFIG_TYPE{
    CONFIG_TYPE_END = 0,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_INT
}HOOK_CONFIG_TYPE;

typedef struct _HOOK_CONFIG_ENTRY{
    const char* section;
    const char* name;
    HOOK_CONFIG_TYPE type;
    void* data;
    int max_length;    
}HookConfigEntry,*PHookConfigEntry;

#define CONFIG_ENTRY(section,name,type,data,max_length) {section, name, type, (void*)data, (int)max_length}


int PIUTools_Config_Read(PHookConfigEntry config);
