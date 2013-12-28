#include "main.h"
#include "at24c512.h"
#include "ethernet.h"
#include "i2c_f415.h"

extern tab_struct               tab;
extern initial_info_struct      info_ini;
extern udp_message_struct       tx_udp_msg[2];
extern udp_message_struct       rx_udp_msg[2];
extern alarm_struct             alarm_data; 
extern ethernet_initial_struct  eth_ini_dat;
extern bad_block_map_struct*    pmap_bb;
 
void cmd_handler(int id)
{
    int i;
 
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
        
          f415_WriteMessage(F415_START_STREAM,NULL,0);
          SendMessage(id,START_AUDIO_STREAM + 100,NULL,0);
       
      break;
      
      case STOP_AUDIO_STREAM: 

          f415_WriteMessage(F415_STOP_STREAM,NULL,0);
          SendMessage(id,STOP_AUDIO_STREAM + 100,NULL,0);
           
      break;  
      
      case SET_GAIN:
        
      break;
      
      case GET_SYS_INFO:
          
          SendMessage(id,GET_SYS_INFO + 100,tab,sizeof(tab_struct));  
        
       break;
 
      case GET_ETH_PARAM:
        
          info_ini.at24_error = AT24_Read(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
          SendMessage(id,GET_ETH_PARAM + 100,&eth_ini_dat,sizeof(eth_ini_dat));
           
      break;

      case SET_ETH_PARAM:
        
        switch(rx_udp_msg[id].data[0])
        {
          case SET_MAC:          memcpy(&eth_ini_dat.MAC_ADR,&rx_udp_msg[id].data[1],6);           break;
          case SET_GW:           memcpy(&eth_ini_dat.GW_ADR,&rx_udp_msg[id].data[1],4);            break;
          case SET_IP:           memcpy(&eth_ini_dat.IP_ADR,&rx_udp_msg[id].data[1],4);            break;
          case SET_MASK:         memcpy(&eth_ini_dat.MASK,&rx_udp_msg[id].data[1],4);              break;          
          case SET_UDP_RX1_PORT: eth_ini_dat.UDP_RX_PORT[id] = *((uint32_t*)&rx_udp_msg[id].data[1]); break;  
          case SET_UDP_RX2_PORT: eth_ini_dat.UDP_RX_PORT[id] = *((uint32_t*)&rx_udp_msg[id].data[1]); break;  
        }
        
        eth_ini_dat.crc = crc32(&eth_ini_dat,sizeof(eth_ini_dat)-4,sizeof(eth_ini_dat)-4);
        info_ini.at24_error = AT24_Write(AT45ADR_ETH_PARAM,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
        SendMessage(id,SET_ETH_PARAM + 100,NULL,0);
        
      break;    
      
      case SET_MAP_BB:
        
        pmap_bb->bad_block_number = 0;
        for(i = 0; i < MAX_DATA_PAGE; i++) pmap_bb->block_address[i] = BLOCK_GOOD;
        CRC_ResetDR();
        pmap_bb->crc = CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4); 
        info_ini.at24_error = AT24_Write(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));
        SendMessage(id,SET_MAP_BB + 100,NULL,0);
        
      break;    
       
      
    }
}
