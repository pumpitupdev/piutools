// Filesystem Redirect Plugin
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>




#include <plugin_sdk/ini.h>
#include <plugin_sdk/dbg.h>
#include <plugin_sdk/plugin.h>
#include <plugin_sdk/PIUTools_Filesystem.h>

static char game_directory[1024];
static char updates_directory[1024];
typedef char dir_entry_t[64];
static dir_entry_t update_entries[64];
static int num_entries = 0;


void remove_symlinks(const char *dir_path){
    DIR *dir = opendir(dir_path);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        char path[1024];
        snprintf(path, 1024, "%s/%s", dir_path, entry->d_name);

        if (entry->d_type == DT_LNK) {
            if (unlink(path) == -1) {
                perror("unlink");
            }
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 &&
                   strcmp(entry->d_name, "..") != 0) {
            remove_symlinks(path);
        }
    }

    closedir(dir);
}


void create_symlinks(const char *src_dir, const char *dest_dir, dir_entry_t*files, int num_files)
{
    for (int i = 0; i < num_files; i++) {
        char src_path[1024];
        char dest_path[1024];

        // Construct the source and destination paths
        snprintf(src_path, 1024, "%s/%s", src_dir, files[i]);
        snprintf(dest_path, 1024, "%s/%s", dest_dir, files[i]);
        printf("[%s] Symlink: %s => %s\n",__FILE__,src_path,dest_path);
        // Create the symbolic link
        if (symlink(src_path, dest_path) == -1) {
            perror("symlink");
        }
    }
}


static int parse_config(void* user, const char* section, const char* name, const char* value){  

    if(strcmp(section,"UPDATE_DIR") == 0){
        if(value == NULL){return 1;}
        strcpy(update_entries[num_entries],value);
        printf("[%s] Adding %s To Updates\n",__FILE__,value);

        num_entries++; 
    }
    return 1;
}

const PHookEntry plugin_init(const char* config_path){
    // Resolve Game Directory
    piutools_resolve_path("${GAME_ROM_PATH}/game",game_directory);
    piutools_resolve_path("${GAME_ROM_PATH}/updates",updates_directory);
    // Remove symbolic links from game directory.
    remove_symlinks(game_directory);

    if(ini_parse(config_path,parse_config,NULL) != 0){return NULL;}
    create_symlinks(updates_directory,game_directory,update_entries,num_entries);
    return NULL;
}


