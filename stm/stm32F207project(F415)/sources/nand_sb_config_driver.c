#include "main.h"
#include "at24c512.h"

extern initial_info_struct    info_ini;
extern bad_block_map_struct  *pmap_bb;
extern super_block_struct    *prsb;
extern super_block_struct    *pwsb;
extern adpcm_page_struct     *padpcm[2][MAX_CHANNEL];
extern alarm_struct           alarm_data;
extern adpcm_page_ctrl_struct adpcm_ctrl[MAX_CHANNEL];

uint32_t SuperBlock_Config(void)
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
  
  memset(adpcm_ctrl,0,sizeof(adpcm_ctrl));
  memset(&alarm_data,0,sizeof(alarm_data));

  if(info_ini.at24_error = AT24_Read(AT45ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct))) return 0;

  CRC_ResetDR();
  
  if(pmap_bb->crc != CRC_CalcBlockCRC((uint32_t*)pmap_bb,(sizeof(bad_block_map_struct)-4)/4)) 
  {
    info_ini.map_bb_error = 1;
    return 0;
  }
  
  return 1;
} 