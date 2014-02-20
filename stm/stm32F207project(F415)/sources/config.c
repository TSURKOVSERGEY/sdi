#include "main.h"
#include "spi_f415.h"
#include "nand_hw_driver.h"

extern total_info_struct      t_info;
extern alarm_struct           alarm_data;
extern bad_block_map_struct*  pmap_bb;
extern total_work_struct      tws;
extern adpcm_page_ctrl_struct adpcm_ctrl[MAX_CHANNEL];
extern tab_struct             tab;
extern super_block_struct*     prsb;
extern super_block_struct*     pwsb;
extern adpcm_page_struct*      padpcm[2][MAX_CHANNEL];   
extern uint8_t                 adpcm_ready;
extern uint16_t                spi_dma_buffer[2][SPI_RX_DMA];
extern void*                   pf;
  
void Data_Config(int mode)
{
  if(mode == DATA_CONFIG_NEW)
  {
    memset(adpcm_ctrl,0,sizeof(adpcm_ctrl));
    memset(&alarm_data,0,sizeof(alarm_data));
    
    memset(prsb,0,sizeof(super_block_struct));
    memset(pwsb,0,sizeof(super_block_struct));
    memset(padpcm[0][0],0,sizeof(adpcm_page_struct)*MAX_CHANNEL*2);
      
    memset(tab,0,sizeof(tab_struct));
    memset(spi_dma_buffer,0,sizeof(spi_dma_buffer));
    
    adpcm_ready = 0;
    pf = NULL;
    
    alarm_data.PageAdressErase     = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.PageAdressWrite     = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_begin   = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_prev    = BEGIN_PAGE & 0xffffffc0;       
    alarm_data.super_block_current = BEGIN_PAGE & 0xffffffc0;   
  
    t_info.f207_mode = F207_IDLE_MODE;
    t_info.f415_mode = F415_IDLE_MODE;
  
  }
  else if(mode == DATA_CONFIG_LOAD)
  {
        
    alarm_data.PageAdressErase     = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.PageAdressWrite     = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_begin   = BEGIN_PAGE & 0xffffffc0;   
    alarm_data.super_block_prev    = BEGIN_PAGE & 0xffffffc0;       
    alarm_data.super_block_current = BEGIN_PAGE & 0xffffffc0;  
    
    t_info.f207_mode = F207_IDLE_MODE;
    t_info.f415_mode = F415_IDLE_MODE;
    pf = NULL;
    LoadTwsStruct();
    alarm_data.index = tws.index;
    
    t_info.mask[0]  = 1;   // (0)     ошибка конфигурации сетевого драйвера
    t_info.mask[1]  = 1;   // (1+2+3) ошибка конфигурации udp сокета
    t_info.mask[2]  = 1;
    t_info.mask[3]  = 1;
    t_info.mask[4]  = 1;   // (4)     ошибка динамической памяти
    t_info.mask[5]  = 1;   // (5)     ошибка канала i2c(f207->f415)
    t_info.mask[6]  = 1;   // (6+7)   ошибка чтения структуры инициализационных сетевых параметров (*)
    t_info.mask[7]  = 1;
    t_info.mask[8]  = 1;   // (8+9)   ошибка чтения структуры общего времени работы и количества циклов записи FLASH
    t_info.mask[9]  = 1;
    t_info.mask[10] = 1;  // (10+11) ошибка инициализации карты файловой системы (две копии)
    t_info.mask[11] = 1;
    t_info.mask[12] = 1;  // (12+13) ошибка инициализации файловой системы
    t_info.mask[13] = 1;
    t_info.mask[14] = 1;  // (14)    ошибка RTC (*)
    t_info.mask[15] = 1;  // (15)    ошибка записи файла 
    t_info.mask[16] = 0;  // (16)    ошибка сервера ( колличество несчитанных файлов )
    t_info.mask[17] = 1;  // (17)    ошибка канала spi(f415->f207) 
    t_info.mask[18] = 1;  // (18)    ошибка канала spi(f415->pga112)
    t_info.mask[19] = 1;  // (19)    ошибка инициализации АЦП 
    t_info.mask[20] = 0;  // (20)    ошибка crc данных (по binar от f415) 
    t_info.mask[21] = 0;  // (21)    ошибка crc данных страницы FLASH (проверка перед записью)
    t_info.mask[22] = 0;  // (22)    ошибка crc данных страницы FLASH (проверка после чтения)
    
    t_info.initial_done = 1;
    
  }

  

 
}

void LoadTwsStruct(void)
{
  for(int i = 0; i < 2; i++)
  { 
    t_info.read_total_time_error[i] = 0;
      
    nand_16bit_read_page_ext(i,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); 

    if(tws.crc != crc32((uint8_t*)&tws,sizeof(total_work_struct)-4,sizeof(total_work_struct))) 
    {
       t_info.read_total_time_error[i] = 1;
      
       if(i == 1)
       {
         if(t_info.read_total_time_error[0] == 0)
         {
           nand_16bit_read_page_ext(0,(uint16_t*)&tws,TWS_ADDRES * 64,sizeof(total_work_struct)); 
         }
         else
         {
           return;
         }
       }
    }  
  }
}


void SysTim_Config(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);  
}

void RTC_Config(void)
{
  uint32_t Timeout = 0xfffff;
    
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR |= PWR_CR_DBP;
  RCC->BDCR |=  RCC_BDCR_BDRST;
  RCC->BDCR &= ~RCC_BDCR_BDRST;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_AHB1Periph_BKPSRAM, ENABLE);
  RCC_LSEConfig(RCC_LSE_ON);
  
  while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) 
  { if((Timeout--) == 0) 
    { t_info.rtc_error = 1;
      return;
    }
  }
  
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  RTC_WaitForSynchro();
  RTC_ITConfig(RTC_IT_TS, ENABLE);
  RCC_RTCCLKCmd(ENABLE);
}

void CRC_Config(void)
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
}

void SRAM_Config(void)
{
  FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
  FSMC_NORSRAMTimingInitTypeDef  p;
  GPIO_InitTypeDef GPIO_InitStructure; 
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE | RCC_AHB1Periph_GPIOF |
                         RCC_AHB1Periph_GPIOG, ENABLE);

  RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE); 
  
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource11, GPIO_AF_FSMC); 
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FSMC);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_4  | GPIO_Pin_5  | 
                                GPIO_Pin_8  | GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 |
                                GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);


  GPIO_PinAFConfig(GPIOE, GPIO_PinSource0 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource1 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource7 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource8 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource9 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource10 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource11 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource12 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource13 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource14 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOE, GPIO_PinSource15 , GPIO_AF_FSMC);


  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_7  | GPIO_Pin_8  |  
                                GPIO_Pin_9  | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 |
                                GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;

  GPIO_Init(GPIOE, &GPIO_InitStructure);


  GPIO_PinAFConfig(GPIOF, GPIO_PinSource0 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource1 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource2 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource3 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource4 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource5 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource12 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource13 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource14 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOF, GPIO_PinSource15 , GPIO_AF_FSMC);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  | GPIO_Pin_3  | 
                                GPIO_Pin_4  | GPIO_Pin_5  | GPIO_Pin_12 | GPIO_Pin_13 |
                                GPIO_Pin_14 | GPIO_Pin_15;      

  GPIO_Init(GPIOF, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOG, GPIO_PinSource0 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource1 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource2 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource3 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource4 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource5 , GPIO_AF_FSMC);
  GPIO_PinAFConfig(GPIOG, GPIO_PinSource10 , GPIO_AF_FSMC);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  | GPIO_Pin_1  | GPIO_Pin_2  | GPIO_Pin_3 | 
                                GPIO_Pin_4  | GPIO_Pin_5  | GPIO_Pin_10;      

  GPIO_Init(GPIOG, &GPIO_InitStructure);

  p.FSMC_AddressSetupTime = 5;
  p.FSMC_AddressHoldTime = 5;
  p.FSMC_DataSetupTime = 10;
  p.FSMC_BusTurnAroundDuration = 0;
  p.FSMC_CLKDivision = 0;
  p.FSMC_DataLatency = 0;
  p.FSMC_AccessMode = FSMC_AccessMode_A;

  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM3;
  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_PSRAM;
  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;  
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

  FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure); 

  FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM3, ENABLE); 

}

void LED_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(GPIOI, &GPIO_InitStructure);
  
  GPIO_WriteBit(GPIOI,GPIO_Pin_0,Bit_SET);  
  GPIO_WriteBit(GPIOI,GPIO_Pin_1,Bit_SET);  
  
}

uint32_t f415_SPI_Config(void)
{
  SPI_Config();
  //SPI_TxDma_Config();
  SPI_RxDma_Config(ENABLE);
  
  return 0;
}

void TIM2_Config(void)
{
  TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  
  TIM_TimeBaseStructure.TIM_Period = 60000000/8000;
  TIM_TimeBaseStructure.TIM_Prescaler = 7;
  
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
  TIM_ITConfig(TIM2,TIM2_IRQn, ENABLE);  
  NVIC_EnableIRQ(TIM2_IRQn);  
  
  TIM_Cmd(TIM2, ENABLE);
  
}


