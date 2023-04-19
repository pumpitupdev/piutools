#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fake_smbios.h"


static unsigned char gen_checksum(const unsigned char *buf, size_t len) {
    unsigned char sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return (unsigned char)(-((signed char)sum));
}

void generate_smbios_header(unsigned char* data, unsigned short table_length, unsigned short table_num_structures){
	memset(data,0,sizeof(struct SMBIOSEntryPoint));
	struct SMBIOSEntryPoint* header = (struct SMBIOSEntryPoint*)data;
	header->EntryPointString[0] = '_';
	header->EntryPointString[1] = 'S';
	header->EntryPointString[2] = 'M';
	header->EntryPointString[3] = '_';
	header->Length = 0x1F;
	header->MajorVersion = 2;
	header->MinorVersion = 7;
	header->MaxStructureSize = 0xE8;
	
	// DMI Entrypoint
	header->EntryPointString2[0] = '_';
	header->EntryPointString2[1] = 'D';
	header->EntryPointString2[2] = 'M';
	header->EntryPointString2[3] = 'I';
	header->EntryPointString2[4] = '_';
	
	header->TableLength = table_length;
	header->TableAddress = 0x1000;
	header->NumberOfStructures = table_num_structures;
	header->BCDRevision = 0x33;
	
	// Generate Checksums        
	header->Checksum2 = gen_checksum((unsigned char*)&header->EntryPointString2, 0x0F);
	header->Checksum = gen_checksum((unsigned char*)header, header->Length);
	
}
 

void generate_fake_smbios(const char* mainboard_vendor, const char* mainboard_product, const char* cpu_version, unsigned short cpu_mhz, const char* output_path){
	// We'll Generate two DMI entries, one for the mainboard data and one for the CPU.
	size_t table_size = 0;
	size_t mainboard_entry_size = strlen(mainboard_vendor) + 1 + strlen(mainboard_product) + 1 + 1 + sizeof(SMBIOS_TABLE_TYPE2);
	unsigned char* mainboard_entry = (unsigned char*)calloc(1,mainboard_entry_size);
	SMBIOS_TABLE_TYPE2* mb_entry = (SMBIOS_TABLE_TYPE2*)mainboard_entry;
	
	mb_entry->Hdr.type = 2;
	mb_entry->Hdr.length = sizeof(SMBIOS_TABLE_TYPE2);
	mb_entry->Hdr.handle = 0;
	mb_entry->Manufacturer = 1;
	mb_entry->ProductName = 2;
	mb_entry->FeatureFlag = 9;
	mb_entry->BoardType = 10; // Motherboard
	
	// Copy the Strings for the Mainboard
	strcpy(mainboard_entry +  sizeof(SMBIOS_TABLE_TYPE2),mainboard_vendor);
	strcpy(mainboard_entry +  sizeof(SMBIOS_TABLE_TYPE2) + strlen(mainboard_vendor) + 1, mainboard_product);
	
	size_t cpu_entry_size = sizeof(SMBIOS_CPU_ENTRY) + strlen(cpu_version) + 2;
	unsigned char* cpu_entry = (unsigned char*)calloc(1,cpu_entry_size);
	SMBIOS_CPU_ENTRY* c_entry = (SMBIOS_CPU_ENTRY*)cpu_entry;
	c_entry->Hdr.type = 4;
	c_entry->Hdr.length = sizeof(SMBIOS_CPU_ENTRY);
	c_entry->Hdr.handle = 1;
	
	c_entry->type = 3; // Central Processor 
	c_entry->family = 0x0F;  // This is the Celeron family but whatever.
	c_entry->proc_family_2 = 0x0F; 
	c_entry->str_version = 1;
	c_entry->voltage = 0x8B;
	c_entry->ext_clock = 0x64;
	c_entry->max_speed = cpu_mhz;
	c_entry->current_speed = cpu_mhz;
	c_entry->status = 0x41;
	c_entry->upgrade = 2; // Unknown
	c_entry->core_count = 16;
	c_entry->enabled_cores = 16;
	c_entry->thread_count = 32;
	
	// Set our CPU String 
	strcpy(cpu_entry + sizeof(SMBIOS_CPU_ENTRY), cpu_version);
	
	// We have to also set up the DMI header and SMBIOS header for the fake file.
	// Therefore, we need the size of the table in total.
	table_size += mainboard_entry_size + cpu_entry_size;
	
	unsigned char smbios_header[32] = {0xFF};
	generate_smbios_header(smbios_header,table_size,2);
	
	// Now we're going to generate the fake file.
	unsigned char* fake_smbios_data = malloc(0x2000);
	memset(fake_smbios_data,0xFF,0x2000);
	memcpy(fake_smbios_data,smbios_header,sizeof(smbios_header));
	
	int offset = 0x1000;
	memcpy(fake_smbios_data+offset,mainboard_entry, mainboard_entry_size);
	offset += mainboard_entry_size;
	memcpy(fake_smbios_data+offset,cpu_entry,cpu_entry_size);
	offset += cpu_entry_size;
	
	FILE* fp = fopen(output_path,"wb");
	fwrite(fake_smbios_data,0x2000,1,fp);
	fclose(fp);
	
}
