#pragma once

#pragma pack(1)

// Total Size 65560 bytes
typedef struct _FIESTA2_RANK_FILE{
	unsigned int adler32;
	char player_id[8];
    char crap[4];
    unsigned int adler_seed;
	unsigned char stuff[65540];
}Fiesta2RankFile,*PFiesta2RankFile;

// Total Size 236616 bytes
typedef struct _FIESTA2_SAVE_FILE{
	unsigned int adler32;
    unsigned int adler_seed;
	char player_id[8];
    char crap[4];
	unsigned int region_id;
    unsigned int avatar_id; 
    unsigned int level; // defaults to 1.
	char crap_2[0x4C];
    unsigned int adler_of_crypt_data;
    unsigned char crypt_data[8];
    unsigned int crypt_seed;
    unsigned char crap_3[592];
    unsigned int dongle_key;
    unsigned char crap_4[32];
    unsigned char idk_data_2[32672];
	unsigned char crap_5[0x319B8];
}Fiesta2SaveFile,*PFiesta2SaveFile;

void USB_Profile_Generate_Fiesta2(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id);