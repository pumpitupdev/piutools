#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void generate_random_bytes(uint8_t *buf, int count) {
    for (int i = 0; i < count; i++) {
        buf[i] = rand() % 255;
    }
}
