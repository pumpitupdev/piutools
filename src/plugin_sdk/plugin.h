#pragma once

typedef struct _HOOK_ENTRY{
    const char* target_module_name;
    const char* target_function_name;
    void* hook_function_addr;
    void** real_function_addr;
    unsigned char enabled;
}HookEntry,*PHookEntry;

// Template Interface for Plugins
typedef int (*plugin_init_t)(const char* config, PHookEntry* table);