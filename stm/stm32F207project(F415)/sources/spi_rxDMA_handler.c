#include "main.h"
#include "ethernet.h"

extern adpcm_message_struct   adpcm_msg[2]; 
extern adpcm_page_ctrl_struct adpcm_ctrl[MAX_CHANNEL];
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern uint8_t                adpcm_ready;
extern initial_info_struct    info_ini;
extern uint16_t spi_dma_buffer[2][SPI_RX_DMA];
extern int msg_8bit_size;
extern int msg_16bit_size;
extern int msg_32bit_size;
extern alarm_struct alarm_data;

void DMA1_Stream0_IRQHandler(void)
{
  static uint16_t msg_id[2] = {0,0};
  static int wrr = 0;
  static int msg_index = 0;
  static uint16_t data_temp[2] = {0,0};
  static uint32_t mode = DLE_STX;
  uint32_t *pdata = (uint32_t*)&data_temp[0];
  uint16_t *padpcm_msg;
  uint32_t* pin;
  uint32_t* pout;
  uint32_t crc;
  int i,j,id,adr;
             
  DMA_ClearFlag(DMA1_Stream0, DMA_IT_TC | DMA_IT_TE);
  DMA_ClearITPendingBit(DMA1_Stream0, DMA_IT_TCIF0 | DMA_IT_TEIF0);
 
  id = DMA_GetCurrentMemoryTarget(DMA1_Stream0);

  for(i = 0;  i < SPI_RX_DMA; i++)
  {
     data_temp[1] = data_temp[0];
     data_temp[0] = spi_dma_buffer[id][i];
    
     if(mode == DLE_STX)
     {
       if(*pdata == DLE_STX) 
       {
         mode = DLE_ETX;
         padpcm_msg = (uint16_t*)&adpcm_msg[msg_index];
         wrr = 0;
       }
     }
     else if(mode == DLE_ETX)
     {       
       if(*pdata == DLE_ETX) 
       {  
         CRC_ResetDR();
         
         crc = CRC_CalcBlockCRC(((uint32_t*)&adpcm_msg[msg_index])+1,msg_32bit_size-1); 
      
         if(crc == adpcm_msg[msg_index].crc)
         {
           if(adpcm_msg[msg_index].msg_id == F415_CHECK_SPI_CONNECT+100)
           {
             info_ini.f415_spi_error = 0;
           }
           else if(adpcm_msg[msg_index].msg_id == F415_AUDIO_STREAM)
           {
               for(i = 0; i < MAX_CHANNEL; i++)
               {   
                 id  = adpcm_ctrl[i].id;
                 adr = adpcm_ctrl[i].adr++;
                 
                 pin  = (uint32_t*)&padpcm[id][i]->adpcm_data[adr][0];
                 pout = (uint32_t*)&adpcm_msg[msg_index].adpcm_block[i].adpcm_data[0];
             
                 for(j = 0; j < 28; j++) *(pin++) = *(pout++);
                 
                 if(adr == 0)
                 {
                   padpcm[id][i]->prevsample = adpcm_msg[msg_index].adpcm_block[i].prevsample;
                   padpcm[id][i]->previndex =  adpcm_msg[msg_index].adpcm_block[i].previndex;
                   padpcm[id][i]->id = i;
                   padpcm[id][i]->time = GetTime();
                   adpcm_ctrl[i].crc = crc32_t(-1,padpcm[id][i],116,116);
                 }
                 else if(adr == (ADPCM_MAX_BLOCK-1)) 
                 {
                   padpcm[id][i]->crc  = crc32_t(adpcm_ctrl[i].crc,&padpcm[id][i]->adpcm_data[adr][0],120,120) ^ 0xffffffff;
                   adpcm_ctrl[i].id  ^= 1;
                   adpcm_ctrl[i].adr  = 0;
                   adpcm_ctrl[i].done = 1;
                 }
                 else
                 {
                    adpcm_ctrl[i].crc = crc32_t(adpcm_ctrl[i].crc,&padpcm[id][i]->adpcm_data[adr][0],112,112);
                 }
               }
               
               if(adr == (ADPCM_MAX_BLOCK-1))
               {
                 for(i = 0; i < MAX_CHANNEL; i++) adpcm_ready |= adpcm_ctrl[i].done;
               }
           }
         }
         
          msg_id[0] = adpcm_msg[msg_index].msg_counter;
          
          if(msg_id[0] != (msg_id[1]+1))
          {
            //sys_info.L151_stream_error++;
            alarm_data.err_crc_binar++;
          }
          
          msg_id[1] = msg_id[0];
              
          *pdata = 0;
          mode = DLE_STX;         
          msg_index ^= 1; 
          
       }
       else if(*pdata == DLE_DLE) 
       {
         data_temp[0] = 0;
       }
       else 
       {
         if(wrr++ < msg_16bit_size) *(padpcm_msg++) = data_temp[0];
       }
     
      }
  }
}
