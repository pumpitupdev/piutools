#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>
#include <stddef.h>

unsigned char data[12] = {
	0x59, 0x38, 0x87, 0xBC, 0x36, 0xFA, 0xEA, 0x29, 0x86, 0x71, 0x05, 0xE9
};
unsigned char* decryptData(unsigned char* inputData,unsigned int len, unsigned int n1){
	unsigned int seed = 0xEBADA1;
	if(n1 != -1){
		seed = n1;
	}
		
	for(int i = 0; i < len;i++){
		unsigned char smbuff = inputData[i];
		inputData[i] ^= (seed >> 8) & 0xff ;
		seed = (0x68993 * (smbuff + seed) + 0x4FDCF) & 0xFFFFFF;
	}


	return inputData;
}


#define BASE 65521

static uint32_t adler32(uint32_t initval, const uint8_t *input, size_t length)
{
  unsigned int s1 = initval & 0xffff;
  unsigned int s2 = (initval >> 16) & 0xffff;
  unsigned int n;

  for (n = 0; n < length; n++) {
    s1 = (s1 + input[n]) % BASE;
    s2 = (s2 + s1) % BASE;
  }

  return (s2 << 16) + s1;
}



int main(){
	
	decryptData(data,12,0x6422599b);
	for(int i=0;i<12;i++){
		printf("%02X",data[i]);
	}printf("\n");
	printf("adler32 %04X\n",adler32(0,data,8));
}