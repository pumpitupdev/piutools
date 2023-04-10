#pragma once

#include <stdint.h>


typedef struct _SMBIOS_STRUCTURE{
	uint8_t type;
	uint8_t length;
	uint16_t handle;
}SMBIOS_STRUCTURE;


typedef struct {
  SMBIOS_STRUCTURE            Hdr;
  uint8_t         Manufacturer;
  uint8_t         ProductName;
  uint8_t         Version;
  uint8_t         SerialNumber;
  uint8_t         AssetTag;
  uint8_t    FeatureFlag;
  uint8_t         LocationInChassis;
  uint16_t                      ChassisHandle;
  uint8_t                       BoardType;            ///< The enumeration value from BASE_BOARD_TYPE.
  uint8_t                       NumberOfContainedObjectHandles;
  uint16_t                      ContainedObjectHandles[1];
} SMBIOS_TABLE_TYPE2;


typedef struct _SMBIOS_CPU_ENTRY{
	SMBIOS_STRUCTURE Hdr;
	uint8_t str_socket;
	uint8_t type;
	uint8_t family;
	uint8_t str_mfg;
	uint64_t processor_id;
	uint8_t str_version;
	uint8_t voltage;
	uint16_t ext_clock;
	uint16_t max_speed;
	uint16_t current_speed;
	uint8_t status;
	uint8_t upgrade;
	uint16_t l1_handle;	
	uint16_t l2_handle;	
	uint16_t l3_handle;	
	uint8_t str_serial;
	uint8_t str_asset_tag;
	uint8_t str_part_number;
	uint8_t core_count;
	uint8_t enabled_cores;
	uint8_t thread_count;
	uint16_t proc_characteristics;
	uint16_t proc_family_2;
}SMBIOS_CPU_ENTRY,*PSMBIOS_CPU_ENTRY;

struct SMBIOSEntryPoint {
 	char EntryPointString[4];    //This is _SM_
 	unsigned char Checksum;              //This value summed with all the values of the table, should be 0 (overflow)
 	unsigned char Length;                //Length of the Entry Point Table. Since version 2.1 of SMBIOS, this is 0x1F
 	unsigned char MajorVersion;          //Major Version of SMBIOS
 	unsigned char MinorVersion;          //Minor Version of SMBIOS
 	unsigned short MaxStructureSize;     //Maximum size of a SMBIOS Structure (we will se later)
 	unsigned char EntryPointRevision;    //...
 	char FormattedArea[5];       //...
 	char EntryPointString2[5];   //This is _DMI_
 	unsigned char Checksum2;             //Checksum for values from EntryPointString2 to the end of table
 	unsigned short TableLength;          //Length of the Table containing all the structures
 	unsigned int TableAddress;	     //Address of the Table
 	unsigned short NumberOfStructures;   //Number of structures in the table
 	unsigned char BCDRevision;           //Unused
};

void generate_fake_smbios(const char* mainboard_vendor, const char* mainboard_product, const char* cpu_version, unsigned short cpu_mhz, const char* output_path);