#include "stm32l1xx_conf.h"
#include "spi_L151.h"
#include "stdio.h"
#include "stdlib.h"
#include "main.h"

extern int16_t  gain_mass[MAX_CHANNEL];

#pragma pack(push,1)   
extern struct spi_message
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint16_t data[256];
 } tx_spi_msg, rx_spi_msg;  
#pragma pack(pop)


uint16_t Command_Handler(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  uint32_t mode = DLE_STX;
  uint16_t data_temp[2] = {0,0};
  uint32_t *pdata = (uint32_t*)&data_temp[0];
  uint8_t  wrr = 0;  
  uint32_t crc = 0;
  uint16_t *prx_spi_msg = (uint16_t*)&rx_spi_msg;
  uint16_t rx_spi_msg_size = sizeof(rx_spi_msg) / 2;
    
  SysTick->CTRL |= SysTick_CTRL_ENABLE;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  
  
  
  while(1)
  {
    while(!(SPI1->SR & SPI_SR_RXNE));

    data_temp[1] = data_temp[0];
    data_temp[0] = SPI1->DR;
  
    if(mode == DLE_STX)
    {
      if(*pdata == DLE_STX)
      {
        mode = DLE_ETX;
        prx_spi_msg = (uint16_t*) &rx_spi_msg;
        wrr = 0;
        continue;
      }
    }
    else if(mode == DLE_ETX)
    {
      if(*pdata == DLE_ETX)
      {
         mode = DLE_STX;
         prx_spi_msg = (uint16_t*) &rx_spi_msg;
         *pdata = 0;
         wrr = 0;
         CRC_ResetDR();
         crc = CRC_CalcBlockCRC(((uint32_t*)&rx_spi_msg)+1,(uint32_t)rx_spi_msg.msg_len & 0xff);
         if(crc != rx_spi_msg.msg_crc) continue;
         else 
         {
           
////////////////////////////////////////////////////////////////////////////////           
           
           switch(rx_spi_msg.msg_id)
            {
              case MODE_SYNC: 
                   SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
                   tx_spi_msg.msg_len = 1;
                   tx_spi_msg.msg_id  = MODE_SYNC;
                   CRC_ResetDR();
                   tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
                   SendF207Message((uint16_t*)&tx_spi_msg,4);
                   SysTick->CTRL |= SysTick_CTRL_ENABLE;
                   SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  
              break;
          
              case MODE_CMD:
                   SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
                   tx_spi_msg.msg_len = 1;
                   tx_spi_msg.msg_id  = MODE_CMD;
                   CRC_ResetDR();
                   tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
                   SendF207Message((uint16_t*)&tx_spi_msg,4);
                   SysTick->CTRL |= SysTick_CTRL_ENABLE;
                   SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  
              break;
          
              case MODE_STREAM:
                   SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
                   return MODE_STREAM; 
              break;
                       
              case MODE_SET_GAIN:
                   for(int i = 0; i < MAX_CHANNEL; i++) gain_mass[i] = rx_spi_msg.data[i+3];
                   //status = MODE_CMD;
              break;
              
            }
           
////////////////////////////////////////////////////////////////////////////////           
        
        }
      }
      else if(*pdata == DLE_DLE)
      {
        data_temp[0] = 0;
        continue;
      }
      else if(wrr++ < rx_spi_msg_size)
      {
        *(prx_spi_msg++) = data_temp[0];
      }
    }


  }
}








/*
            case MODE_GET_GAIN:
              
              SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;

              for(int i = 0; i < MAX_CHANNEL; i++) tx_msg[i+1] = gain_mass[i];
              L151_SendMessage(tx_msg,MAX_CHANNEL+1);
              SysTick->CTRL |= SysTick_CTRL_ENABLE;
              RCC_GetClocksFreq(&RCC_Clocks);
              SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  

              break;
  */          
