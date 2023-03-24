#pragma once

#define MAX_FILESYSTEM_SUB 256

enum _SUB_TYPE{
    SUB_TYPE_START,
    SUB_TYPE_FILE,
    SUB_TYPE_ANY
};

typedef struct _MOUNTS_ENTRY{
    char enabled;
    char from[128];
    char to[128];
}MountsEntry,*PMountsEntry;

typedef struct _PATH_SUBST{
    char enabled;
    char* src_path;
    char* replacement_path;
    char sub_type;
}PathSubst,*PPathSubst;

// This is a jank way to do it - I get it.
extern unsigned int num_subst_paths;
extern PathSubst PIUTools_Filesystem_Sub[MAX_FILESYSTEM_SUB];

#define SEARCH_ANY_TAG "{*}"
#define SEARCH_FILE_TAG "{F}"

void PIUTools_Filesystem_Init(void);
PPathSubst PIUTools_Filesystem_Add(const char* path_from, const char* path_to);
void PIUTools_Filesystem_RemoveEntry(PPathSubst entry);
void PIUTools_Filesystem_RemoveMountEntry(PMountsEntry entry);
PMountsEntry PIUTools_Filesystem_AddMountEntry(const char* mount_from, const char* mount_to);
char* PIUTools_Filesystem_Resolve_Path(const char* orig_path, char* sub_path);
int PIUTools_Filesystem_Create_Directory(const char* path);
int PIUTools_Filesystem_Path_Exist(const char* path);