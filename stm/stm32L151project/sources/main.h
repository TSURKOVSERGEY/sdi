#include "stm32l1xx.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_crc.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_i2c.h"
#include "stm32l1xx_tim.h"
#include "stm32l1xx_adc.h"
#include "stm32l1xx_dma.h"
#include "stm32l1xx_spi.h"

#include "misc.h"
#include "stdio.h"

#define SYSTEMTICK_PERIOD_MS 100

#define DLE          0x0010
#define STX          0x0002
#define ETX          0x0003
#define DLE_DLE      0x00100010
#define DLE_STX      0x00100002 
#define DLE_ETX      0x00100003 

#define DMA_ADC_SIZE            16
#define DMA_SPI_SIZE            2048

#define MAX_CHANNEL             16
#define MAX_SAMPLE              224

#define F415_AUDIO_STREAM       1

#define F415_CHECK_I2C_CONNECT  1
#define F415_CHECK_SPI_CONNECT  2
#define F415_SET_GAIN           3
#define F415_GET_GAIN           4
#define F415_START_STREAM       5
#define F415_STOP_STREAM        6


#define GN0     0x0
#define GN1     0x10
#define GN2     0x20
#define GN3     0x30
#define GN4     0x40
#define GN5     0x50
#define GN6     0x60
#define GN7     0x70
#define GN8     0x80
#define GN9     0x90
#define GN10    0xA0
#define GN11    0xB0
#define GN12    0xC0
#define GN13    0xD0
#define GN14    0xE0
#define GN15    0xF0

#define CH0     0x0
#define CH1     0x1

#pragma pack(push,1)
 typedef struct
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint8_t  data[256];
 } f207_message_struct;
#pragma pack(pop)

#pragma pack(push,1)   
 typedef struct 
 { int16_t  prevsample;
   int8_t   previndex;
   int8_t   channel_id;
   uint8_t  adpcm_data[MAX_SAMPLE / 2];
   uint32_t time[2];
   uint32_t crc;
 } adpcm_struct;
#pragma pack(pop)
 
#pragma pack(push,1)   
 typedef struct
 { uint32_t crc;
   uint16_t msg_id;
   uint16_t msg_counter;
   adpcm_struct adpcm_block[MAX_CHANNEL];
 } adpcm_message_struct;
#pragma pack(pop)
 

void TIM2_Disable(void);
void TIM2_Enable(void);
void TIM2_Config(void);
void CRC_Config(void);
void delay(uint32_t nCount);
void SysTim_Config(void);
void ADC_DMA_Config(void);
void f207_TransmitHandler(void);
void f207_TransmitHandler(void);
uint32_t crc32(void* pcBlock,  uint32_t len, uint32_t tot_len);