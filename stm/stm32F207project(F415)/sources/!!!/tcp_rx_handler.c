#include "main.h"
#include "ethernet.h"

#include "spi_f207.h"

extern struct udp_pcb        *pudp_pcb;
extern struct pbuf           *pOut;
extern struct ip_addr        ip_addr_tx;  

extern super_block_struct    *prsb;
extern super_block_struct    *pwsb;
extern system_info_struct    sys_info;
extern tcp_message_struct    tx_tcp_msg;
extern tcp_message_struct    rx_tcp_msg;
extern spi_message_struct    tx_spi_msg;
extern spi_message_struct    rx_spi_msg; 
extern alarm_struct          alarm_data; 
extern struct                tcp_pcb *pout_pcb;
extern int                   index_status;
extern int                   wait_state;

struct info
{
  uint32_t unit_index;
  uint32_t unit_mode;
  uint32_t super_block_real_read;
  uint32_t super_block_real_write;
  uint64_t super_block_begin_time;
  uint64_t super_block_end_time;
  uint32_t super_block_addres;
  uint32_t crc;
} *pinfo = (struct info*) &tx_tcp_msg.data[0];

struct adr
{
  uint32_t file_begin;
  uint32_t file_end;
} *padr = (struct adr*) &rx_tcp_msg.data[0];
          
void tcp_rx_handler(void)
{
    switch(rx_tcp_msg.msg_id)
    {
      case CHECK_CONNECT: 
        
           tx_tcp_msg.msg_id  = CHECK_CONNECT + 100;
           tx_tcp_msg.msg_len = 0;
           tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len); 
           send_data(&tx_tcp_msg,tx_tcp_msg.msg_len + TCP_HEADER_SIZE);
           
      break;
      
      case SET_TIME: 
        
           SetTime((uint8_t*)&rx_tcp_msg.data[0]);
           
      break;
    
      case START_AUDIO_STREAM:
        
           SPI_RxInt_Config(DISABLE);
           tx_spi_msg.msg_len = 1;
           tx_spi_msg.msg_id  = MODE_STREAM;
           CRC_ResetDR();        
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
           SendL151Message((uint16_t*)&tx_spi_msg,4);
           SPI_RxDma_Config(ENABLE);
          
      break;
      
      case STOP_AUDIO_STREAM: 
        
           SPI3->DR = DLE;
           SPI_RxDma_Config(DISABLE);
           delay(1000);      
           SPI_RxInt_Config(ENABLE); 
        
      break;  
      
      case SET_GAIN: 
        
           SPI3->DR = DLE;
           SPI_RxDma_Config(DISABLE);
           delay(1000); 
           SPI_RxInt_Config(ENABLE); 
           tx_spi_msg.msg_len = 9;
           tx_spi_msg.msg_id = MODE_SET_GAIN;
           for(int i = 0; i < MAX_CHANNEL; i++) tx_spi_msg.data[i] = rx_tcp_msg.data[i];
           CRC_ResetDR();        
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
           SendL151Message((uint16_t*)&tx_spi_msg,20);
           
      break;
      
      case GET_SBLOCK: 
        
           SetReadSuperBlock(padr->file_begin,padr->file_end);

      break;
  
      case GET_SYS_INFO_0:
            
           SPI3->DR = DLE;
           SPI_RxDma_Config(DISABLE);
           delay(1000); 
           SPI_RxInt_Config(ENABLE); 
           tx_spi_msg.msg_len = 1;
           tx_spi_msg.msg_id = MODE_GET_GAIN;
           CRC_ResetDR();        
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
           SendL151Message((uint16_t*)&tx_spi_msg,4);
           
      break;
        
      case GET_SYS_INFO_1:
          
           pinfo->unit_index = alarm_data.index;
           pinfo->unit_mode = sys_info.L151_mode;
           pinfo->super_block_real_read = alarm_data.super_block_real_read;
           pinfo->super_block_real_write = alarm_data.super_block_real_write;
           pinfo->super_block_begin_time = alarm_data.super_block_time[0];
           pinfo->super_block_end_time = alarm_data.super_block_time[1];
           pinfo->super_block_addres = alarm_data.PageAddress;
           pinfo->crc = alarm_data.crc;
            
           tx_tcp_msg.msg_id  = GET_SYS_INFO_1 + 100;
           tx_tcp_msg.msg_len = sizeof(struct info);
           tx_tcp_msg.msg_crc = crc32((uint8_t*)&tx_tcp_msg+4,(TCP_HEADER_SIZE-4) + tx_tcp_msg.msg_len); 
           send_data(&tx_tcp_msg,tx_tcp_msg.msg_len+TCP_HEADER_SIZE);
   
           GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
            

      break;
  
    }
}
