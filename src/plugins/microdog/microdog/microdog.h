#ifndef __MICRODOG_H
#define __MICRODOG_H

#define MD40_PACKET_SIZE 596
#define MD40_DAEMON_ADDR "/var/run/microdog/u.daemon"
#define MD40_SOCKET_MASK "/tmp/u.XXXXXX"

#define MD33_IOCTL 0x6B00
#define MD34_IOCTL 0x6B01

#define MD34_PATH_USB "/dev/usbdog"
#define MD34_PATH_LPT "/dev/mhdog"

void MicroDog_Init(const char* path_to_dongle_file);
void MicroDog_HandlePacket(unsigned char* packet_data);
#endif
