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
extern total_info_struct       t_info;
extern total_work_struct       tws;
extern uint64_t total_timer;
extern uint8_t  server_control[2][MAX_FILE];

static int  write_page(uint8_t *page_bufer,unsigned long int page);
static int  CheckBlockGetPageAdr(int mode);
static int  CheckTheEndOfTheNandMemory(void);
static int  CheckChangeNandMemory(void);

static void SaveTotalTimeTotalCycle(int id);
static void UpdateBeadBlockMap(void);
static void* f_open(uint32_t f_name);
static void* f_close(void);
static uint32_t f_write(void *padpcm_msg);
static void update_tab_info(int mode);
void SetServerControlBit(uint32_t sb_index);
static uint32_t CheckServer(void);


static uint8_t buffer[2048];  

enum
{
  MODE_OPEN,
  MODE_WRITE,
  MODE_CLOSE,
  MODE_CHANGE_FLASH
};

enum
{
  F_OPEN_WRITE,
  F_CLOSE
};

// *****************************************************************************
//                     ОБРАБОТЧИК ВЕТВИ ЗАПИСИ ДАННЫХ
// *****************************************************************************
void nand_sb_write_handler(void)
{
  static void* pf = NULL;
  uint32_t status;
  
  if(adpcm_ready == 0) return;                  // Ожидаем все 16 каналов
  else if(CheckChangeNandMemory() == 1) return; // Ожидаем стирание блока
   

      
  for(int i = 0; i < MAX_CHANNEL; i++)     
  {
    if(adpcm_ctrl[i].done == 1)
    {
      if(pf == NULL)
      {
        if((status == CheckTheEndOfTheNandMemory()) == F_OPEN_WRITE)
        {
           if((pf = f_open(alarm_data.super_block_real_write)) == NULL)
          {
            return;
          }
        }
        else return;
      }
        
      GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 
      
      if((f_write(padpcm[adpcm_ctrl[i].id ^ 0x1][i]) == MAX_DATA_PAGE) || (CheckTheEndOfTheNandMemory() == F_CLOSE))
      {
        SetServerControlBit(pwsb->sb_num);
        pf = f_close();
        GPIO_ToggleBits(GPIOI, GPIO_Pin_0); 
      }
        
      GPIO_ToggleBits(GPIOI, GPIO_Pin_1);    
      
      adpcm_ctrl[i].done = 0;
    }
  }
  
  adpcm_ready = 0;
  

}

////////////////////////////////////////////////////////////////////////////////
// f_open
////////////////////////////////////////////////////////////////////////////////
static void* f_open(uint32_t f_name)
{
  uint32_t status;
  
    if((status = CheckBlockGetPageAdr(HEADER_MODE)) == DONE)  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ЗАГОЛОВОК ЗАПИСЫВАЕМОГО ФАЙЛА
    {    
      pwsb->id = SUPER_BLOCK_ID;                                // идентификатор файла
      pwsb->status = SUPER_BLOCK_OPEN;                          // текущий режим файла
      pwsb->time_open  = GetTime();                             // время открытия файла
      pwsb->time_close = 0;                                     // время закрытия файла
      pwsb->sb_num = f_name;                                    // номер (имя) записываемого файла
      pwsb->page_real_write = 0;                                // количество страниц в файле 
      pwsb->super_block_prev = alarm_data.super_block_current;  // адрес предидущего файла (связанный список)
      pwsb->super_block_next = 0;                               // адрес следующего  файла (связанный список)
      
      alarm_data.super_block_current = alarm_data.PageAdressWrite;  // адрес текущего файла
      alarm_data.PageAdressWrite += PAGE_IN_BLOCK;                  // устанавливаем адрес записи в поле данных (за границей заголовка файла)
    }
    else if(status == FAILURE)
    {
      t_info.f_write_error = 0x1;                        // КРИТИЧЕСКАЯ ОШИБКА !!!!
      t_info.f_o = pwsb->sb_num;
      return NULL;
    }
    else if(status == ENDED_MEMORY)
    {
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
  uint32_t status;
  
  if(pwsb->sb_num == 3)
  {
    delay(0);
  }
  
  if((status = CheckBlockGetPageAdr(HEADER_MODE)) != FAILURE)  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ЗАГОЛОВОК СЛЕДУЩЕГО ФАЙЛА
  {
    pwsb->status = SUPER_BLOCK_RECORDED;                  // текущий режим файла (ФАЙЛ ЗАПИСАН)
    pwsb->time_close = GetTime();                         // время закрытия файла
    if(alarm_data.PageRealWrite == 0) 
    {
      pwsb->page_real_write = 0;
    }
    else 
    {
      pwsb->page_real_write = alarm_data.PageRealWrite-1;   // количество записанных страниц в файле
    }
    
    if(status == DONE)
    {
      pwsb->super_block_next = alarm_data.PageAdressWrite; // указатнль на следующий файл
    }
    else if(status == ENDED_MEMORY)
    {
      pwsb->super_block_next = 0;
    }
    
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
    t_info.f_write_error = 0x1;  // КРИТИЧЕСКАЯ ОШИБКА !!!!
    t_info.f_c = pwsb->sb_num;

  }
  
  update_tab_info(MODE_CLOSE);
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// f_write
////////////////////////////////////////////////////////////////////////////////
static uint32_t f_write(void *padpcm_msg)
{
  uint32_t status;
  
  if((status = CheckBlockGetPageAdr(DATA_MODE)) == DONE)  // ПРОВЕРКА ЦЕЛОСТНОСТИ БЛОКА ПОД ДАННЫЕ
  {
    if(write_page(padpcm_msg,alarm_data.PageAdressWrite) == 0)
    {
      UpdateBeadBlockMap(); // Установка флага " битого " сектора 
    }
    alarm_data.PageRealWrite++; 
    alarm_data.PageAdressWrite++;
  }
  else if(status == FAILURE)
  {
    t_info.f_write_error = 0x1;  // КРИТИЧЕСКАЯ ОШИБКА !!!!
    t_info.f_w = pwsb->sb_num;

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
  uint32_t block_adr = alarm_data.PageAdressWrite >> 6;
  uint32_t page_index;
      
  // проверка границы памяти
  if((alarm_data.PageAdressWrite) >= MAX_PAGE_IN_NAND) return ENDED_MEMORY;

  // если блок битый
  if(pmap_bb->block_address[block_adr] == BLOCK_BAD)
  {
    block_adr++;                                 // переходим на следующий блок
    block_adr &= 0xfff;                          // выравниваю по границе блока
    alarm_data.PageAdressWrite = block_adr << 6; // получаю действ. адресс
    return CheckBlockGetPageAdr(mode);           // новая проверка
  }
  else if(pmap_bb->block_address[block_adr] == BLOCK_GOOD)
  {

    if(mode == HEADER_MODE) // если пишем заголовок файла  
    {
      if((alarm_data.PageAdressWrite & 0x3f) > 0) // проверка на выравнивание 
      { // заголовок всегда должен писаться с нулевой страницы блока !!!!!!!!!                                           
        alarm_data.PageAdressWrite = block_adr+1 << 6; 
      }
    }
    else if(mode == DATA_MODE)
    {
      // сохраняю данные о текущем состоянии файла в его заголовке
      page_index = alarm_data.PageRealWrite;
      pwsb->ips[page_index].page_address = alarm_data.PageAdressWrite;
      pwsb->ips[page_index].page_index = ADPCM_PAGE_ID;
    }

    return DONE;
  }

  return FAILURE;
}

////////////////////////////////////////////////////////////////////////////////
// write_page
////////////////////////////////////////////////////////////////////////////////
static int write_page(uint8_t *page_buffer,unsigned long int page)
{    
  uint32_t index = alarm_data.index;
    
  adpcm_page_struct* padpcm = (adpcm_page_struct*)page_buffer;
     
  if((crc32(page_buffer,ADPCM_BLOCK_SIZE-4,ADPCM_BLOCK_SIZE-4)) != padpcm->crc) t_info.crc_wr_nand_error++;
  
  nand_16bit_write_page(index,(uint16_t*)page_buffer,page);
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// UpdateBeadBlockMap
////////////////////////////////////////////////////////////////////////////////
static void UpdateBeadBlockMap(void)
{
  nand_erase_block(0,MAP_BB_ADDRES * 64); // первая копия карты
  nand_erase_block(1,MAP_BB_ADDRES * 64); // вторая копия карты
        
  pmap_bb->block_address[alarm_data.PageAdressWrite >> 6] = BLOCK_BAD;
  
  pmap_bb->crc = crc32((uint8_t*)pmap_bb,MAP_BB_MAX_BLOCK,MAP_BB_MAX_BLOCK);

  nand_16bit_write_page(0,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); // первая копия карты
  nand_16bit_write_page(1,(uint16_t*)pmap_bb,MAP_BB_ADDRES * 64); // вторая копия карты
  
  t_info.bad_block_number++;
}

////////////////////////////////////////////////////////////////////////////////
// CheckTheEndOfTheNandMemory
////////////////////////////////////////////////////////////////////////////////
int CheckTheEndOfTheNandMemory(void)
{
  if(alarm_data.state == STATE_WAIT_END_NFLASH)
  {
    if((alarm_data.PageAdressWrite) >= MAX_PAGE_IN_NAND)
    {
      
      if((alarm_data.nand_real_write[0] == 1) && (alarm_data.nand_real_write[1] == 1))
      {
        t_info.server_error = CheckServer();
      }
      
      alarm_data.PageAdressErase = BEGIN_PAGE & 0xffffffc0;  
      
      nand_erase_super_block(alarm_data.index ^ 0x1,alarm_data.PageAdressErase);
      alarm_data.state = STATE_ERASE_NFLASH_DONE;
      return F_CLOSE;
    }
  }
  return F_OPEN_WRITE;
}
////////////////////////////////////////////////////////////////////////////////
// CheckChangeNandMemory
////////////////////////////////////////////////////////////////////////////////
static int CheckChangeNandMemory(void)
{
  
  if(alarm_data.state == STATE_ERASE_NFLASH_DONE)
  {

// ПЕРЕХОД НА ДРУГОЙ КОМПЛЕКТ !!!!!!
    
    SaveTotalTimeTotalCycle(alarm_data.index);
    
    t_info.crc_binar_error = 0;
    t_info.crc_rd_nand_error = 0;
    t_info.crc_wr_nand_error = 0;
    t_info.f_write_error = 0;
    
    alarm_data.nand_real_write[alarm_data.index] = 1;  // флаг заполнености флеш
      
    alarm_data.index ^= 1;
        
    alarm_data.super_block_real_write = 0;  
    alarm_data.super_block_time[0] = GetTime();
    alarm_data.super_block_time[1] = GetTime();
    alarm_data.PageRealWrite = 0;
 
    alarm_data.PageAdressWrite     = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_begin   = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_prev    = BEGIN_PAGE & 0xffffffc0;    
    alarm_data.super_block_current = BEGIN_PAGE & 0xffffffc0;   
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
  int32_t  adr_dif = alarm_data.PageAdressErase - alarm_data.PageAdressWrite;
  
 // if(alarm_data.state == STATE_ERASE_NFLASH) 
  //{
    // стирание флеш  перед переходом на новый цикл
    //nand_erase_super_block(index ^ 0x1,alarm_data.PageAdressErase);
    //alarm_data.state = STATE_ERASE_NFLASH_DONE;
 // }
  // стирание флеш в активном блоке 
 // else 
    
  if((adr_dif > 0) && (adr_dif <= PAGE_IN_SBLOCK))
  {
    if(alarm_data.PageAdressErase < MAX_PAGE_IN_NAND)
    {
      nand_erase_block(index,(unsigned long)alarm_data.PageAdressErase);
      alarm_data.PageAdressErase += 64; // адрес стираемой страницы
    }
 
  }    
}


////////////////////////////////////////////////////////////////////////////////
// nand_erase_super_block
////////////////////////////////////////////////////////////////////////////////
void nand_erase_super_block(uint32_t id, uint32_t page)
{ 
  alarm_data.PageAdressErase = page & 0xffffffc0;
  
  for(int i = 0; i < PAGE_IN_SBLOCK; i+= 64)
  {
    nand_erase_block(id,(unsigned long)alarm_data.PageAdressErase);	
    alarm_data.PageAdressErase+= 64;
  }
}

////////////////////////////////////////////////////////////////////////////////
// проверка сервера
////////////////////////////////////////////////////////////////////////////////
static uint32_t CheckServer(void)
{
  uint32_t f_num = 0;
    
  uint32_t index = alarm_data.index ^ 1;   
    
  for(int i = 0; i < MAX_FILE; i++)
  {
    if(server_control[index][i] == 1)
    {
      f_num++;
    }
  }

  return f_num;
}

void SetServerControlBit(uint32_t sb_index)
{
  if(sb_index < MAX_FILE)
  {
    uint32_t index = alarm_data.index;
    server_control[index][sb_index] = 1;
  }
}
    
////////////////////////////////////////////////////////////////////////////////
// SaveTotalTimeTotalCycle
////////////////////////////////////////////////////////////////////////////////
void SaveTotalTimeTotalCycle(int id)
{
  LoadTwsStruct();
  
  nand_erase_block(0,TWS_ADDRES * 64); // первая копия 
  nand_erase_block(1,TWS_ADDRES * 64); // вторая копия 
  
  delay(100);
  
  tws.total_cucle[id] += 1;
  tws.total_time += total_timer;
  tws.crc  = crc32((uint8_t*)&tws,sizeof(total_work_struct)-4,sizeof(total_work_struct)); 
   
  total_timer = 0;
  
  nand_16bit_write_page_ext(0,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); // первая копия 
  nand_16bit_write_page_ext(1,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); // вторая копия 
          
  
  delay(100);
 
}
