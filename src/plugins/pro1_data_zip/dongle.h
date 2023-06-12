#pragma once
#include <stdint.h>
#include "enc_zip_file.h"

int derive_aes_key_from_ds1963s(const enc_zip_file_header *h, uint8_t out[24]);
int ds1963s_compute_data_sha(const uint8_t *input, uint8_t *out);
