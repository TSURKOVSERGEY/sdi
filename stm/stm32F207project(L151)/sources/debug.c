#include "debug.h"
#include "main.h"
#include "nand_hw_driver.h"

char dump_buffer[1024];

int index = 0;

void myprint(char* pb)
{
  while(*pb != 0)
  {
    dump_buffer[index++] = *(pb++);
  }

}

void reset_index(void)
{
  index = 0;
  memset(dump_buffer,0,sizeof(dump_buffer));
}


void debug(void)
{
  unsigned short tbuf[2048];
  unsigned short rbuf[2048];
  
  int crc;
  int i;
  
  
  for(i = 0; i < 2048; i++) tbuf[i] = i;
  
  crc = crc32(tbuf,2048,2048);
  
  nand_16bit_write_page(0,tbuf,64);
  
  nand_16bit_read_page(0,rbuf,64);
  
  crc = crc32(rbuf,2048,2048);
  
  printf("crc %d",crc);
  
  
}
 