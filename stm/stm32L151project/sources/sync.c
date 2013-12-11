#include "stm32l1xx_conf.h"
#include "spi_L151.h"
#include "stdio.h"
#include "stdlib.h"
#include "main.h"

extern uint16_t tx_msg[TX_MSG_SIZE];

#pragma pack(push,1)   
extern struct spi_message
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint16_t data[256];
 } spi_msg, *pspi_msg;  
#pragma pack(pop)


uint16_t Synchronization(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  uint16_t rx_msg[RX_MSG_SIZE];
  uint16_t data_temp[2] = {0,0};
  int32_t  mode = DLE_STX;
  uint8_t  wrr;  
  uint32_t crc;
  uint32_t *pdata = (uint32_t*)&data_temp[0];
  pspi_msg = (struct spi_message*)&rx_msg;
  
  tx_msg[2] = MODE_SYNC;
    
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
          wrr = 0;
        }
      }
      else if(mode == DLE_ETX)
      {
      
        if(*pdata == DLE_ETX)
        {
           *pdata = 0;
            mode = DLE_STX;
           CRC_ResetDR();
           crc = CRC_CalcBlockCRC(((uint32_t*)pspi_msg)+1,(uint32_t)pspi_msg->msg_len&0xff);
           
           if(crc == pspi_msg->msg_crc) 
           {
             if(pspi_msg->msg_id == MODE_CMD)
             {
               break;
             }
           }
        }
        else if(*pdata == DLE_DLE)
        {
          data_temp[0] = 0;
        }
        else
        {
          if(wrr < RX_MSG_SIZE) rx_msg[wrr++] = data_temp[0];
        }
      }
    }
    
      
  SysTick->CTRL &= SysTick_CTRL_ENABLE_Msk;
  
  return pspi_msg->msg_id;
}





