#include "main.h"

#include "nand_hw_driver.h"


void NAND_Config(void)
{
  nand_rst(0);
  nand_rst(1);
  
  nand_ecc_enable(0);
  nand_ecc_enable(1);
  
  nand_erase_super_block(0,0);
  nand_erase_super_block(1,0);

}

void nand_ecc_enable(uint32_t id)
{
  nand_command_wr(id,0xEFEF);
  nand_adr_wr(id,0x9090);
  nand_data_wr(id,0x0808);
  nand_data_wr(id,0x0000);
  nand_data_wr(id,0x0000);
  nand_data_wr(id,0x0000);
  nand_rdy(id);
}

uint16_t nand_read_status(uint32_t id)
{
  uint16_t data;
  
  nand_rdy(id);
  
  nand_command_wr(id,0x7070);
  data=nand_data_rd(id);
  return (data);
}

void nand_command_wr(uint32_t id, uint16_t data)
{
  if(id) *(uint16_t *) (sram_bank1+0x00020000) = data;
  else   *(uint16_t *) (sram_bank2+0x00020000) = data;  
}

void nand_adr_wr(uint32_t id, uint16_t data)
{
  if(id)*(uint16_t *) (sram_bank1+0x00040000) = data;
  else  *(uint16_t *) (sram_bank2+0x00040000) = data;
}

void nand_page_adr_wr(uint32_t id, unsigned long int page)
{
  nand_adr_wr(id,(page & 0xFF) + ((page & 0xFF) << 8));
  nand_adr_wr(id,((page >> 8) & 0xFF)+(page & 0xFF00));
  nand_adr_wr(id,((page >> 16) & 0xFF)+((page >> 8) & 0xFF00));
}

void nand_data_wr(uint32_t id, uint16_t data)
{
  if(id) *(uint16_t *) (sram_bank1+0x00000000) = data;
  else   *(uint16_t *) (sram_bank2+0x00000000) = data;

}

uint16_t nand_data_rd(uint32_t id)
{
  uint16_t data;
  if(id) data = *(__IO uint16_t*) (sram_bank1 + 0x00000000);
  else   data = *(__IO uint16_t*) (sram_bank2 + 0x00000000);
  return (data);
}

void nand_rst(uint32_t id)
{
  nand_command_wr(id,0xFFFF);
  nand_rdy(id);
}

void nand_rdy(uint32_t id)
{
  unsigned long int rdy,i;
	
  for (i=0;i<16;i++)
  {
  }
  do
  {
    if(id) rdy = GPIOG->IDR & 0x00000040;
    else   rdy = GPIOG->IDR & 0x00000080;
 
  } while(rdy == 0);
}

void nand_8bit_write_page(uint32_t id, uint8_t *page_bufer,unsigned long int page)
{
  uint16_t i;
  uint8_t crc_buf[64];
  
  nand_8bit_read_page_info(id,crc_buf,page);
  
  nand_rdy(id);
	
  nand_command_wr(id,0x80);					 
  nand_adr_wr(id,0x00);
  nand_adr_wr(id,0x00);
  nand_page_adr_wr(id,page);
	
  for (i=0;i<16;i++);
	
  for (i=0;i<2048;i++)
  {
    nand_data_wr(id,page_bufer[i]);
  }	
  
  for (i=0;i<64;i++)
  {
    nand_data_wr(id,crc_buf[i]);
  }	
  
  nand_command_wr(id,0x10);
  
  nand_rdy(id);
}

void nand_16bit_write_page(uint32_t id, uint16_t *page_bufer,unsigned long int page)
{
  uint16_t i;
    
  nand_command_wr(id,0x8080);					 
  nand_adr_wr(id,0x0000);
  nand_adr_wr(id,0x0000);
  nand_page_adr_wr(id,page);
	
  for (i=0;i<16;i++);
  
  for (i=0;i<2048;i++) nand_data_wr(id,page_bufer[i]);

  nand_command_wr(id,0x1010);
  
  nand_rdy(id);
}

void nand_8bit_read_page_info(uint32_t id, uint8_t *page_bufer,unsigned long int page)
{  
  uint16_t i;
  
  nand_command_wr(id,0x00);					
  nand_adr_wr(id,0x00);
  nand_adr_wr(id,0x08);
  nand_page_adr_wr(id,page);
  nand_command_wr(id,0x30);
  
  for (i=0;i<16;i++);
	
  nand_rdy(id);

  for (i=0;i<64;i++) page_bufer[i]=nand_data_rd(id);
}

void nand_16bit_read_page_info(uint32_t id, uint16_t *page_bufer,unsigned long int page)
{  
  uint16_t i;

  nand_command_wr(id,0x0000);					
  nand_adr_wr(id,0x0000);
  nand_adr_wr(id,0x0808);
  nand_page_adr_wr(id,page);
  nand_command_wr(id,0x3030);
  
  for (i=0;i<16;i++);
	
  nand_rdy(id);

  for (i=0;i<64;i++) page_bufer[i]=nand_data_rd(id);
}

void nand_8bit_read_page(uint32_t id, uint8_t *page_bufer,unsigned long int page)
{
  uint16_t i;
  
  nand_command_wr(id,0x00);					
  nand_adr_wr(id,0x00);
  nand_adr_wr(id,0x00);
  nand_page_adr_wr(id,page);
  nand_command_wr(id,0x30);
  
  for (i=0;i<16;i++);
	
  nand_rdy(id);

  for (i=0;i<2048;i++) page_bufer[i]=nand_data_rd(id);
}

void nand_16bit_read_page(uint32_t id, uint16_t *page_bufer,unsigned long int page)
{
  uint16_t i;
  
  nand_rdy(id);
   
  nand_command_wr(id,0x0000);					
  nand_adr_wr(id,0x0000);
  nand_adr_wr(id,0x0000);
  nand_page_adr_wr(id,page);
  nand_command_wr(id,0x3030);
	
  for (i=0;i<16;i++);
	
  nand_rdy(id);
  
  for (i=0;i<2048;i++) page_bufer[i]=nand_data_rd(id);
  
}

void nand_erase_block(uint32_t id, unsigned long int page)
{
    
  uint16_t i;
    
  nand_command_wr(id,0x6060);					 
  nand_page_adr_wr(id,page);
  nand_command_wr(id,0xD0D0);
          
  for (i=0;i<16;i++);
    
  nand_rdy(id);
}