#pragma once

typedef enum _HOOK_TYPE {
    HOOK_ENTRY_END = 0, // Sentinel to mark the end of our hook table
    HOOK_TYPE_INLINE,   // Inline hook
    HOOK_TYPE_IMPORT    // Import table hook
} HOOK_TYPE;

typedef struct _HOOK_ENTRY {
    HOOK_TYPE hook_type;
    const char* target_binary_name;          // Name of the binary (executable or shared library) to hook
    const char* source_library_name;         // Name of the library containing the function to hook
    const char* target_function_name;        // Name of the function to hook
    void* hook_function_addr;                // Address of the hook function
    void** original_function_addr_ptr;       // Pointer to the original function address
    unsigned char hook_enabled;              // Hook enabled status (1 for enabled, 0 for disabled)
} HookEntry, *PHookEntry;

#define HOOK_TARGET_BASE_EXECUTABLE ((void*)0)

#define HOOK_ENTRY(type, target_module, lib_name, func_name, hook_func, orig_func_ptr, enable) \
    { (type), (target_module), (lib_name), (func_name), (void *)(hook_func), (void **)(orig_func_ptr), (enable) }

// Template Interface for Plugins
typedef const PHookEntry (*plugin_init_t)(const char* config_path);
