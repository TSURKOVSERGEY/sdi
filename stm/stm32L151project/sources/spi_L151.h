#include "stm32l1xx_conf.h"


//void SPI_L151Config(void);
void SPI_L151Config(uint16_t speed);
void SPI_L151GPIO_Config(void);
void SPI_RxInt_Config(void);
void SPI_RxDma_Config(void);
void SPI_TxDma_Config(void);
void L151_WriteData(uint16_t data);
void L151_SendMessage(uint16_t *pbuffer, int size);
//int Write16BitData(unsigned short data);
//int Write16BitMessage(unsigned short *pbuffer, int size);
   
