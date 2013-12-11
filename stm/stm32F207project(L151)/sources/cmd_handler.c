#include "main.h"
#include "at24c512.h"
#include "ethernet.h"

#include "spi_f207.h"

extern tab_struct               tab;
extern system_info_struct       sys_info;
extern udp_message_struct       tx_udp_msg[2];
extern udp_message_struct       rx_udp_msg[2];
extern spi_message_struct       tx_spi_msg;
extern spi_message_struct       rx_spi_msg; 
extern alarm_struct             alarm_data; 
extern ethernet_initial_struct  eth_ini_dat;
extern bad_block_map_struct*    pmap_bb;
extern l151_info_struct         l151_info;
extern uint32_t                 max_page_in_nand;

 
void cmd_handler(int id)
{
  int i;
  uint8_t index,index_pos;
  info_1_struct i1s;
  
    switch(rx_udp_msg[id].msg_id)
    {
      case CHECK_CONNECT: 

           SendMessage(id,CHECK_CONNECT + 100,NULL,0);
           
      break;
      
      case SET_TIME: 
        
           SetTime((uint8_t*)&rx_udp_msg[id].data[0]);
           SendMessage(id,SET_TIME + 100,NULL,0);
           
      break;
    
      case GET_TIME: 
        
           *((uint64_t*)&tx_udp_msg[id].data[0]) = GetTime();
            SendMessage(id,GET_TIME + 100,NULL,8);
           
      break;
      
           
      case GET_SB_HEADER: 
        
           GetSuperBlockHeader(*((uint32_t*)&rx_udp_msg[id].data[0]),*((uint32_t*)&rx_udp_msg[id].data[4]));
           
      break;
      
      case GET_PAGE: 
          
           GetSuperBlockPage(*((uint32_t*)&rx_udp_msg[id].data[0]));
 
       break;
      
          
      case START_AUDIO_STREAM:
        
           if(sys_info.L151_mode != MODE_STREAM)
           {
             SPI_RxInt_Config(DISABLE);
             tx_spi_msg.msg_len = 1;
             tx_spi_msg.msg_id  = MODE_STREAM;
             CRC_ResetDR();        
             tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
             SendL151Message((uint16_t*)&tx_spi_msg,4);
             SPI_RxDma_Config(ENABLE);
             
             for(i = 0; i < 100; i++)
             {
               if(sys_info.L151_mode == MODE_STREAM)
               {
                 SendMessage(id,START_AUDIO_STREAM + 100,NULL,0);
                 break;
               }
               else
               {
                 delay(10);
               }
             }
           }
           
      break;
      
      case STOP_AUDIO_STREAM: 

           SendMessage(id,STOP_AUDIO_STREAM + 100,NULL,0);
           SPI3->DR = DLE;
           SPI_RxDma_Config(DISABLE);
           delay(1000);      
           SPI_RxInt_Config(ENABLE); 
           
      break;  
      
      case SET_GAIN: 
      
           SendMessage(id,SET_GAIN + 100,NULL,0);
           SPI3->DR = DLE;
           SPI_RxDma_Config(DISABLE);
           delay(1000); 
           SPI_RxInt_Config(ENABLE); 
           tx_spi_msg.msg_len = 9;
           tx_spi_msg.msg_id = MODE_SET_GAIN;
           for(int i = 0; i < MAX_CHANNEL; i++) tx_spi_msg.data[i] = tx_udp_msg[id].data[i];
           CRC_ResetDR();        
           tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len); 
           SendL151Message((uint16_t*)&tx_spi_msg,20);

      break;
      
      case SET_SYS_INFO_0:
                       
           SendMessage(id,SET_SYS_INFO_0 + 100,NULL,0);
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
       
      case GET_SYS_INFO_0:
        
           SendMessage(id,GET_SYS_INFO_0 + 100,&l151_info,sizeof(l151_info));
     
      break;
        
      case GET_SYS_INFO_1:
          
           i1s.unit_index = alarm_data.index;
           i1s.unit_mode = sys_info.L151_mode;
           i1s.super_block_real_read = alarm_data.super_block_real_read;
           i1s.super_block_real_write = alarm_data.super_block_real_write;
           i1s.super_block_begin_time = alarm_data.super_block_time[0];
           i1s.super_block_end_time = alarm_data.super_block_time[1];
           i1s.super_block_addres = alarm_data.PageAddress;
           i1s.err_crc[0] = alarm_data.err_crc_binar;
           i1s.err_crc[1] = alarm_data.err_crc_wr_Nand;
           i1s.err_crc[2] = alarm_data.err_crc_rd_Nand;
           SendMessage(id,GET_SYS_INFO_1 + 100,&i1s,sizeof(i1s));
   
      break;
      
      case GET_SYS_INFO_2:
        
           SendMessage(id,GET_SYS_INFO_2 + 100,tab,sizeof(tab_struct));
           
      break;
      
      case GET_BM:

          index = alarm_data.bookmark_index;
          index_pos = rx_udp_msg[id].data[0] + 1;
          while(index_pos-- > 0) index = (index - 1) & 0xf;
          SendMessage(id,GET_BM + 100,&alarm_data.bms[index],sizeof(tab_struct));
 
      break;      
      
      case SET_FIXED: 
        
           if(alarm_data.fixed_index[0] != rx_udp_msg[id].data[0])
           {
             alarm_data.close_file_flag = 1;
             alarm_data.fixed_index[0] = rx_udp_msg[id].data[0];
           }
           
           SendMessage(id,SET_FIXED + 100,NULL,0);
                   
      break;
      
      case GET_FIXED: 
     
           SendMessage(id,GET_FIXED + 100,&alarm_data.fixed_index[1],1);
                
      break;
      
      case GET_ETH_PARAM:
        
           AT45_Read(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
           SendMessage(id,GET_ETH_PARAM + 100,&eth_ini_dat,sizeof(eth_ini_dat));
           
      break;

      case SET_ETH_PARAM:
        
        switch(rx_udp_msg[id].data[0])
        {
          case SET_MAC:          memcpy(&eth_ini_dat.MAC_ADR,&rx_udp_msg[id].data[1],6);           break;
          case SET_GW:           memcpy(&eth_ini_dat.GW_ADR,&rx_udp_msg[id].data[1],4);            break;
          case SET_IP:           memcpy(&eth_ini_dat.IP_ADR,&rx_udp_msg[id].data[1],4);            break;
          case SET_MASK:         memcpy(&eth_ini_dat.MASK,&rx_udp_msg[id].data[1],4);              break;          
          case SET_UDP_RX1_PORT: eth_ini_dat.UDP_RX_PORT[SERV] = *((uint32_t*)&rx_udp_msg[id].data[1]); break;  
          case SET_UDP_RX2_PORT: eth_ini_dat.UDP_RX_PORT[PTUK] = *((uint32_t*)&rx_udp_msg[id].data[1]); break;  
        }
        
        eth_ini_dat.crc = crc32(&eth_ini_dat,sizeof(eth_ini_dat)-4,sizeof(eth_ini_dat)-4);
        AT45_Write(AT45ADR_ETH_PARAM,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
        SendMessage(id,SET_ETH_PARAM + 100,NULL,0);
        
      break;    
      
      case SET_MAP_BB:
        
        pmap_bb->bad_block_number = 0;
        for(i = 0; i < max_page_in_nand; i++) pmap_bb->block_address[i] = BLOCK_GOOD;
        CRC_ResetDR();
        pmap_bb->crc = CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4); 
        AT45_Write(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));
        SendMessage(id,SET_MAP_BB + 100,NULL,0);
        
      break;    
       
    }
}
