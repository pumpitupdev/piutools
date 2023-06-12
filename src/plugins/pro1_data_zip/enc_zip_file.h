#ifndef ENC_ZIP_FILE_H
#define ENC_ZIP_FILE_H

#include <stdint.h>

#define PRO1_SUBKEY_SIZE 1024
#define PRO1_AES_BLOCK_SIZE 16

typedef struct __attribute__((__packed__)) enc_zip_file_header {
    char magic[2]; // ">>"
    uint32_t subkey_size;
    uint8_t subkey[PRO1_SUBKEY_SIZE];

    // encryption salt
    uint8_t salt[PRO1_AES_BLOCK_SIZE];

    // file size of the decrypted file
    uint32_t file_size;

    // needs to start with "<<" when decrypted
    uint8_t verify_block[PRO1_AES_BLOCK_SIZE];
} enc_zip_file_header;

enc_zip_file_header *generate_header(int fd);


#endif
