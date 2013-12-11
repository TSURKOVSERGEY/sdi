#include "main.h"
#include "ethernet.h"
#include "spi_f207.h"

uint16_t rx_spi_msg_size;

extern system_info_struct    sys_info;
extern spi_message_struct    tx_spi_msg;
extern spi_message_struct    rx_spi_msg; 
//extern udp_message_struct    tx_udp_msg;
//extern udp_message_struct    rx_udp_msg;
//extern struct tcp_pcb *pout_pcb;
//extern struct pbuf *pOut;
//extern struct ip_addr ip_addr_tx;  
extern l151_info_struct  l151_info;

extern __IO uint32_t LocalTime;

          
void SPI3_IRQHandler(void)
{
  static uint32_t mode = DLE_STX;
  static uint16_t data_temp[2] = {0,0};
  static uint32_t *pdata = (uint32_t*)&data_temp[0];
  static uint8_t wrr;  
  static uint16_t *prx_spi_msg;
  uint32_t crc;
  
  data_temp[1] = data_temp[0];
  data_temp[0] = SPI3->DR;
  
  if(mode == DLE_STX)
  {
    if(*pdata == DLE_STX)
    {
      mode = DLE_ETX;
      prx_spi_msg = (uint16_t*) &rx_spi_msg;
      wrr = 0;
      return;
    }
  }
  else if(mode == DLE_ETX)
  {
    if(*pdata == DLE_ETX)
    {
       mode = DLE_STX;
       *pdata = 0;

       CRC_ResetDR();
       crc = CRC_CalcBlockCRC(((uint32_t*)&rx_spi_msg)+1,(uint32_t)rx_spi_msg.msg_len & 0xff);
       
       if(crc != rx_spi_msg.msg_crc) return;
       else 
       {
         
////////////////////////////////////////////////////////////////////////////////
         
         switch(sys_info.L151_mode = rx_spi_msg.msg_id)
         {
            case MODE_GET_GAIN:
               for(int i = 0; i < MAX_CHANNEL; i++) sys_info.L151_gain[i] = rx_spi_msg.data[i]; 
               tx_spi_msg.msg_len = 1;
               tx_spi_msg.msg_id  = MODE_CMD;
               CRC_ResetDR();        
               tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
               SendL151Message((uint16_t*)&tx_spi_msg,4);
               
               {
                 
                 for(int i = 0; i < 16; i++) l151_info.L151_gain[i] = sys_info.L151_gain[i];

                 l151_info.L151_mode = sys_info.L151_mode;             
                 l151_info.L151_initial_error = sys_info.L151_initial_error;  
                 l151_info.L151_stream_error = sys_info.L151_stream_error;  
               
               }
               
               memset(&tx_spi_msg,0,sizeof(tx_spi_msg));

            break;          
         }

////////////////////////////////////////////////////////////////////////////////
       }
    }
    else if(*pdata == DLE_DLE)
    {
      data_temp[0] = 0;
    }
    else if(wrr++ < rx_spi_msg_size) 
    {
      *(prx_spi_msg++) = data_temp[0];
    }
  }
}
