#include "main.h"
#include "at24c512.h"
#include "nand_hw_driver.h"

extern total_info_struct     t_info;
extern bad_block_map_struct  *pmap_bb;
extern super_block_struct    *prsb;
extern super_block_struct    *pwsb;
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern alarm_struct           alarm_data;


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
  
} 