#include "main.h"
#include "at24c512.h"
#include "ethernet.h"
#include "i2c_f415.h"
#include "nand_hw_driver.h"

extern tab_struct               tab;
extern total_info_struct        t_info;
extern udp_message_struct       tx_udp_msg[2];
extern udp_message_struct       rx_udp_msg[2];
extern alarm_struct             alarm_data; 
extern ethernet_initial_struct  eth_ini_dat;
extern bad_block_map_struct*    pmap_bb;
extern total_work_struct        tws;
extern int                      time_sync_status;

 
void cmd_handler(int id)
{
    int i;
 
    switch(rx_udp_msg[id].msg_id)
    {
      case CHECK_CONNECT: 

           SendMessage(id,CHECK_CONNECT + 100,NULL,0);
           
      break;
      
      case SET_TIME: 
        
        // SetTime((uint8_t*)&rx_udp_msg[id].data[0]);
        // SendMessage(id,SET_TIME + 100,NULL,0);
        
           Set_SNTP_Timer();
           time_sync_status = 1;
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
      
      case NEW:


         f415_WriteMessage(F415_STOP_STREAM,NULL,0);
         GPIO_WriteBit(GPIOI,GPIO_Pin_0,Bit_SET); 
         Data_Config(DATA_CONFIG_NEW);
         nand_erase_super_block(0,alarm_data.PageAdressErase);   
         tab[0].time[0]  = GetTime();    
         SendMessage(id,NEW + 100,NULL,0);
         
      break;
      
      case START_AUDIO_STREAM:

          if((t_info.f207_mode != F207_AUDIO_STREAM_MODE) && (time_sync_status != 0))
          {
            t_info.f207_mode = F207_AUDIO_STREAM_MODE;
            SendMessage(id,START_AUDIO_STREAM + 100,NULL,0);
            SaveTotalTimeTotalMode(F207_AUDIO_STREAM_MODE);
            f415_WriteMessage(F415_START_STREAM,NULL,0);
            GPIO_WriteBit(GPIOI,GPIO_Pin_0,Bit_SET); 
          }
         
      break;
      
      case STOP_AUDIO_STREAM: 
    
          t_info.f207_mode = F207_STOP_MODE;
          f415_WriteMessage(F415_STOP_STREAM,NULL,0);            
          SaveTotalTimeTotalMode(F207_STOP_MODE);
          SendMessage(id,STOP_AUDIO_STREAM + 100,NULL,0);
          
      break;  
      
      case SET_GAIN:
        
      break;
      
      case GET_SYS_INFO:
          
          SendMessage(id,GET_SYS_INFO + 100,&tab,sizeof(tab_struct));  
        
      break;
       
      case GET_TOTAL_INFO:
        
        t_info.nand_work_counter[0] = tws.total_cucle[0];
        t_info.nand_work_counter[1] = tws.total_cucle[1];
        t_info.total_time           = tws.total_time;
        t_info.current_time         = GetTime();
        
        SendMessage(id,GET_TOTAL_INFO + 100,&t_info,sizeof(total_info_struct)); 
                
      break;
      
 
      case GET_ETH_PARAM:
        
       //   t_info.at24_error = AT24_Read(0,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
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
        //info_ini.at24_error = AT24_Write(AT45ADR_ETH_PARAM,(uint8_t*)&eth_ini_dat,sizeof(eth_ini_dat));
        SendMessage(id,SET_ETH_PARAM + 100,NULL,0);
        
      break;    
      
      case FORMAT_MAP_BB:
        
        nand_erase_block(0,MAP_BB_ADDRES * 64); // первая копия карты
        nand_erase_block(1,MAP_BB_ADDRES * 64); // вторая копия карты
        
        for(i = 0; i < MAP_BB_MAX_BLOCK; i++) pmap_bb->block_address[i] = BLOCK_GOOD;

        pmap_bb->crc = crc32((uint8_t*)pmap_bb,MAP_BB_MAX_BLOCK,MAP_BB_MAX_BLOCK); 
        
        nand_16bit_write_page(0,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); // первая копия карты
        nand_16bit_write_page(1,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); // вторая копия карты
       
        SendMessage(id,FORMAT_MAP_BB + 100,NULL,0);
        
      break; 
      
      case FORMAT_TWS:
        
        nand_erase_block(0,TWS_ADDRES * 64); // первая копия 
        nand_erase_block(1,TWS_ADDRES * 64); // вторая копия 

        memset(&tws,0,sizeof(total_work_struct));
  
        tws.crc  = crc32((uint8_t*)&tws,sizeof(total_work_struct)-4,sizeof(total_work_struct)); 
   
        nand_16bit_write_page_ext(0,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); // первая копия 
        nand_16bit_write_page_ext(1,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); // вторая копия 
        
        SendMessage(id,FORMAT_TWS + 100,NULL,0);
        
       break;     
        
       
      
    }
}
