#pragma once

#define PIUBTN_DRV_VID 0x0D2F
#define PIUBTN_DRV_PID 0x1010
#define PIUBTN_DRV_BUFFER_SIZE 8

void piubtn_read_input(unsigned char* input_buffer);
void piubtn_write_output(unsigned char* output_buffer);
