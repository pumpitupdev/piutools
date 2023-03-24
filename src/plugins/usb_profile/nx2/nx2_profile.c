#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nx2_save.h"
#include "nx2_rank.h"

#include <plugin_sdk/PIUTools_Filesystem.h>

void USB_Profile_Generate_NX2(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id){
    char nx2_rank_file_path[1024] = {0x00};
    char nx2_save_file_path[1024] = {0x00};
    sprintf(nx2_save_file_path,"%s/nx2save.bin",profile_path);
    sprintf(nx2_rank_file_path,"%s/nx2rank.bin",profile_path);
    if(!PIUTools_Filesystem_Path_Exist(nx2_save_file_path)){
        struct asset_nx2_usb_save * save_file = asset_nx2_usb_save_new();
        memset(save_file->review.player_id,0,sizeof(save_file->review.player_id));
        memset(save_file->stats.player_id,0,sizeof(save_file->stats.player_id));
        save_file->stats.avatar_id = avatar_id;

        strcpy(save_file->review.player_id,player_name);
        strcpy(save_file->stats.player_id,player_name);
        strcpy(save_file->stats.usb_serial,usb_serial);
        asset_nx2_usb_save_finalize(save_file);
        asset_nx2_usb_save_encrypt((uint8_t *)save_file, sizeof(struct asset_nx2_usb_save));
        FILE* fp = fopen(nx2_save_file_path,"wb");
        fwrite(save_file,sizeof(struct asset_nx2_usb_save),1,fp);
        fclose(fp);
    }
    if(!PIUTools_Filesystem_Path_Exist(nx2_rank_file_path)){
        struct asset_nx2_usb_rank * rank_file = asset_nx2_usb_rank_new();
        asset_nx2_usb_rank_finalize(rank_file);
        struct asset_nx2_usb_rank rank_enc;
        memcpy(&rank_enc, rank_file, sizeof(struct asset_nx2_usb_rank));
        asset_nx2_usb_rank_encrypt((uint8_t *) &rank_enc, sizeof(struct asset_nx2_usb_rank));
        FILE* fp = fopen(nx2_rank_file_path,"wb");
        fwrite(&rank_enc,sizeof(struct asset_nx2_usb_rank),1,fp);
        fclose(fp);
    }
}