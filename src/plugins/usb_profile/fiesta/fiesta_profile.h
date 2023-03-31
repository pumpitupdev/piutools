#pragma once

#pragma pack(1)

// Total Size 90132 bytes
typedef struct _FIESTA_RANK_FILE{
	unsigned int adler32;
	char player_id[8];
	unsigned char stuff[90120];
}FiestaRankFile,*PFiestaRankFile;

// Total Size 242124 bytes
typedef struct _FIESTA_SAVE_FILE{
	unsigned int adler32;
	char usb_serial[64];
	char dongle_serial[4];
	unsigned char last_saved_timestamp[8];
	unsigned char avatar_id;
	unsigned char level;
	unsigned short country_code;
	char player_id[8];
	unsigned char stuff[242032];
}FiestaSaveFile,*PFiestaSaveFile;

void USB_Profile_Generate_Fiesta(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id);