#include "main.h"


#define GPIO_PIN_PORTA    GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7
#define GPIO_PIN_PORTB    GPIO_Pin_0 | GPIO_Pin_1 
#define GPIO_PIN_PORTC    GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5   

extern uint16_t raw_buffer[DMA_ADC_SIZE];

void SysTim_Config(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 100);
}


void TIM2_Config(void)
{
  TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 32000000 / 8000; 
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);  
}

void TIM2_Enable(void)
{
  TIM_ITConfig(TIM2,TIM2_IRQn, ENABLE);  
  NVIC_EnableIRQ(TIM2_IRQn);  
  TIM_Cmd(TIM2, ENABLE);
}

void TIM2_Disable(void)
{
  TIM_ITConfig(TIM2,TIM2_IRQn,DISABLE);  
  NVIC_EnableIRQ(TIM2_IRQn);  
  TIM_Cmd(TIM2,DISABLE);
}

void CRC_Config(void)
{
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
}


void ADC_DMA_Config(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  GPIO_InitTypeDef      GPIO_InitStructure;

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); 
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(((ADC_TypeDef*)ADC1_BASE)->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) &raw_buffer[0];
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = MAX_CHANNEL;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  DMA_Cmd(DMA1_Channel1, ENABLE);  
 
  RCC_HSICmd(ENABLE);  
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC,ENABLE);    
  GPIO_InitStructure.GPIO_Pin  = GPIO_PIN_PORTA;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_Init(GPIOA,&GPIO_InitStructure); 
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN_PORTB;
  GPIO_Init(GPIOB,&GPIO_InitStructure); 
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN_PORTC;
  GPIO_Init(GPIOC,&GPIO_InitStructure); 
 
  while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);
 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); 
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInit (&ADC_CommonInitStructure);
  
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = MAX_CHANNEL;
  ADC_Init(ADC1, &ADC_InitStructure);  
 
  ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
  
  #define ADC_SampleTime_My_Cycles ADC_SampleTime_4Cycles
  
  ADC_RegularChannelConfig(ADC1,ADC_Channel_0, 1, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 2, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_2, 3, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_3, 4, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_5, 5, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_6, 6, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_7, 7, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_8, 8, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_9, 9, ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_10,10,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_11,11,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_12,12,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_13,13,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_14,14,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_15,15,ADC_SampleTime_My_Cycles);
  ADC_RegularChannelConfig(ADC1,ADC_Channel_18,16,ADC_SampleTime_My_Cycles);
  
  ADC_DMACmd(ADC1, ENABLE);
  ADC_Cmd(ADC1, ENABLE);
  
  while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADONS) == RESET);

  ADC_SoftwareStartConv(ADC1);

}
