#pragma once

// Total Size 90136 bytes
typedef struct _FIESTAEX_RANK_FILE{
	unsigned int adler32;
	char player_id[8];
	unsigned char stuff[90124];
}FiestaEXRankFile,*PFiestaEXRankFile;

// Total Size 242140 bytes
typedef struct _FIESTAEX_SAVE_FILE{
	unsigned int adler32;
	char usb_serial[64];
	char dongle_serial[4];
	unsigned char last_saved_timestamp[8];
	unsigned char avatar_id;
	unsigned char level;
	unsigned short country_code;
	char player_id[8];
	unsigned char stuff[242048];
}FiestaEXSaveFile,*PFiestaEXSaveFile;

void USB_Profile_Generate_FiestaEX(const char* profile_path, const char* player_name, const char* usb_serial, int avatar_id);