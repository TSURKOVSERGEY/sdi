#include "main.h"
#include "ethernet.h"
#include "nand_hw_driver.h"

extern alarm_struct            alarm_data;
extern super_block_struct*     pwsb;
extern bad_block_map_struct*   pmap_bb;
extern adpcm_page_struct*      padpcm[2][MAX_CHANNEL]; 
extern uint8_t                 adpcm_ready;
extern adpcm_page_ctrl_struct  adpcm_ctrl[MAX_CHANNEL];
extern tab_struct              tab;
extern initial_info_struct     info_ini;

static int  write_page(uint8_t *page_bufer,unsigned long int page);
static int  CheckBlockGetPageAdr(int mode);
static int  CheckTheEndOfTheNandMemory(void);
static int  CheckChangeNandMemory(void);

static void UpdateBeadBlockMap(void);
static void* f_open(uint32_t f_name);
static void* f_close(void);
static uint32_t f_write(void *padpcm_msg);
static void update_tab_info(int mode);

static uint8_t buffer[2048];  

enum
{
  MODE_OPEN,
  MODE_WRITE,
  MODE_CLOSE,
  MODE_CHANGE_FLASH
};

// *****************************************************************************
//                     ОБРАБОТЧИК ВЕТВИ ЗАПИСИ ДАННЫХ
// *****************************************************************************
void nand_sb_write_handler(void)
{
  static void* pf = NULL;
  
  if(adpcm_ready == 0) return;                  // Ожидаем все 16 каналов
  else if(CheckChangeNandMemory() == 1) return; // Ожидаем стирание блока
   
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
      
  for(int i = 0; i < MAX_CHANNEL; i++)     
  {
    if(adpcm_ctrl[i].done == 1)
    {
      if(pf == NULL)
      {
        pf = f_open(alarm_data.super_block_real_write);
      }
      
      if((f_write(padpcm[adpcm_ctrl[i].id ^ 0x1][i]) == MAX_DATA_PAGE) || CheckTheEndOfTheNandMemory())
      {
        pf = f_close();
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
      }
         
      adpcm_ctrl[i].done = 0;
    }
  }
  
  adpcm_ready = 0;
  
  GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
}

////////////////////////////////////////////////////////////////////////////////
// f_open
////////////////////////////////////////////////////////////////////////////////
static void* f_open(uint32_t f_name)
{
    if(CheckBlockGetPageAdr(HEADER_MODE))  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ЗАГОЛОВОК ЗАПИСЫВАЕМОГО ФАЙЛА
    {    
      pwsb->id = SUPER_BLOCK_ID;                                // идентификатор файла
      pwsb->status = SUPER_BLOCK_OPEN;                          // текущий режим файла
      pwsb->time_open  = GetTime();                             // время открытия файла
      pwsb->time_close = 0;                                     // время закрытия файла
      pwsb->sb_num = f_name;                                    // номер (имя) записываемого файла
      pwsb->page_real_write = 0;                                // количество страниц в файле 
      pwsb->super_block_prev = alarm_data.super_block_current;  // адрес предидущего файла (связанный список)
      pwsb->super_block_next = 0;                               // адрес следующего  файла (связанный список)
      
      alarm_data.super_block_current = alarm_data.PageAddress;  // адрес текущего файла
      alarm_data.PageAddress += PAGE_IN_BLOCK;                  // устанавливаем адрес записи в поле данных (за границей заголовка файла)
    }
    else
    {
      info_ini.write_driver_error = 0x1;                        // КРИТИЧЕСКАЯ ОШИБКА !!!!
      return NULL;
    }
     
    update_tab_info(MODE_OPEN);
    return pwsb;
}

////////////////////////////////////////////////////////////////////////////////
// f_close
////////////////////////////////////////////////////////////////////////////////
static void* f_close(void)
{   
  int i,j;
  uint32_t index = alarm_data.index;
  uint16_t *pin  = (uint16_t*)buffer;
  uint16_t *pout = (uint16_t*)pwsb;
    
  if(CheckBlockGetPageAdr(HEADER_MODE))  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ЗАГОЛОВОК СЛЕДУЩЕГО ФАЙЛА
  {
    pwsb->status = SUPER_BLOCK_RECORDED;                  // текущий режим файла (ФАЙЛ ЗАПИСАН)
    pwsb->time_close = GetTime();                         // время закрытия файла
    pwsb->page_real_write = alarm_data.PageRealWrite-1;   // количество записанных страниц в файле
    pwsb->super_block_next = alarm_data.PageAddress;      // указатнль на следующий файл
   
    for(i = 0; i < 64; i++) // Запись заголовка файла (64 страницы)
    {
      // флеш нормально записывает только через внутренею память, 
      // поэтому копируем из внешней ОЗУ во внутренею
      
      for(j = 0; j < 1024; j++) *pin++ = *pout++;         

      pin = (uint16_t*)buffer;  
    
      nand_8bit_write_page(index,buffer,alarm_data.super_block_current+i);  
    }
  
    alarm_data.super_block_prev = alarm_data.super_block_current; // сохранение указателя текущего файла
    alarm_data.super_block_time[1] = pwsb->time_close;            // время закрытия файла
    alarm_data.super_block_real_write++;                          // номер последнего записанного файла
    alarm_data.PageRealWrite = 0;                                 // количество страниц в файле 

  }
  else
  {
    info_ini.write_driver_error = 0x1;  // КРИТИЧЕСКАЯ ОШИБКА !!!!
  }
  
  update_tab_info(MODE_CLOSE);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// f_write
////////////////////////////////////////////////////////////////////////////////
static uint32_t f_write(void *padpcm_msg)
{
  if(CheckBlockGetPageAdr(DATA_MODE))  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ДАННЫЕ
  {
    if(write_page(padpcm_msg,alarm_data.PageAddress) == 0)
    {
      UpdateBeadBlockMap(); // Установка флага " битого " сектора 
    }
    alarm_data.PageRealWrite++; 
    alarm_data.PageAddress++;
  }
  else
  {
    info_ini.write_driver_error = 0x1;  // КРИТИЧЕСКАЯ ОШИБКА !!!!
  }
  
  update_tab_info(MODE_WRITE);
  return alarm_data.PageRealWrite;
}

////////////////////////////////////////////////////////////////////////////////
// update_tab_info
////////////////////////////////////////////////////////////////////////////////
static void update_tab_info(int mode)
{
  static int first_start = 1;
  
  uint32_t index = alarm_data.index;
  
  if(first_start == 1)
  {
    tab[index].time[0]  = GetTime();   // время окончания записи флеш
    first_start = 0;
  }
  
  switch(mode)
  {
    case MODE_OPEN:
      
      tab[0].unit_index   = alarm_data.index;  // индекс активного комплекта флеш
      tab[1].unit_index   = alarm_data.index;  // индекс активного комплекта флеш
      tab[index].time[1] = GetTime();  
     
    break;
    
    case MODE_WRITE:
      
     tab[index].time[1] = GetTime();  
      
    break;
    
    case MODE_CLOSE:
          
      tab[index].time[1]  = pwsb->time_close;
      tab[index].sbrw = alarm_data.super_block_real_write;  
      
    break;
    
    case MODE_CHANGE_FLASH:
      
      tab[0].unit_index   = alarm_data.index;  // индекс активного комплекта флеш
      tab[1].unit_index   = alarm_data.index;  // индекс активного комплекта флеш
      tab[index].time[0]  = GetTime();         // время начала записи флеш
      tab[index].time[1]  = GetTime();         // время окончания записи флеш
      tab[index].sbrw = 0;                     // индекс записанного файла
  
    break;
    
  }
}

////////////////////////////////////////////////////////////////////////////////
// CheckBlockGetPageAdr
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
// write_page
////////////////////////////////////////////////////////////////////////////////
static int write_page(uint8_t *page_buffer,unsigned long int page)
{    
  uint32_t index = alarm_data.index;
    
  adpcm_page_struct* padpcm = (adpcm_page_struct*)page_buffer;
     
  if((crc32(page_buffer,ADPCM_BLOCK_SIZE-4,ADPCM_BLOCK_SIZE-4)) != padpcm->crc) alarm_data.err_crc_wr_Nand++;
  
  nand_16bit_write_page(index,(uint16_t*)page_buffer,page);
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// UpdateBeadBlockMap
////////////////////////////////////////////////////////////////////////////////
static void UpdateBeadBlockMap(void)
{
}

////////////////////////////////////////////////////////////////////////////////
// CheckTheEndOfTheNandMemory
////////////////////////////////////////////////////////////////////////////////
int CheckTheEndOfTheNandMemory(void)
{
  if(alarm_data.state == STATE_WAIT_END_NFLASH)
  {
    if((alarm_data.PageAddress) >= MAX_PAGE_IN_NAND)
    {
      alarm_data.ErasePageCounter = MAX_ERASE_PAGE;
      alarm_data.PageRealErase = 0;
      alarm_data.state = STATE_ERASE_NFLASH;
      return 1;
    }
  }
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
// CheckChangeNandMemory
////////////////////////////////////////////////////////////////////////////////
static int CheckChangeNandMemory(void)
{
  if(alarm_data.state == STATE_ERASE_NFLASH)
  {
    return 1;
  }
  else if(alarm_data.state == STATE_ERASE_NFLASH_DONE)
  {
    alarm_data.index ^= 1;

    alarm_data.super_block_real_write = 0;  
    alarm_data.super_block_time[0] = GetTime();
    alarm_data.super_block_time[1] = GetTime();
    alarm_data.PageRealWrite = 0;
    alarm_data.PageRealErase = 0;
    alarm_data.PageAddress = 0;
    alarm_data.ErasePageCounter = 0;
    alarm_data.super_block_begin = 0;
    alarm_data.super_block_prev = 0;
    alarm_data.super_block_current = 0;
    update_tab_info(MODE_CHANGE_FLASH);
    alarm_data.state = STATE_WAIT_END_NFLASH;
  }
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Установка режима < STATE_ERASE_NFLASH > производится при переходе на другой 
// комплект флеш. Режим служит для очистки буффера под новые данные.
// Переход на другой (активный) комплект флеш выполняется после установки флага 
// < STATE_ERASE_NFLASH_DONE >
// Объем стираемого буффера равен количеству страниц в одном файле определяется
// параметром MAX_PAGE
// Если режим < STATE_ERASE_NFLASH > не установлен, и колличество чистых страниц
// менее (MAX_PAGE + 64), происходит стирание в активном блоке флеш.
// Это условие отслеживается по разнице адресов относительно адреса текущей стертой 
// страницы и текущего адреса записываемой страницы 
////////////////////////////////////////////////////////////////////////////////
void nand_erase_handler(void)
{
  uint32_t index = alarm_data.index;
  
  if(alarm_data.state == STATE_ERASE_NFLASH) 
  {
    if(alarm_data.ErasePageCounter-- > 0) // стирание начального не активного блока 
    {
      nand_erase_block(index ^ 0x1,(unsigned long)alarm_data.PageRealErase);	
      alarm_data.PageRealErase += 64; // адрес стираемой страницы
    }
    else
    { // подготовка буффера завершена, можно переходить на другой комплект
      alarm_data.state = STATE_ERASE_NFLASH_DONE;
    }
    return;
  }
  // стирание флеш в активном блоке 
  else if((alarm_data.PageRealErase - alarm_data.PageAddress) <= PAGE_IN_SBLOCK)
  {
    nand_erase_block(index,(unsigned long)alarm_data.PageRealErase);	
    alarm_data.PageRealErase += 64; // адрес стираемой страницы
  }    
}

////////////////////////////////////////////////////////////////////////////////
// nand_erase_super_block
////////////////////////////////////////////////////////////////////////////////
void nand_erase_super_block(uint32_t id, uint32_t page)
{ 
  alarm_data.PageRealErase = page & 0xffffffc0;
  
  for(int i = 0; i < PAGE_IN_SBLOCK; i+= 64)
  {
    nand_erase_block(id,(unsigned long)alarm_data.PageRealErase);	
    alarm_data.PageRealErase+= 64;
  }
}
