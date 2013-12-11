#include "stm32f4xx_conf.h"

#define DMA_STREAM   DMA1_Stream5

void SPI_f207GPIO_Config(void);
void SPI_f207Config(void);
void SPI_TxDma_Config(void);
void SPI_f207SendMessage(void *pbuffer, uint32_t len);

/*

void SPI_L151Config(uint16_t speed);
void SPI_L151GPIO_Config(void);
void SPI_RxInt_Config(void);
void SPI_RxDma_Config(void);
void SPI_TxDma_Config(void);
void L151_WriteData(uint16_t data);
void L151_SendMessage(uint16_t *pbuffer, int size);

*/