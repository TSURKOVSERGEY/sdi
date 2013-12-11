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
  uint16_t data_temp[2] = {0,0};
  uint32_t *pdata = (uint32_t*)&data_temp[0];
  uint32_t  crc;
  uint16_t *prx_spi_msg;
  uint16_t *end_pointer;  

  end_pointer = (uint16_t*)&rx_spi_msg + (sizeof(rx_spi_msg) / 2);
    
  SPI_L151Config(SPI_BaudRatePrescaler_256);
  SysTick->CTRL |= SysTick_CTRL_ENABLE;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  
  
  while(1)
  {
    prx_spi_msg = (uint16_t*)&rx_spi_msg;
    *pdata = 0;
   
    while(*pdata != DLE_STX)
    {
      while(!(SPI1->SR & SPI_SR_RXNE));
      data_temp[1] = data_temp[0];
      data_temp[0] = SPI1->DR;
    }

    while(*pdata != DLE_ETX)
    {
      while(!(SPI1->SR & SPI_SR_RXNE));

      data_temp[1] = data_temp[0];
      data_temp[0] = SPI1->DR;

      if(*pdata == DLE_DLE) 
      {
        data_temp[0] = 0;
      }
      else if(prx_spi_msg < end_pointer)  
      {
        *(prx_spi_msg++) = data_temp[0];
      }
      else 
      {
        crc = 0;
        break;
      }
    }

    CRC_ResetDR();
    crc = CRC_CalcBlockCRC(((uint32_t*)&rx_spi_msg)+1,(uint32_t)rx_spi_msg.msg_len & 0xff);

    if(crc != rx_spi_msg.msg_crc) continue;
           
////////////////////////////////////////////////////////////////////////////////           
    SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
           
    switch(rx_spi_msg.msg_id)
    {
      case MODE_SYNC: 
           tx_spi_msg.msg_len = 1;
           tx_spi_msg.msg_id  = MODE_SYNC;
           CRC_ResetDR();
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
           SendF207Message((uint16_t*)&tx_spi_msg,4);
      break;
          
      case MODE_CMD:
           tx_spi_msg.msg_len = 1;
           tx_spi_msg.msg_id  = MODE_CMD;
           CRC_ResetDR();
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
           SendF207Message((uint16_t*)&tx_spi_msg,4);
      break;
          
      case MODE_STREAM: 
           SPI_L151Config(SPI_BaudRatePrescaler_8);
           return MODE_STREAM; 
      break;
                       
      case MODE_GET_GAIN:
           tx_spi_msg.msg_len = (MAX_CHANNEL / 2) + 1;
           tx_spi_msg.msg_id  = MODE_GET_GAIN;
           for(int i = 0; i < MAX_CHANNEL; i++) tx_spi_msg.data[i] = gain_mass[i];
           CRC_ResetDR();
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
           SendF207Message((uint16_t*)&tx_spi_msg,MAX_CHANNEL + 4);
      break;
      
      case MODE_SET_GAIN:
           for(int i = 0; i < MAX_CHANNEL; i++) gain_mass[i] = rx_spi_msg.data[i];
      break;
      
      
    }

    SysTick->CTRL |= SysTick_CTRL_ENABLE;
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  

////////////////////////////////////////////////////////////////////////////////           
        
  }
}






