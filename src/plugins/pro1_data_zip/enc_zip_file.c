#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "PIUTools_Debug.h"
#include "enc_zip_file.h"
#include "util.h"

enc_zip_file_header *generate_header(int fd) {
    enc_zip_file_header *header = (enc_zip_file_header *)malloc(sizeof(enc_zip_file_header));
    if (header == NULL) {
        perror("malloc()");
        return NULL;
    }

    strncpy(header->magic, ">>", 2);
    header->subkey_size = PRO1_SUBKEY_SIZE;
    generate_random_bytes(header->subkey, PRO1_SUBKEY_SIZE);
    generate_random_bytes(header->salt, PRO1_AES_BLOCK_SIZE);

    // get file size
    off_t curpos;
    curpos = lseek(fd, 0, SEEK_CUR);
    header->file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, curpos, SEEK_SET);

    return header;
}
