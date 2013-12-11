#include <stdafx.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include "nand_sb_driver.h"


#define _error_bb_map
#define _error_bp_map


int sb_number = 0;
int PageRealRead = 0;

super_block_struct   *psuper_block;
bad_block_map_struct *pmap_bb;
alarm_struct          alarm_data;
super_block_struct    sram_bank3;

FILE *fp[2];


////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
uint32_t SuperBlock_Config(void)
{
	fp[0] = fopen("super_block.txt","w");
	fp[1] = fopen("temp.ini","r+");

    pmap_bb      = new bad_block_map_struct;
	psuper_block = new super_block_struct;

	memset(psuper_block,0,sizeof(super_block_struct));
	memset(pmap_bb,0,sizeof(bad_block_map_struct));
	memset(&alarm_data,0,sizeof(alarm_data));

	fprintf(fp[0],"\n Œ“À¿ƒ ◊»  ‘¿…ÀŒ¬Œ… —»—“≈Ã€ ver - 0 \n");
    fprintf(fp[0],"\n ");


	#ifdef _error_bb_map 	

	AT45_Read(ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));

	fprintf(fp[0],"\n “¿¡À»÷¿  ¿–“€ Õ≈»—œ–¿¬Õ€’ ¡ÀŒ Œ¬ \n \n");

	for(int j = 0; j < MAX_BLOCK; j++)
	{
		if(pmap_bb->block_address[j] == BLOCK_BAD) fprintf(fp[0]," Œÿ»¡ ¿ ¬ ¡ÀŒ E %003d, ¿ƒ–≈—¿ —“–¿Õ»÷ «¿œ–≈Ÿ≈ÕÕ€’   «¿œ»—» [%d-%d] \n",j,(j * 64),(j * 64)+63);

 	}
    #endif

	fprintf(fp[0],"\n");

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
int CheckBlockGetPageAdr(int mode)
{
  uint32_t block_adr = alarm_data.PageAddress >> 6;
   

  if(pmap_bb->block_address[block_adr] == BLOCK_BAD)
  {
    block_adr++;
    //block_adr &= 0xfff; !!??
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
    

    psuper_block->ips[alarm_data.PageRealWrite].page_address = alarm_data.PageAddress;
    psuper_block->ips[alarm_data.PageRealWrite].page_index = ADPCM_PAGE_ID;
    return 1;
  }
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
void WriteNandPage(adpcm_message_struct *padpcm_msg)
{
  if(alarm_data.PageRealWrite == 0)
  {
    if(CheckBlockGetPageAdr(HEADER_MODE)) // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ «¿√ŒÀŒ¬Œ 
    {
      psuper_block = (super_block_struct*)&sram_bank3;
      psuper_block->id = SUPER_BLOCK_ID;
      psuper_block->status = SUPER_BLOCK_OPEN;
      psuper_block->time_open = GetTime();
      
      psuper_block->super_block_prev = alarm_data.super_block_current;
      alarm_data.super_block_current = alarm_data.PageAddress;
      alarm_data.PageAddress += 64; 
     
      if(CheckBlockGetPageAdr(DATA_MODE)) // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ ƒ¿ÕÕ€≈ (Õ”À≈¬¿ﬂ —“–¿Õ»÷¿)
      {
        if(nand_write_page((uint16_t*)&padpcm_msg,alarm_data.PageAddress,fp[1]) == 1)
		{
			alarm_data.PageRealWrite++;
            alarm_data.PageAddress++;
		}
		else rewrite_block(REWRITE_BLOCK,(uint16_t*)&padpcm_msg);
      }
    }
  }
  else if(alarm_data.PageRealWrite == 2047)
  {
    if(CheckBlockGetPageAdr(DATA_MODE)) // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ ƒ¿ÕÕ€≈ (œŒ—À≈ƒÕﬂﬂ —“–¿Õ»÷¿)
    {
      psuper_block->status = SUPER_BLOCK_RECORDED;
      psuper_block->time_close = GetTime();

      if(nand_write_page((uint16_t*)&padpcm_msg,alarm_data.PageAddress,fp[1]) == 1)
	  {
		  alarm_data.PageRealWrite = 0; 
          alarm_data.PageAddress++;
	  }
	  else 
	  {
		  rewrite_block(REWRITE_BLOCK,(uint16_t*)&padpcm_msg);
          alarm_data.PageRealWrite = 0; 
	  }
    
      if(CheckBlockGetPageAdr(HEADER_MODE)) // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ «¿√ŒÀŒ¬Œ  —À≈ƒ”ﬁŸ≈√Œ —”œ≈– ¡ÀŒ ¿
	  {
		  psuper_block->super_block_prev = alarm_data.super_block_prev;
	      psuper_block->super_block_next = alarm_data.PageAddress;
		  alarm_data.super_block_prev    = alarm_data.super_block_current;
	  }
  
      WriteSuperBlockHeader();
      alarm_data.TargetIndex ^= 0x1; 
	  sb_number ++;

    }
  }
  else
  {
    if(CheckBlockGetPageAdr(DATA_MODE))  // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ ƒ¿ÕÕ€≈ (“≈ ”Ÿ¿ﬂ —“–¿Õ»÷¿)
    {
      if(nand_write_page((uint16_t*)&padpcm_msg,alarm_data.PageAddress,fp[1]) == 1)
	  {
		  alarm_data.PageRealWrite++; 
          alarm_data.PageAddress++;
	  }
	  else rewrite_block(REWRITE_BLOCK,(uint16_t*)&padpcm_msg);
    
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
int nand_read_page(uint16_t *page_bufer,unsigned long int page,FILE *fp)
{
	fprintf(fp," READ  super_block = %d     block = %d     page = %d     page_real_read = %d \n",sb_number,page >> 6,page,PageRealRead);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
int nand_write_page(uint16_t *page_bufer,unsigned long int page,FILE *fp)
{
	
 #ifdef _error_bb_map 

	FILE *pfile;
	
	static int fp_flag = 1;

	unsigned int page_index;

	if((pfile = fopen("map_bp.ini","r")) != NULL)
	{
		while(fscanf(pfile,"%d",&page_index) > 0)
		{
			if(page_index == page) 
			{
				fclose(pfile);
				return 0;
			}
		}
 	
	}
	else 
	{
		if(fp_flag) printf(" error open file map_bp.ini");

	}

 #endif

	fprintf(fp," WRITE super_block = %d     block = %d     page = %d     page_real_write = %d \n",sb_number,page >> 6,page,alarm_data.PageRealWrite);

	fclose(pfile);
	return 1;
}


////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
void WriteSuperBlockHeader(void)
{
  int brw;
  unsigned char data_temp;

  fprintf(fp[0],"\n write super block header \n");

  fprintf(fp[0],"\n ********************************************************************************* \n");
  fprintf(fp[0],"\n * SB%d * SB%d * SB%d * SB%d * SB%d * SB%d * SB%d * SB%d * \n",sb_number,sb_number,sb_number,sb_number,sb_number,sb_number,sb_number,sb_number);
  fprintf(fp[0],"\n ********************************************************************************* \n");

  fprintf(fp[0]," psuper_block->crc = %d \n",psuper_block->crc);
  fprintf(fp[0]," psuper_block->id = 0x%x \n",psuper_block->id);
  fprintf(fp[0]," psuper_block->status = %d \n",psuper_block->status);
  fprintf(fp[0]," psuper_block->super_block_next = %d \n",psuper_block->super_block_next);
  fprintf(fp[0]," psuper_block->super_block_prev = %d \n",psuper_block->super_block_prev);
  fprintf(fp[0]," psuper_block->time_open = %d \n",psuper_block->time_open);
  fprintf(fp[0]," psuper_block->time_close = %d \n",psuper_block->time_close);
  
  fprintf(fp[0],"\n");
  fprintf(fp[0],"\n ********************************************************************************* \n");
  fprintf(fp[0],"\n * HEADER * HEADER * HEADER * HEADER * HEADER * HEADER * HEADER * HEADER * HEADER \n");
  fprintf(fp[0],"\n ********************************************************************************* \n");

  for(int i = 0; i < 64; i++) nand_write_page((uint16_t*)&psuper_block,alarm_data.super_block_current+i,fp[0]);
    


  brw = ftell(fp[1]);
  
  fseek(fp[1],0,SEEK_SET);

  fprintf(fp[0],"\n");
  fprintf(fp[0],"\n *********************************************************************************** \n");
  fprintf(fp[0],"\n * DATA * DATA * DATA * DATA * DATA * DATA * DATA * DATA * DATA * DATA * DATA * DATA \n");
  fprintf(fp[0],"\n *********************************************************************************** \n");


  while(brw-- > 0)
  {
	  if(fread(&data_temp,1,1,fp[1]))
	  {
		  fwrite(&data_temp,1,1,fp[0]);
	  }
  }

  fclose(fp[1]);
  fp[1] = fopen("temp.ini","w+");


}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
uint64_t GetTime(void)
{
	static int time = 0;
    return time++;
}


////////////////////////////////////////////////////////////////////////////////
// SIMULATOR AT45 READ
////////////////////////////////////////////////////////////////////////////////
uint32_t AT45_Read(uint16_t Addr, uint8_t *pdata, int size)
{
	FILE *fp;

	if((fp = fopen("map_bb.mas","r")) != NULL)
	{
		fread(pdata,1,size,fp);
		fclose(fp);
	
	}
	else 
	{
		printf("\n");
		printf("Error AT45_Read (map_bb) \n");
		printf("\n");
		return 0;
	}


	return 1;
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
void rewrite_block(int mode_rewrite,uint16_t *page_buffer_old)
{
	static uint32_t start_adr;
    static uint32_t stop_adr;
    static uint32_t byte_real_write;
	static uint16_t page_buffer[2048];
	static int rewrite_num;


 	if(mode_rewrite == REWRITE_BLOCK)
	{
		start_adr = alarm_data.PageAddress & 0xFFFFFFC0;
		stop_adr  = alarm_data.PageAddress;
   	    byte_real_write = stop_adr - start_adr;    	
		pmap_bb->block_address[start_adr >> 6] = BLOCK_BAD;
		rewrite_num = 1;
		PageRealRead = 0;

        fprintf(fp[1],"\n");  
        fprintf(fp[1]," ///////////////////////////////////////////////////////////////////////////// \n");
        fprintf(fp[1]," Œÿ»¡ ¿ «¿œ»—» —“–¿Õ»÷€ %d ¡ÀŒ ¿ %d \n",alarm_data.PageAddress,start_adr >> 6);
        fprintf(fp[1],"  ŒÀÀ»◊≈—“¬Œ —“–¿Õ»÷ “–≈¡”ﬁŸ»’ œ≈–≈«¿œ»—‹ %d + 1 (—“–¿Õ»÷¿ — Œÿ»¡ Œ…) \n",byte_real_write);
        fprintf(fp[1]," ¿ƒ–≈—— Õ¿◊¿À¿  Œœ»–Œ¬¿Õ»ﬂ %d  - ¿ƒ–≈——  ŒÕ÷¿  Œœ»–Œ¬¿Õ»ﬂ %d \n",start_adr,stop_adr-1);
        fprintf(fp[1]," ///////////////////////////////////////////////////////////////////////////// \n");
        fprintf(fp[1],"\n");       


	}
	else if(mode_rewrite == REWRITE_BLOCK_NEXT)
	{
		fprintf(fp[1],"\n"); 
        fprintf(fp[1]," ///////////////////////////////////////////////////////////////////////////// \n");
        fprintf(fp[1]," Œÿ»¡ ¿ «¿œ»—» —“–¿Õ»÷€ %d ¡ÀŒ ¿ %d œŒ¬“Œ–ÕŒ≈  Œœ»–Œ¬¿Õ»≈ %d \n",alarm_data.PageAddress,(alarm_data.PageAddress & 0xFFFFFFC0) >> 6,rewrite_num);
        fprintf(fp[1],"  Œœ»–Œ¬¿Õ»≈ ¬ ¡ÀŒ  %d \n",(alarm_data.PageAddress >> 6) + 1);
	    fprintf(fp[1],"\n"); 

		pmap_bb->block_address[alarm_data.PageAddress >> 6] = BLOCK_BAD;
		rewrite_num ++;
	
	}

	
	alarm_data.PageAddress = ((alarm_data.PageAddress >> 6) + 1) << 6;

	if(CheckBlockGetPageAdr(DATA_MODE)) // œ–Œ¬≈– ¿ ÷≈ÀŒ—“ÕŒ—“» ¡ÀŒ ¿ œŒƒ ƒ¿ÕÕ€≈ (œ≈–≈«¿œ»—€¬¿≈Ã€ ¡ÀŒ )
    {
		for(uint32_t i = 0; i < byte_real_write; i++)
		{
		   nand_read_page(page_buffer,start_adr+i,fp[1]);
		   PageRealRead ++;

		   if(nand_write_page(page_buffer,alarm_data.PageAddress,fp[1]) == 1)
		   {
			   alarm_data.PageAddress++;
		   }
		   else 
		   {
			    rewrite_block(REWRITE_BLOCK_NEXT,page_buffer_old);
				return;
		   }
		}


	    if(nand_write_page(page_buffer_old,alarm_data.PageAddress,fp[1]) == 1)
		{
			 alarm_data.PageRealWrite++; 
 		     alarm_data.PageAddress++;
		}
	}
  
    fprintf(fp[1],"\n");       
    fprintf(fp[1]," ///////////////////////////////////////////////////////////////////////////// \n");
    fprintf(fp[1],"\n");       

	Sleep(0);

}


void OnClose(void)
{
	#ifdef _error_bb_map 

	fprintf(fp[0],"\n “¿¡À»÷¿  ¿–“€ Õ≈»—œ–¿¬Õ€’ ¡ÀŒ Œ¬ \n \n");

	for(int j = 0; j < MAX_BLOCK; j++)
	{
		if(pmap_bb->block_address[j] == BLOCK_BAD) fprintf(fp[0]," Œÿ»¡ ¿ ¬ ¡ÀŒ E %003d, ¿ƒ–≈—¿ —“–¿Õ»÷ «¿œ–≈Ÿ≈ÕÕ€’   «¿œ»—» [%d-%d] \n",j,(j * 64),(j * 64)+63);

 	}

	// AT45_Write(ADR_MAP_BB,(uint8_t*)pmap_bb,sizeof(bad_block_map_struct));

    #endif

}