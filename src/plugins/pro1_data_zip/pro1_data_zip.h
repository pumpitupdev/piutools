#pragma once
#include <stdint.h>
#include "aes.h"
#include "enc_zip_file.h"

/**
 * bookkeeping for each opened file
 */
typedef struct zip_enc_context {
    char *pathname;
    int fd;
    off_t pos;
    uint8_t aes_key[24];
    struct AES_ctx aes_ctx;
    enc_zip_file_header *header;
    struct zip_enc_context *next;
    // each data zip can have a signature field at the end
    // that is created by a keypair internal to FiM. The public bits are
    // hardcoded in the piu binary. This is followed by a magic "SRSLY"
    // footer.
    uint8_t sig[128+5];
} zip_enc_context;
