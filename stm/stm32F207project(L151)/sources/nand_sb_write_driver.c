#include "main.h"
#include "ethernet.h"
#include "nand_hw_driver.h"

#define REWRITE_BLOCK        1
#define REWRITE_BLOCK_NEXT   2

extern udp_message_struct      tx_udp_msg[MAX_UDP_SOCK];
extern alarm_struct            alarm_data;
extern super_block_struct*     pwsb;
extern super_block_struct*     prsb;
extern bad_block_map_struct*   pmap_bb;
extern adpcm_page_struct*      padpcm[2][MAX_CHANNEL]; 
extern uint8_t                 adpcm_ready;
extern adpcm_page_ctrl_struct  adpcm_ctrl[MAX_CHANNEL];
extern tab_struct              tab;

static int write_page(uint8_t *page_bufer,unsigned long int page);
static int CheckBlockGetPageAdr(int mode);
static void WriteSuperBlockHeader(void);
static void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old);
static void WriteNandPage(void *padpcm_msg);

static uint8_t buffer[2048];  

void nand_erase_super_block(uint32_t id, uint32_t page)
{ 
   
  alarm_data.PageRealErase = page & 0xffffffc0;
  
  for(int i = 0; i < (MAX_PAGE + 64); i+= 64)
  {
    nand_erase_block(id,(unsigned long)alarm_data.PageRealErase);	
    alarm_data.PageRealErase+= 64;
  }
}


void nand_erase_handler(void)
{
  uint32_t index = alarm_data.index;
  
  if(alarm_data.change_index_state == STATE_ERASE_NFLASH) 
  {
    if(alarm_data.EraseCounter-- > 0)
    {
      nand_erase_block(index ^ 0x1,(unsigned long)alarm_data.PageRealErase);	
      alarm_data.PageRealErase += 64;
    }
    else
    {
      alarm_data.change_index_state = STATE_ERASE_NFLASH_DONE;
    }
    return;
  }
  else if((alarm_data.PageRealErase - alarm_data.PageAddress) <= (MAX_PAGE + 64))
  {
      nand_erase_block(index,(unsigned long)alarm_data.PageRealErase);	
      alarm_data.PageRealErase += 64;
  }    
}


void nand_sb_write_handler(void)
{   
  if(adpcm_ready == 0) return;
   
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
      
  for(int i = 0; i < MAX_CHANNEL; i++)
  {
    if(adpcm_ctrl[i].done == 1)
    {
      int id = adpcm_ctrl[i].id ^ 0x1;
         
      WriteNandPage(padpcm[id][i]);
      
      adpcm_ctrl[i].done = 0;
    }
  }
  
  adpcm_ready = 0;
  
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
}


static void WriteNandPage(void *padpcm_msg)
{
  uint32_t index = alarm_data.index;

/* ___________________________ ÍÀ×ÀËÎ ÇÀÏÈÑÈ ÔÀÉËÀ _____________________________ */
  
  if(alarm_data.PageRealWrite == 0)
  {
    if(CheckBlockGetPageAdr(HEADER_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÇÀÃÎËÎÂÎÊ
    {
      pwsb->sb_num = alarm_data.super_block_real_write;
      pwsb->id = SUPER_BLOCK_ID;
      pwsb->status = SUPER_BLOCK_OPEN;
      pwsb->time_open = GetTime();
      pwsb->super_block_this = alarm_data.PageAddress;      
      
      if(tab[index].sb_index++ == 0)
      {
        tab[0].unit_index = alarm_data.index;
        tab[1].unit_index = alarm_data.index;
        tab[index].time[0]  = pwsb->time_open;
        tab[index].index[0] = pwsb->sb_num;
      }
      
      if(alarm_data.super_block_time[0] == 0) alarm_data.super_block_time[0] = pwsb->time_open;
      
      pwsb->super_block_prev = alarm_data.super_block_current;
      alarm_data.super_block_current = alarm_data.PageAddress;
      alarm_data.PageAddress += PAGE_IN_BLOCK; 
     
      if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÍÀ×ÀËÜÍÀß ÑÒÐÀÍÈÖÀ ÄÀÍÍÛÕ)
      {
        if(write_page(padpcm_msg,alarm_data.PageAddress) == 1)
        {
          alarm_data.PageRealWrite++;
          alarm_data.PageAddress++;
        }
	else rewrite_block(REWRITE_BLOCK,padpcm_msg);
      }
    }
  }
 
/* ___________________________ ÎÊÎÍ×ÀÍÈÅ ÇÀÏÈÑÈ ÔÀÉËÀ __________________________ */
/* ________________________ ÏÐÈÍÓÄÈÒÅËÜÍÎÅ ÇÀÊÐÛÒÈÅ ÔÀÉËÀ ______________________ */
  
  
  else if(alarm_data.PageRealWrite == (MAX_PAGE-1) || alarm_data.close_file_flag == 1) // çàêðûòèå ôàéëà
  {
    if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÊÎÍÅ×ÍÀß ÑÒÐÀÍÈÖÀ ÑÓÏÅÐ-ÁËÎÊÀ)
    {
      if(write_page(padpcm_msg,alarm_data.PageAddress) == 1)
      {        
        pwsb->page_real_write = alarm_data.PageRealWrite;
        pwsb->sb_num = alarm_data.super_block_real_write;
        alarm_data.PageRealWrite = 0; 
        alarm_data.PageAddress++;
      }
      else 
      {
        rewrite_block(REWRITE_BLOCK,padpcm_msg);
        alarm_data.PageRealWrite = 0; 
      }
    
      if(CheckBlockGetPageAdr(HEADER_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÇÀÃÎËÎÂÎÊ ÑËÅÄÓÞÙÅÃÎ ÑÓÏÅÐ ÁËÎÊÀ
      {
        pwsb->super_block_prev = alarm_data.super_block_prev;
	pwsb->super_block_next = alarm_data.PageAddress;    
        pwsb->status = SUPER_BLOCK_RECORDED;
        pwsb->time_close = GetTime();
        
        WriteSuperBlockHeader();
               
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
        	
        alarm_data.super_block_prev = alarm_data.super_block_current;
        alarm_data.super_block_time[1] = pwsb->time_close;   // ÂÐÅÌß ÏÎÑËÅÄÍÅÃÎ ÇÀÏÈÑÀÍÍÎÃÎ ÑÓÏÅÐ ÁËÎÊÀ 
        alarm_data.super_block_real_write++;                 // ÍÎÌÅÐ ÏÎÑËÅÄÍÅÃÎ ÇÀÏÈÑÀÍÍÎÃÎ ÑÓÏÅÐ ÁËÎÊÀ
        tab[index].sbrw++;
      
      }
      else
      {
        // ÎØÈÁÊÀ ÏÐÎÂÅÐÊÈ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÇÀÃÎËÎÂÎÊ  ( ERROR_1) 
      }

    }
    else
    {
      // ÎØÈÁÊÀ ÏÐÎÂÅÐÊÈ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ ( ERROR_2 )
    }
     
    if(alarm_data.close_file_flag == 1)
    {
/* _____________________ ÏÅÐÅÕÎÄ ÍÀ ÄÐÓÃÎÉ ÊÎÌÏËÅÊÒ ÔËÅØÀ ______________________ */

      if(alarm_data.change_index_state == STATE_CHANGE_NFLASH)
      {
        alarm_data.index ^= 1;
        alarm_data.change_index_state = STATE_WAIT_END_NFLASH;
        alarm_data.close_file_flag = 0;
        alarm_data.PageAddress = 0;
	alarm_data.super_block_real_write = 0;

      }
      else
      {
        alarm_data.fixed_index[1] = alarm_data.fixed_index[0];
        alarm_data.close_file_flag = 0;
        memcpy(&alarm_data.bms[alarm_data.bookmark_index],&tab,sizeof(tab));
        memcpy(&tx_udp_msg[SERV].data[0],&tab,sizeof(tab));
        memset(&tab,0,sizeof(tab));
        alarm_data.bookmark_index = (alarm_data.bookmark_index + 1) & 0xf;
      }
    //  SendUdpMessage(GET_SYS_INFO_3 + 100,sizeof(tab)); 
    }
  }
 
 /* ___________________________ ÒÅÊÓÙÀß ÇÀÏÈÑÜ ÔÀÉËÀ ___________________________ */
  
  else
  {
    if(CheckBlockGetPageAdr(DATA_MODE))  // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÒÅÊÓÙÀß ÑÒÐÀÍÈÖÀ)
    {
      if(write_page(padpcm_msg,alarm_data.PageAddress) == 1)
      {
        alarm_data.PageRealWrite++; 
        alarm_data.PageAddress++;
      }
      else rewrite_block(REWRITE_BLOCK,padpcm_msg);
      
 /* ___________________ ÏÐÎÂÅÐÊÀ ÊÎÍÖÀ ÏÀÌßÒÈ  __________________________ */
      
     if(alarm_data.change_index_state == STATE_WAIT_END_NFLASH)
     {
       if((alarm_data.PageAddress) >= MAX_PAGE_IN_NAND)
       {
         alarm_data.EraseCounter = MAX_PAGE;
         alarm_data.change_index_state = STATE_ERASE_NFLASH;
         alarm_data.PageRealErase = 0;
       }
     }
     else if(alarm_data.change_index_state == STATE_ERASE_NFLASH_DONE)
     {
        alarm_data.close_file_flag    = 1;
        alarm_data.change_index_state = STATE_CHANGE_NFLASH;
     }
      
    }
  }
}



static int CheckBlockGetPageAdr(int mode)
{
  uint32_t block_adr = alarm_data.PageAddress >> 6;
  uint32_t page_index;

  if(pmap_bb->block_address[block_adr] == BLOCK_BAD)
  {
    block_adr++;
    block_adr &= 0xfff;
    alarm_data.PageAddress = block_adr << 6;
    return CheckBlockGetPageAdr(mode);
  }
  else if(pmap_bb->block_address[block_adr] == BLOCK_GOOD)
  {
    if(mode == HEADER_MODE) 
    {
      if((alarm_data.PageAddress & 0x3f) > 0) 
      {
        alarm_data.PageAddress = block_adr+1 << 6;
      }
    }
    else if(mode == DATA_MODE)
    {
      page_index = alarm_data.PageRealWrite;
      pwsb->ips[page_index].page_address = alarm_data.PageAddress;
      pwsb->ips[page_index].page_index = ADPCM_PAGE_ID;
    }

    return 1;
  }

  return 0;
}
  
static void WriteSuperBlockHeader(void)
{ 
  int i,j;
  uint32_t index = alarm_data.index;
  
  uint16_t *pin  = (uint16_t*)buffer;
  uint16_t *pout = (uint16_t*)pwsb;

  for(i = 0; i < 64; i++) 
  {
    for(j = 0; j < 1024; j++) *pin++ = *pout++;

    pin = (uint16_t*)buffer;  
    
    nand_8bit_write_page(index,buffer,alarm_data.super_block_current+i);  
  }
  
   pwsb->sb_num++;
}



static int write_page(uint8_t *page_buffer,unsigned long int page)
{
  uint32_t index = alarm_data.index;
    
  adpcm_page_struct* padpcm = (adpcm_page_struct*)page_buffer;
     
  if((crc32(page_buffer,ADPCM_BLOCK_SIZE-4,ADPCM_BLOCK_SIZE-4)) != padpcm->crc) alarm_data.err_crc_wr_Nand++;
  
  nand_16bit_write_page(index,(uint16_t*)page_buffer,page);
  
  tab[index].index[1] = pwsb->sb_num;
  tab[index].time[1]  = padpcm->time;
   
  return 1;
}


static void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old)
{
  static uint32_t start_adr;
  static uint32_t stop_adr;
  static uint32_t byte_real_write;
  static uint8_t  page_buffer[2048];
  static int rewrite_num;
  
  if(mode_rewrite == REWRITE_BLOCK)
  {
    start_adr = alarm_data.PageAddress & 0xFFFFFFC0;
    stop_adr  = alarm_data.PageAddress;
    byte_real_write = stop_adr - start_adr;    	
    pmap_bb->block_address[start_adr >> 6] = BLOCK_BAD;
    rewrite_num = 1;
  }
  else if(mode_rewrite == REWRITE_BLOCK_NEXT)
       {
         pmap_bb->block_address[alarm_data.PageAddress >> 6] = BLOCK_BAD;
	 rewrite_num++;
       }
	
  alarm_data.PageAddress = ((alarm_data.PageAddress >> 6) + 1) << 6;

  if(CheckBlockGetPageAdr(DATA_MODE)) // ÏÐÎÂÅÐÊÀ ÖÅËÎÑÒÍÎÑÒÈ ÁËÎÊÀ ÏÎÄ ÄÀÍÍÛÅ (ÏÅÐÅÇÀÏÈÑÛÂÀÅÌÛ ÁËÎÊ)
  {
    for(uint32_t i = 0; i < byte_real_write; i++)
    {
      // nand_read_page(page_buffer,start_adr+i,fp[1]);
      // PageRealRead ++;

      if(write_page(page_buffer,alarm_data.PageAddress) == 1)
      {
        alarm_data.PageAddress++;
      }
      else 
      {
        rewrite_block(REWRITE_BLOCK_NEXT,page_buffer_old);
        return;
      }
    }

    if(write_page(page_buffer_old,alarm_data.PageAddress) == 1)
    {
      alarm_data.PageRealWrite++; 
      alarm_data.PageAddress++;
    }
  }
}






