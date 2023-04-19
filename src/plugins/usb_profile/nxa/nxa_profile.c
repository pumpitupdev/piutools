#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nxa_save.h"
#include "nxa_rank.h"

static int Path_Exists(const char* path){
     struct stat st;
     return stat(path, &st) != -1;
}

void USB_Profile_Generate_NXA(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id){
    char nxa_rank_file_path[1024] = {0x00};
    char nxa_save_file_path[1024] = {0x00};
    sprintf(nxa_save_file_path,"%s/nxasave.bin",profile_path);
    sprintf(nxa_rank_file_path,"%s/nxarank.bin",profile_path);
    if(!Path_Exists(nxa_save_file_path)){
        struct asset_nxa_usb_save * save_file = asset_nxa_usb_save_new();
        memset(save_file->review.player_id,0,sizeof(save_file->review.player_id));
        memset(save_file->stats.player_id,0,sizeof(save_file->stats.player_id));
        save_file->stats.avatar_id = avatar_id;

        strcpy(save_file->review.player_id,player_name);
        strcpy(save_file->stats.player_id,player_name);        
        strcpy(save_file->stats.usb_serial,usb_serial);
        asset_nxa_usb_save_finalize(save_file);
        asset_nxa_usb_save_encrypt((uint8_t *)save_file, sizeof(struct asset_nxa_usb_save));
        FILE* fp = fopen(nxa_save_file_path,"wb");
        fwrite(save_file,sizeof(struct asset_nxa_usb_save),1,fp);
        fclose(fp);
    }
    if(!Path_Exists(nxa_rank_file_path)){
        struct asset_nxa_usb_rank * rank_file = asset_nxa_usb_rank_new();
        struct asset_nxa_usb_rank rank_enc;
        strcpy(rank_file->usb_serial,usb_serial);
        asset_nxa_usb_rank_finalize(rank_file);  
        memcpy(&rank_enc, rank_file, sizeof(struct asset_nxa_usb_rank));                      
        asset_nxa_usb_rank_encrypt((uint8_t *) &rank_enc, sizeof(struct asset_nxa_usb_rank));
        FILE* fp = fopen(nxa_rank_file_path,"wb");
        fwrite(&rank_enc,sizeof(struct asset_nxa_usb_rank),1,fp);
        fclose(fp);
    }
}