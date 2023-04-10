// Handler for UCS Functionality 
#define _FILE_OFFSET_BITS 64
// Filesystem Redirect Plugin
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>


int path_exists(const char *path) {
    if (access(path, F_OK) == 0) {
        // File or directory exists
        return 1;
    } else {
        // File or directory does not exist
        return 0;
    }
}


void
rmtree(const char path[])
{
    size_t path_len;
    char *full_path;
    DIR *dir;
    struct stat stat_path, stat_entry;
    struct dirent *entry;

    // stat for the path
    int res = stat(path, &stat_path);
    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0) {
        fprintf(stderr, "%s: %s\n", "Is not directory", path);
        exit(-1);
    }
  

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, "%s: %s\n", "Can`t open directory", path);
        exit(-1);
    }

    // the length of the path
    path_len = strlen(path);

    // iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL) {

        // skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // determinate a full path of an entry
        full_path = calloc(path_len + strlen(entry->d_name) + 1, sizeof(char));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // stat for the entry
        stat(full_path, &stat_entry);

        // recursively remove a nested directory
        if (S_ISDIR(stat_entry.st_mode) != 0) {
            rmtree(full_path);
            continue;
        }

        // remove a file object
        if (unlink(full_path) == 0)
            printf("Removed a file: %s\n", full_path);
        else
            printf("Can`t remove a file: %s\n", full_path);
        free(full_path);
    }

    // remove the devastated directory and close the object of it
    if (rmdir(path) == 0)
        printf("Removed a directory: %s\n", path);
    else
        printf("Can`t remove a directory: %s\n", path);

    closedir(dir);
}


int mkdirp(const char *path, mode_t mode) {
    char *dir;
    char *slash;
    int ret;
    
    if (!path || !*path) {
        return 0;
    }
    
    dir = strdup(path);
    if (!dir) {
        return -1;
    }
    
    slash = dir;
    while ((slash = strchr(slash + 1, '/'))) {
        *slash = '\0';
        ret = mkdir(dir, mode);
        if (ret != 0 && errno != EEXIST) {
            free(dir);
            return -1;
        }
        *slash = '/';
    }
    
    ret = mkdir(path, mode);
    free(dir);
    
    return ret;
}


const PHookEntry plugin_init(const char* config_path){
    // Resolve Game Directory
    char ucs_base_path[1024] = {0x00};
    piutools_resolve_path("${SAVE_ROOT_PATH}/ucs",ucs_base_path);
    // Remove The Directory if it exists
    char ucs_base_slash[1024] = {0x00};
    sprintf(ucs_base_slash,"%s/",ucs_base_path);
    if(path_exists(ucs_base_slash)){
        rmtree(ucs_base_slash);
    }
    

    char ucs_path[1024] = {0x00};
    char orig_ucs_path[64] = {0x00};
    PIUTools_Filesystem_Add("/var/ucs",ucs_base_path);
    for(int i=0;i<2;i++){
        sprintf(ucs_path,"%s/%d",ucs_base_path,i);
        sprintf(orig_ucs_path,"/var/ucs/%d",i);
       // mkdirp(ucs_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        PIUTools_Filesystem_Add(orig_ucs_path,ucs_path);
    }
    return NULL;
}


