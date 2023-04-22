#pragma once

#define MAX_FILESYSTEM_SUB 256

enum _SUB_TYPE{
    SUB_TYPE_FULL,
    SUB_TYPE_START,
    SUB_TYPE_FILE,
    SUB_TYPE_ANY,
};



typedef struct _PATH_SUBST{
    char enabled;
    char* src_path;
    char* replacement_path;
    char sub_type;
}PathSubst,*PPathSubst;

// This is a jank way to do it - I get it.
extern unsigned int num_subst_paths;
extern PathSubst PIUTools_Filesystem_Sub[MAX_FILESYSTEM_SUB];


// New Section
const char* PIUTools_Filesystem_GetFileName(const char* path);
int PIUTools_Filesystem_Create_Directory(const char* path);
int PIUTools_Filesystem_Path_Exist(const char* path);

PPathSubst PIUTools_Filesystem_AddRedirect(const char* path_from, const char* path_to);
void PIUTools_Filesystem_RemoveRedirect(PPathSubst entry);
char* PIUTools_Filesystem_Redirect_Path(const char* func_name, const char* orig_path, char* sub_path);
void PIUTools_Filesystem_Redirect_Init(void);