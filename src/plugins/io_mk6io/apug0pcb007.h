#pragma once 

#define PIUIO_VID 0x0547
#define PIUIO_PID 0x1002

unsigned int mk6io_read_input(void);

void mk6io_write_output(unsigned char* bytes);