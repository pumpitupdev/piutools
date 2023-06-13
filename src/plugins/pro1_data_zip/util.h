#pragma once
#include <stdint.h>

#define min(x,y) ((x) > (y) ? (y) : (x))

void generate_random_bytes(uint8_t *buf, int count);
