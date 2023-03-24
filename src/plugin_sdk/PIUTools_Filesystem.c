// Extensions for Filesystem Manipulation Support
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "dbg.h"
#include "plugin.h"
#include "PIUTools_Filesystem.h"
// #define DEBUG_REDIRECT
unsigned int num_subst_paths = 0;
PathSubst PIUTools_Filesystem_Sub[MAX_FILESYSTEM_SUB];


#define MAX_MOUNT_ENTRIES 8

static MountsEntry mount_entries[MAX_MOUNT_ENTRIES];
static char fake_mount_path[1024] = {0x00};

void UpdateMountFile(void){
    char line[512]={0x00};
    FILE* fp = fopen(fake_mount_path,"wb");
    if(!fp){return;}
    for(int i=0;i<MAX_MOUNT_ENTRIES;i++){
        if(mount_entries[i].enabled){
            sprintf(line,"%s %s\n",mount_entries[i].from,mount_entries[i].to);
            fwrite(line,strlen(line),1,fp);
        }
    }
    fclose(fp);
}


void PIUTools_Filesystem_RemoveEntry(PPathSubst entry){
    entry->enabled = 0;
    num_subst_paths--;
}

void PIUTools_Filesystem_RemoveMountEntry(PMountsEntry entry){
    entry->enabled = 0;
    UpdateMountFile();
}

PMountsEntry PIUTools_Filesystem_AddMountEntry(const char* mount_from, const char* mount_to){
    for(int i=0; i < MAX_MOUNT_ENTRIES; i++){
        if(mount_entries[i].enabled == 0){
            strncpy(mount_entries[i].from,mount_from,sizeof(mount_entries[i].from));
            strncpy(mount_entries[i].to,mount_to,sizeof(mount_entries[i].to));
            mount_entries[i].enabled = 1;
            UpdateMountFile();
            return &mount_entries[i];
        }
    }
    return NULL;
}

void PIUTools_Filesystem_Init(void){
    for(int i=0;i<MAX_FILESYSTEM_SUB;i++){
        memset(&PIUTools_Filesystem_Sub[i],0,sizeof(PathSubst));
    }
    // Addition for Mounts
    sprintf(fake_mount_path,"%s/mounts",piutools_game_save_path);
    PIUTools_Filesystem_Add("/proc/mounts",fake_mount_path);    
}


PPathSubst PIUTools_Filesystem_Add(const char* path_from, const char* path_to){
        PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_START;

        if(strncmp(path_from,SEARCH_ANY_TAG,strlen(SEARCH_ANY_TAG)) == 0){
            PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_ANY;
            path_from += strlen(SEARCH_ANY_TAG);
        }

        if(strncmp(path_from,SEARCH_FILE_TAG,strlen(SEARCH_FILE_TAG)) == 0){
            PIUTools_Filesystem_Sub[num_subst_paths].sub_type = SUB_TYPE_FILE;
            path_from += strlen(SEARCH_FILE_TAG);
        }        

        PIUTools_Filesystem_Sub[num_subst_paths].src_path = malloc(strlen(path_from)+1);
        strcpy(PIUTools_Filesystem_Sub[num_subst_paths].src_path,path_from);
        // We may have to deal with piutools wildcards for resolved paths so let's do that.
        char resolved_path[1024] = {0x00};
        piutools_resolve_path(path_to,resolved_path);
        if(strlen(resolved_path) > 1){path_to = resolved_path;}
        
        PIUTools_Filesystem_Sub[num_subst_paths].replacement_path = malloc(strlen(path_to)+1);
        strcpy(PIUTools_Filesystem_Sub[num_subst_paths].replacement_path,path_to);
        DBG_printf("[%s] %s => %s",__FILE__,PIUTools_Filesystem_Sub[num_subst_paths].src_path,PIUTools_Filesystem_Sub[num_subst_paths].replacement_path);
        num_subst_paths++;
        return &PIUTools_Filesystem_Sub[num_subst_paths-1];        
}

int PIUTools_Filesystem_Path_Exist(const char* path){
     struct stat st;
     return stat(path, &st) != -1;
}


int PIUTools_Filesystem_Create_Directory(const char* path){
    struct stat st;
    if (!PIUTools_Filesystem_Path_Exist(path)) {
        return mkdir(path, 0755);
    }
    return 0;
}


const char* get_filename_from_path(const char* path) {
    const char* filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}

char* PIUTools_Filesystem_Resolve_Path(const char* orig_path, char* sub_path){


    for(int i=0;i<num_subst_paths;i++){
        if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_START){
            if(strncmp(orig_path,PIUTools_Filesystem_Sub[i].src_path,strlen(PIUTools_Filesystem_Sub[i].src_path)) == 0){
                sprintf(sub_path,"%s%s",PIUTools_Filesystem_Sub[i].replacement_path,orig_path+strlen(PIUTools_Filesystem_Sub[i].src_path));
                #ifdef DEBUG_REDIRECT
                    printf("[%s] Redirect 1: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                #endif                  
                return sub_path;
            }            
        }else if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_ANY){
            if(strlen(orig_path) < strlen(PIUTools_Filesystem_Sub[i].src_path)){continue;}
            if(strstr(orig_path,PIUTools_Filesystem_Sub[i].src_path) != NULL){
                if(strcmp(orig_path,PIUTools_Filesystem_Sub[i].replacement_path)){
                    strcpy(sub_path,PIUTools_Filesystem_Sub[i].replacement_path);
                    #ifdef DEBUG_REDIRECT
                        printf("[%s] Redirect 2: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                    #endif                    
                    return sub_path;
                }
            }
        }else if(PIUTools_Filesystem_Sub[i].sub_type == SUB_TYPE_FILE){
            if(strlen(orig_path) < strlen(PIUTools_Filesystem_Sub[i].src_path)){continue;}
            if(strcmp(get_filename_from_path(orig_path),PIUTools_Filesystem_Sub[i].src_path) == 0){
                if(strcmp(orig_path,PIUTools_Filesystem_Sub[i].replacement_path)){
                    strcpy(sub_path,PIUTools_Filesystem_Sub[i].replacement_path);
                    #ifdef DEBUG_REDIRECT
                        printf("[%s] Redirect 3: %s => %s\n",__FUNCTION__,orig_path,sub_path);
                    #endif                    
                    return sub_path;
                }
            }
        }         
    }
    #ifdef DEBUG_REDIRECT
        if(orig_path){printf("[%s] Bypass: %s\n",__FUNCTION__,orig_path);}
    #endif    
    return (char*)orig_path;
}


