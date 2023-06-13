#ifndef _SIG_H_
#define _SIG_H_
#include "pro1_data_zip.h"
#include <stdint.h>

const unsigned char their_pubkey[140], our_pubkey[140], our_privkey[608];

typedef ssize_t (*read_func_t)(int, void *, size_t);
typedef int (*lseek_func_t)(int, off_t, int);

int generate_file_signature(zip_enc_context *ctx, int fd,
        read_func_t read_func, lseek_func_t lseek_func,
        uint8_t *out);

#endif /* _SIG_H_ */
