#include "spi_f207.h"
#include "main.h"

DMA_InitTypeDef   DMA_TxInitStructure;

void SPI_f207Config(void)
{ 
  SPI_InitTypeDef  SPI_InitStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE); 
  SPI_f207GPIO_Config();
  SPI_Cmd(SPI3, ENABLE); 
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; 
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 0;
  SPI_Init(SPI3,&SPI_InitStructure);
  SPI_SSOutputCmd(SPI3,DISABLE);
  SPI_I2S_DMACmd(SPI3,SPI_I2S_DMAReq_Tx, ENABLE);  
  SPI_TxDma_Config();
}


void SPI_f207GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI3);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI3);
  GPIO_PinLockConfig(GPIOB,GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5);
}

void SPI_TxDma_Config(void)
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  DMA_DeInit(DMA_STREAM);   
  while(DMA_GetCmdStatus(DMA_STREAM) != DISABLE);
  
  DMA_TxInitStructure.DMA_Channel = DMA_Channel_0;  
  DMA_TxInitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR); 
  DMA_TxInitStructure.DMA_Memory0BaseAddr = 0; 
  DMA_TxInitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_TxInitStructure.DMA_BufferSize = 0;
  DMA_TxInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_TxInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_TxInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_TxInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_TxInitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_TxInitStructure.DMA_Priority = DMA_Priority_High;
  DMA_TxInitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; 
  DMA_TxInitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_TxInitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_TxInitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA_STREAM,&DMA_TxInitStructure);
}

void SPI_f207SendMessage(void *pbuffer, uint32_t len)
{
  DMA_DeInit(DMA_STREAM);   
  while(DMA_GetCmdStatus(DMA_STREAM) != DISABLE);
   
  DMA_TxInitStructure.DMA_Memory0BaseAddr = (uint32_t)pbuffer; 
  DMA_TxInitStructure.DMA_BufferSize = len;
  DMA_Init(DMA_STREAM,&DMA_TxInitStructure);
  DMA_Cmd(DMA_STREAM, ENABLE); 
}

