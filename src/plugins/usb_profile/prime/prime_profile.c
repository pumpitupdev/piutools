
#define _LARGEFILE64_SOURCE
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include "prime_profile.h"

static int Path_Exists(const char* path){
     struct stat st;
     return stat(path, &st) != -1;
}
void generate_random_bytes(unsigned char *buf, size_t len) {
    // Seed the random number generator with the current time
    srand(time(NULL));

    for (size_t i = 0; i < len; i++) {
        // Generate a random number between 0 and 255
        buf[i] = rand() % 256;
    }
}


void USB_Profile_Generate_PRIME(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id){
    unsigned char profile_id[16] = {0x00};
    // Generate a random 16 byte id value.
    generate_random_bytes(profile_id,16);
    char save_file_path[1024] = {0x00};
    const char* save_filename = "prime.bin";
    sprintf(save_file_path,"%s/%s",profile_path,save_filename);
    FILE* fp = fopen(save_file_path,"wb");
    fwrite(profile_id,sizeof(profile_id),1,fp);
    fclose(fp);
    chmod(save_file_path, 0666);
}