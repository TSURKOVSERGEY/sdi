#include "main.h"
#include "spi_f207.h"

extern int                  msg_8bit_size;
extern int                  msg_16bit_size;
extern int                  msg_32bit_size;
extern adpcm_message_struct adpcm_msg[2]; 
extern uint8_t              half_index;
extern int8_t               msg_index;
extern uint16_t             spi_tx_buffer[DMA_SPI_SIZE];

void f207_TransmitHandler(void)
{
    
  int      index;
  int16_t  wrw;  
  uint16_t *p; 
  static uint16_t msg_counter = 0;
  
   if(half_index == MAX_SAMPLE) 
   {
     index      = msg_index;
     msg_index ^= 1;
     half_index = 0;
     wrw        = 0;
            
     p = (uint16_t*)&adpcm_msg[index];
     adpcm_msg[index].msg_id = F415_AUDIO_STREAM; //MSG_STERAM;
     adpcm_msg[index].msg_counter = msg_counter++;
     CRC_ResetDR();
     adpcm_msg[index].crc = CRC_CalcBlockCRC(((uint32_t*)&adpcm_msg[index])+1,msg_32bit_size-1);
          
     spi_tx_buffer[wrw++] = DLE;
     spi_tx_buffer[wrw++] = STX;
         
     for(int i = 0; i < msg_16bit_size; i++)
     {
       spi_tx_buffer[wrw++] = *(p++);  
       if(spi_tx_buffer[wrw-1] == DLE) spi_tx_buffer[wrw++] = DLE;
     }
      
      spi_tx_buffer[wrw++] = DLE;
      spi_tx_buffer[wrw++] = ETX;
         
      SPI_f207SendMessage(spi_tx_buffer,1500);
      
    }     
 }
