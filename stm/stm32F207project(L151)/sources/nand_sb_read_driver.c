#include "main.h"
#include "ethernet.h"
#include "nand_hw_driver.h"

#define ONE_PAGE   1
#define ALL_PAGE   0

extern                         udp_message_struct  tx_udp_msg[MAX_UDP_SOCK];
extern alarm_struct            alarm_data;
extern super_block_struct*     prsb;
static page_header_struct      ph;

static int GetSuperBlockAdr(uint32_t *padr, uint32_t sb_num);
static void ReadSuperBlockHeader(uint32_t adr, int read_mode);
static int CheckSuperBlock(void);
extern int rew_status;
extern int wait_state;


void GetSuperBlockPage(uint32_t page_address)
{
  ph.page_index = 4;
  ph.page_address = page_address;
}

void GetSuperBlockHeader(uint32_t unit_index, uint32_t sb_index)
{
  uint32_t page_adr;
	
  #define SB_OK                1
  #define SB_NOT_FOUND         2
  #define SB_INVALID           3
  
  #pragma pack(push,1)   
  struct super_block_header_struct
  {
     uint8_t   status;
     uint64_t  time_open;
     uint64_t  time_close;
     uint32_t  sb_num;
     uint32_t  page_real_write;

  } *psbh = (struct super_block_header_struct*) &tx_udp_msg[SERV].data[0];
  #pragma pack(pop) 
  
  ph.unit_index = unit_index;
  
  if(sb_index == prsb->sb_num+1)
  {
    if(prsb->super_block_next) 
    {
      page_adr = prsb->super_block_next;
    }
    else
    {
      psbh->status = SB_NOT_FOUND;
      SendMessageExt(SERV,GET_SB_HEADER+100,NULL,sizeof(struct super_block_header_struct));
      return;
      
    }
  }
  else if(!GetSuperBlockAdr(&page_adr,sb_index)) 
  {
    psbh->status = SB_NOT_FOUND;
    SendMessageExt(SERV,GET_SB_HEADER+100,NULL,sizeof(struct super_block_header_struct));
    return;  
  }
  
    ReadSuperBlockHeader(page_adr,ALL_PAGE);
  
    if(CheckSuperBlock()) 
    { 
      psbh->status = SB_OK;
      psbh->time_open = prsb->time_open;
      psbh->time_close = prsb->time_close; 
      psbh->sb_num = prsb->sb_num;
      psbh->page_real_write = prsb->page_real_write;
    }
    else
    {
      psbh->status = SB_INVALID;
    }
  
   SendMessageExt(SERV,GET_SB_HEADER+100,NULL,sizeof(struct super_block_header_struct));
}    

uint8_t rx_buffer[4096];

void nand_sb_read_handler(void)
{
  uint32_t  i;
  uint32_t  page_adr;
  uint32_t* pin;
  uint32_t* pout;
  
  uint32_t  half_adpcm_size = ADPCM_BLOCK_SIZE / 4; 
  adpcm_page_struct* padpcm = (adpcm_page_struct*)rx_buffer;
  
  if(ph.page_index > 0)
  {
    
    page_adr = prsb->ips[ph.page_address].page_address;
    
    switch(ph.page_index)
    {
       case 4:  
         
         nand_16bit_read_page(ph.unit_index,(uint16_t*)rx_buffer,page_adr);
         nand_16bit_read_page(ph.unit_index,(uint16_t*)rx_buffer,page_adr);
          
         if(crc32(padpcm,ADPCM_BLOCK_SIZE-4,ADPCM_BLOCK_SIZE-4) != padpcm->crc) 
         {
           ph.page_index = 0;
           alarm_data.err_crc_rd_Nand++;
           pin  = (uint32_t*)&tx_udp_msg[SERV].data[PAGE_HEADER_SIZE];
           memcpy(&tx_udp_msg[SERV].data[0],&ph,PAGE_HEADER_SIZE);
           for(i = 0; i < (half_adpcm_size / 4); i++) *(pin++) = 0;
         }
         else
         {
           memcpy(&tx_udp_msg[SERV].data[0],&ph,PAGE_HEADER_SIZE);
           ph.page_index = 3;
           pin  = (uint32_t*)&tx_udp_msg[SERV].data[PAGE_HEADER_SIZE];
           pout = (uint32_t*)&rx_buffer[0];
           for(i = 0; i < (half_adpcm_size / 4); i++) *(pin++) = *(pout++);
         }

       break;
       
       case 3:

         memcpy(&tx_udp_msg[SERV].data[0],&ph,PAGE_HEADER_SIZE);
         ph.page_index = 2;
         pin  = (uint32_t*)&tx_udp_msg[SERV].data[PAGE_HEADER_SIZE];
         pout = (uint32_t*)&rx_buffer[half_adpcm_size];
         for(i = 0; i < (half_adpcm_size / 4); i++) *(pin++) = *(pout++);

       break;

       case 2:
         
         memcpy(&tx_udp_msg[SERV].data[0],&ph,PAGE_HEADER_SIZE);
         ph.page_index = 1;
         pin  = (uint32_t*)&tx_udp_msg[SERV].data[PAGE_HEADER_SIZE];
         pout = (uint32_t*)&rx_buffer[half_adpcm_size*2];
         for(i = 0; i < (half_adpcm_size / 4); i++) *(pin++) = *(pout++);

       break;

       case 1:
         
         memcpy(&tx_udp_msg[SERV].data[0],&ph,PAGE_HEADER_SIZE);
         ph.page_index = 0;
         pin  = (uint32_t*)&tx_udp_msg[SERV].data[PAGE_HEADER_SIZE];
         pout = (uint32_t*)&rx_buffer[half_adpcm_size*3];
         for(i = 0; i < (half_adpcm_size / 4); i++) *(pin++) = *(pout++);

       break;
       
    } 
    SendMessageExt(SERV,GET_PAGE+100,NULL,PAGE_HEADER_SIZE + half_adpcm_size);  
  }
}


static int GetSuperBlockAdr(uint32_t *padr, uint32_t sb_num)
{

  uint32_t adr = alarm_data.super_block_begin;
  
  while(1)
  {
    ReadSuperBlockHeader(adr,ONE_PAGE);
  
    if(prsb->sb_num != sb_num)
    {
      if(prsb->id != SUPER_BLOCK_ID) return 0;
      else if(prsb->super_block_next == NULL) return 0;
           else
           {
             adr = prsb->super_block_next;
           }
    }
    else break;
  }
  
  *padr = adr;
  
  return 1;
}


static void ReadSuperBlockHeader(uint32_t adr, int read_mode)
{
   uint32_t index = ph.unit_index;
   
   uint8_t *pbuffer = (uint8_t*)prsb;
  
    for(int i = 0; i < 64; i++) 
    {
      nand_8bit_read_page(index,pbuffer,adr+i);
      pbuffer+=2048;
      if(read_mode) break;
    }
}


static int CheckSuperBlock(void)
{
  if(prsb->id == SUPER_BLOCK_ID) return 1;
  else return 0;
}