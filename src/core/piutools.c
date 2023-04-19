// The EntryPoint for PIUTools
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <PIUTools_SDK.h>

static int loader_initialized = 0;
void __attribute__((constructor)) PIUTools_Init() {
    if(loader_initialized){return;}
    loader_initialized = 1;
    PIUTools_Plugin_Init();
    // After everything is initialized, we'll init the filesystem layer.
    PIUTools_Filesystem_Redirect_Init();
}