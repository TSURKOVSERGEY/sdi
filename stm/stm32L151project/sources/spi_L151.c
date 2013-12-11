#include "spi_L151.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_spi.h"
#include "stm32l1xx_dma.h"
#include "string.h"
#include "main.h"

extern uint16_t spi_tx_buffer[DMA_TX_SIZE];

void SPI_L151Config(uint16_t speed)
{ 
  SPI_InitTypeDef  SPI_151InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); 
  SPI_L151GPIO_Config();
  SPI_151InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_151InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_151InitStructure.SPI_DataSize = SPI_DataSize_16b;
  SPI_151InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_151InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_151InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_151InitStructure.SPI_BaudRatePrescaler = speed; //SPI_BaudRatePrescaler_2;
  SPI_151InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_151InitStructure.SPI_CRCPolynomial = 0;
  SPI_Init(SPI1,&SPI_151InitStructure);
  SPI_SSOutputCmd(SPI1,DISABLE);
  SPI_TxDma_Config();
  SPI_Cmd(SPI1, ENABLE);  
}

void SPI_L151GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3; 
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_SPI1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3,  GPIO_AF_SPI1);
  GPIO_PinLockConfig(GPIOA,GPIO_Pin_11 | GPIO_Pin_12);
  GPIO_PinLockConfig(GPIOB,GPIO_Pin_3);
}

void SPI_TxDma_Config(void)
{
  DMA_InitTypeDef   DMA_TxInitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_DeInit(DMA1_Channel3);    
  DMA_TxInitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR); 
  DMA_TxInitStructure.DMA_MemoryBaseAddr = 0;  
  DMA_TxInitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_TxInitStructure.DMA_BufferSize = 0;
  DMA_TxInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_TxInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_TxInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_TxInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_TxInitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_TxInitStructure.DMA_Priority = DMA_Priority_High;
  DMA_TxInitStructure.DMA_M2M = DMA_M2M_Disable;  
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE); 
  DMA_Init(DMA1_Channel3, &DMA_TxInitStructure);
}


void L151_SendMessage(uint16_t* pbuffer, int size)
{
  L151_WriteData(DLE);
  L151_WriteData(STX);
  
  while(size-- > 0)
  {
    L151_WriteData(*(pbuffer++));
    if(*(pbuffer-1) == DLE) L151_WriteData(DLE);
  }
  
  L151_WriteData(DLE);
  L151_WriteData(ETX);
}

void L151_WriteData(uint16_t data)
{
  SPI_I2S_SendData(SPI1,data);
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) != RESET);
}



