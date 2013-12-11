#include "spi_PGA112.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_i2c.h"
#include "main.h"



GPIO_TypeDef* GPIO_PORT[CSn] = {CS0_GPIO_PORT,
                                CS1_GPIO_PORT,
                                CS2_GPIO_PORT,
                                CS3_GPIO_PORT,
                                CS4_GPIO_PORT,
                                CS5_GPIO_PORT,
                                CS6_GPIO_PORT,
                                CS7_GPIO_PORT,
                                CS8_GPIO_PORT,
                                CS9_GPIO_PORT,
                                CS10_GPIO_PORT,
                                CS11_GPIO_PORT,
                                CS12_GPIO_PORT,
                                CS13_GPIO_PORT,
                                CS14_GPIO_PORT,
                                CS15_GPIO_PORT};

const uint16_t GPIO_PIN[CSn]  = {CS0_PIN,
                                 CS1_PIN,
                                 CS2_PIN,
                                 CS3_PIN,
                                 CS4_PIN,
                                 CS5_PIN,
                                 CS6_PIN,
                                 CS7_PIN,
                                 CS8_PIN,
                                 CS9_PIN,
                                 CS10_PIN,
                                 CS11_PIN,
                                 CS12_PIN,
                                 CS13_PIN,
                                 CS14_PIN,                                 
                                 CS15_PIN};


const uint32_t GPIO_CLK[CSn]  = {CS0_GPIO_CLK,
                                 CS1_GPIO_CLK,
                                 CS2_GPIO_CLK,
                                 CS3_GPIO_CLK,
                                 CS4_GPIO_CLK,
                                 CS5_GPIO_CLK,
                                 CS6_GPIO_CLK,
                                 CS7_GPIO_CLK,
                                 CS8_GPIO_CLK,
                                 CS9_GPIO_CLK,
                                 CS10_GPIO_CLK,
                                 CS11_GPIO_CLK,
                                 CS12_GPIO_CLK,
                                 CS13_GPIO_CLK,
                                 CS14_GPIO_CLK,
                                 CS15_GPIO_CLK};


void SPI_PGA112Config(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
    
  for(int i = 0; i < MAX_CHANNEL; i++) 
  {
    SPI_PGA112_CS_GPIO_Config((CS_TypeDef)i);
    PGA112_Clr_CS((CS_TypeDef)i);
  }
 
  SPI_PGA112GPIO_Config();

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE); 
  SPI_I2S_DeInit(SPI2);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 0;
  SPI_Init(SPI2, &SPI_InitStructure);
  SPI_SSOutputCmd(SPI2,DISABLE);
  SPI_NSSInternalSoftwareConfig(SPI2,SPI_NSSInternalSoft_Set);
  SPI_Cmd(SPI2, ENABLE);  
}

void SPI_PGA112GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_15; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
  
  GPIO_PinLockConfig(GPIOB,GPIO_Pin_13 | GPIO_Pin_15);

}


void SPI_PGA112_CS_GPIO_Config(CS_TypeDef CS)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  RCC_AHBPeriphClockCmd(GPIO_CLK[CS], ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[CS];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_Init(GPIO_PORT[CS], &GPIO_InitStructure);
  GPIO_PinLockConfig(GPIO_PORT[CS],GPIO_PIN[CS]);
}


void PGA112_WriteData(int CS, unsigned short gain, unsigned short channel)
{
  int i;
  
  PGA112_Set_CS((CS_TypeDef)CS);
  
  for(i = 0xffff; i > 0; i--);
  
  SPI_I2S_SendData(SPI2,channel | gain | 0x2a00);
  
  while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) != RESET);
  
  for(i = 0xffff; i > 0; i--);
  
  PGA112_Clr_CS((CS_TypeDef)CS);
  
}


void PGA112_Set_CS(CS_TypeDef CS){ GPIO_PORT[CS]->BSRRH = GPIO_PIN[CS]; }

void PGA112_Clr_CS(CS_TypeDef CS){ GPIO_PORT[CS]->BSRRL = GPIO_PIN[CS]; }

