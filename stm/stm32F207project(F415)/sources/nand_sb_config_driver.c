#include "main.h"
#include "at24c512.h"
#include "nand_hw_driver.h"
#include "i2c_f415.h"

extern tab_struct             tab;
extern total_info_struct     t_info;
extern bad_block_map_struct  *pmap_bb;
extern super_block_struct    *prsb;
extern super_block_struct    *pwsb;
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern alarm_struct           alarm_data;
extern total_work_struct      tws;
extern int                    time_sync_status;

int Restart_Handler(void)
{
  if(time_sync_status == 0) return 0;
  
  if(tws.mode == F207_AUDIO_STREAM_MODE)
  {
    if(t_info.f207_mode != F207_AUDIO_STREAM_MODE)
    {
      f415_WriteMessage(F415_START_STREAM,NULL,0);
      GPIO_WriteBit(GPIOI,GPIO_Pin_0,Bit_SET); 
    }
  }
  
  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// процедура поиска последнего записанного файла в цепочке связанного списка
////////////////////////////////////////////////////////////////////////////////
uint32_t GetLastSuperBlock(uint32_t index, uint32_t mode)
{
NEXT_STEP: // рекурсия глючит поэтому "goto"
  
   nand_8bit_read_page(index,(uint8_t*)pwsb,alarm_data.PageAdressWrite);
   
   if(pwsb->crc_header != crc32((uint8_t*)pwsb,SB_HEADER_SIZE,SB_HEADER_SIZE))
   {
     if(alarm_data.super_block_real_write > 0) // если в цепочке был хоть один файл 
     {
       alarm_data.super_block_real_write+= 1;
       if(mode == ERASE_MODE) nand_erase_super_block(index,alarm_data.PageAdressErase);  
       
       tab[index].unit_index = alarm_data.index;
       tab[index].time[0]  = alarm_data.super_block_time[0];
       tab[index].time[1]  = alarm_data.super_block_time[1];
       tab[index].sbrw = alarm_data.super_block_real_write;  
      
       return 0;
     }
     else if((alarm_data.PageAdressWrite) >= MAX_PAGE_IN_NAND) // проверка конца памяти
     {
       alarm_data.PageAdressWrite = BEGIN_PAGE & 0xffffffc0;
       alarm_data.PageAdressErase = BEGIN_PAGE & 0xffffffc0;
       
   
       if(mode == ERASE_MODE) nand_erase_super_block(index,alarm_data.PageAdressErase);  
       return 1; // последний файл не найден 
     }
     else
     {
       alarm_data.PageAdressWrite+= 64;  // ищу файл дальше
       goto NEXT_STEP;
     }
   }
   else // контрольная сумма заголовка файла совпала 
   {
     alarm_data.super_block_real_write = pwsb->sb_num;   // индекс последнего записанного файла     
     alarm_data.super_block_time[1] = pwsb->time_open;   // время последнего записанного файла   
      
     if((pwsb->sb_num == 0) && (alarm_data.super_block_real_write == 0)) // если это первый файл
     {
       alarm_data.super_block_begin = alarm_data.PageAdressWrite;
       alarm_data.super_block_time[0] = pwsb->time_open;
     }
          
     if(pwsb->super_block_next != NULL) // ищу файл дальше
     {
       alarm_data.PageAdressWrite = pwsb->super_block_next;
       alarm_data.PageAdressErase = pwsb->super_block_next;
       goto NEXT_STEP;
     }
     else // проверка на последний файл
     {
       alarm_data.super_block_real_write+= 1;
       tab[index].unit_index = alarm_data.index;
       tab[index].time[0]  = alarm_data.super_block_time[0];
       tab[index].time[1]  = alarm_data.super_block_time[1];
       tab[index].sbrw = alarm_data.super_block_real_write;  
       return 0;
     }
 
   }
  
}

void SuperBlock_Config(void)
{
  int i;
  
  prsb = (super_block_struct*)sram_bank3;
  pwsb = (super_block_struct*)((uint8_t*)prsb + (66*2048));
  pmap_bb = (bad_block_map_struct*) ((uint8_t*)pwsb + (66*2048));
 
  padpcm[0][0] = (adpcm_page_struct*)((uint8_t*)pmap_bb+sizeof(bad_block_map_struct));
  for(i = 1; i < MAX_CHANNEL; i ++) padpcm[0][i] = (adpcm_page_struct*)((uint8_t*)padpcm[0][i-1]+sizeof(adpcm_page_struct));
  
  padpcm[1][0] = (adpcm_page_struct*)((uint8_t*)padpcm[0][15]+sizeof(adpcm_page_struct));
  for(i = 1; i < MAX_CHANNEL; i ++) padpcm[1][i] = (adpcm_page_struct*)((uint8_t*)padpcm[1][i-1]+sizeof(adpcm_page_struct)); 
   
  
  memset(prsb,0,sizeof(super_block_struct));
  memset(pwsb,0,sizeof(super_block_struct));
  memset(pmap_bb,0,sizeof(bad_block_map_struct));
  memset(padpcm[0][0],0,sizeof(adpcm_page_struct)*MAX_CHANNEL*2);
  
  // считываю карту поврежденных блоков ( две копии )
  for(i = 0; i < 2; i++)
  { 
    t_info.read_map_bb_error[i] = 0;
     
    nand_16bit_read_page(i,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); 

    if(pmap_bb->crc !=  crc32((uint8_t*)pmap_bb,MAP_BB_MAX_BLOCK,MAP_BB_MAX_BLOCK)) 
    {
       t_info.read_map_bb_error[i] = 1;
      
       if(i == 1)
       {
         if(t_info.read_map_bb_error[0] == 0)
         {
           nand_16bit_read_page(0,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); 
         }
       }
      
    }
  }
  
  // подсчет поврежденных блоков FLASH
  CRC_ResetDR();
  if(pmap_bb->crc == CRC_CalcBlockCRC((uint32_t*)pmap_bb,(MAP_BB_MAX_BLOCK / 4))) 
  {
    for(int i = 0; i < MAP_BB_MAX_BLOCK; i++)
    {
      if(pmap_bb->block_address[i] != BLOCK_GOOD)
      {
          t_info.bad_block_number++;
      }
    }
  }
  
  
  Data_Config(DATA_CONFIG_LOAD);   
  
  t_info.nand_fs_error[tws.index^1] = GetLastSuperBlock(tws.index^1,ERASE_MODE);
  
  Data_Config(DATA_CONFIG_LOAD);   
  
  t_info.nand_fs_error[tws.index] = GetLastSuperBlock(tws.index,ERASE_MODE);
  
} 