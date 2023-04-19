#pragma once

#define MAX_MOUNT_ENTRIES 8

typedef struct _MOUNTS_ENTRY{
    char enabled;
    char from[128];
    char to[128];
}MountsEntry,*PMountsEntry;

void PIUTools_Mount_RemoveMountEntry(PMountsEntry entry);
PMountsEntry PIUTools_Mount_AddEntry(const char* mount_from, const char* mount_to);