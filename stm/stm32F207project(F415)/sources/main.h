#include "stm32f2xx.h"
#include "stm32f2x7_eth.h"
#include "stm32f2x7_eth_bsp.h"   
#include "stm32f2xx_conf.h"
#include "stm32f2xx_tim.h"
#include "stm32f2xx_rtc.h"
#include "stm32f2xx_pwr.h"
#include "stm32f2xx_dma.h"
#include "stm32f2xx_crc.h"  
#include "stm32f2xx_i2c.h"   
#include "stm32f2xx_spi.h"   
#include "netconf.h"
#include "lwip\udp.h"
#include "lwip\tcp.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

////////////////////////////////////////////////////////////////////////////////
// SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTEM * SYSTE 
////////////////////////////////////////////////////////////////////////////////

#define MAX_UDP_SOCK         3
#define HEADER_MODE          1
#define DATA_MODE            2

#define SYSTEMTICK_PERIOD_MS 100

#define	sram_bank1	     0x60000000
#define	sram_bank2	     0x64000000
#define sram_bank3	     0x68000000
#define sram_bank4	     0x6C000000

#define AT45ADR_ETH_PARAM    0
#define AT45ADR_MAP_BB       128

#define BLOCK_GOOD           0x1
#define BLOCK_BAD            0x2

#define MAX_ERASE_PAGE       PAGE_IN_SBLOCK 
#define MAX_PAGE_IN_NAND     2048 // максимальное колличество страниц на всей флеш
#define PAGE_IN_BLOCK        64
#define MAX_DATA_PAGE        256// 2048  // максимальное кол-во страниц в супер блоке
#define PAGE_IN_SBLOCK       (MAX_DATA_PAGE + 64)
#define MAX_SBLOCK           100
#define MAX_BLOCK            (MAX_SBLOCK * PAGE_IN_SBLOCK / 64)

#define SERV                 0
#define PTUK                 1
#define SNTP                 2

#define ETH_INFO_CONNECT     1
#define ETH_INFO_SRAM        2
#define ETH_INFO_L151        3
#define ETH_INFO_ASTREAM     4

////////////////////////////////////////////////////////////////////////////////

#define CHECK_CONNECT        1
#define START_AUDIO_STREAM   2
#define STOP_AUDIO_STREAM    3

#define GET_SYS_INFO         20
//#define GET_SYS_INFO_0       20
//#define GET_SYS_INFO_1       21
//#define GET_SYS_INFO_2       22
//#define GET_SYS_INFO_3       23

#define GET_SB_HEADER        30
#define GET_PAGE             31
#define GET_TIME             32
#define GET_ETH_PARAM        33
//#define GET_BM               34
//#define GET_FIXED            35
//#define GET_CFI              36

#define SET_GAIN             40
//#define SET_INDEX            41
#define SET_TIME             42
#define SET_ETH_PARAM        43
#define SET_MAP_BB           44
//#define SET_FIXED            45
//#define SET_SYS_INFO_0       46
//#define SET_CFI              47

////////////////////////////////////////////////////////////////////////////////

#define SET_MAC              1
#define SET_GW               2
#define SET_IP               3
#define SET_MASK             4
#define SET_UDP_RX1_PORT     5
#define SET_UDP_RX2_PORT     6

////////////////////////////////////////////////////////////////////////////////

#define SUPER_BLOCK_CLEAR    1
#define SUPER_BLOCK_OPEN     2
#define SUPER_BLOCK_RECORDED 3
#define SUPER_BLOCK_READ     4
 
#define SUPER_BLOCK_ID       0xA1B2C3D4
#define ADPCM_PAGE_ID        2
#define EONE_PAGE_ID         3
#define ALARM_ID             4

#define I2C_TIMEOUT         0xfffff
#define I2C_SPEED            100000

#define F415_AUDIO_STREAM         1
#define F415_CHECK_I2C_CONNECT    1
#define F415_CHECK_SPI_CONNECT    2
#define F415_SET_GAIN             3
#define F415_GET_GAIN             4
#define F415_START_STREAM         5
#define F415_STOP_STREAM          6

#define STATE_WAIT_END_NFLASH     0 
#define STATE_ERASE_NFLASH        1
#define STATE_ERASE_NFLASH_DONE   2
#define STATE_CHANGE_NFLASH       3

#define MAX_CHANNEL          16
#define MAX_SAMPLE           224
#define MAX_BOOKMARK         16
#define SPI_RX_DMA           1500  
#define SPI_RX_IRQ           100

////////////////////////////////////////////////////////////////////////////////
// STUFFING * STUFFING * STUFFING * STUFFING * STUFFING * STUFFING * STUFFING *  
////////////////////////////////////////////////////////////////////////////////
  
#define  DLE                 0x0010
#define  STX                 0x0002
#define  ETX                 0x0003
#define  DLE_DLE             0x00100010
#define  DLE_STX             0x00100002 
#define  DLE_ETX             0x00100003 

////////////////////////////////////////////////////////////////////////////////
// TYPE_DEF * TYPE_DEF * TYPE_DEF * TYPE_DEF * TYPE_DEF * TYPE_DEF * TYPE_DEF *     
////////////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)   
 typedef struct 
 { int16_t  prevsample;
   int8_t   previndex;
   int8_t   channel_id;
   int8_t   adpcm_data[MAX_SAMPLE / 2];
   uint64_t time;
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

#define ADPCM_BLOCK_SIZE  4048 // 4 + (36 * (224/2)) + 8 + 4 !!!! 4048
#define ADPCM_MAX_BLOCK   36
 
#pragma pack(push,1)
 typedef struct
 { int16_t  prevsample;
   int8_t   previndex;
   int8_t   id;
   int8_t   adpcm_data[ADPCM_MAX_BLOCK][MAX_SAMPLE / 2];  
   uint64_t time;
   uint32_t crc;   
 } adpcm_page_struct;
#pragma pack(pop) 
 
#pragma pack(push,1) 
typedef struct 
{ int id;
  int adr;
  int wait;
  int done;
  uint32_t crc;
} adpcm_page_ctrl_struct;
#pragma pack(pop) 

#define UDP_DATA_SIZE    1026 
#define UDP_HEADER_SIZE  12
#define UDP_MESSAGE_SIZE UDP_HEADER_SIZE + UDP_DATA_SIZE

#pragma pack(push,1)   
 typedef struct  
 { uint32_t msg_crc;
   uint32_t msg_id;
   uint32_t msg_len; 
   uint8_t  data[UDP_DATA_SIZE];
 } udp_message_struct;
#pragma pack(pop)

#define MAX_ERROR 10
 
#pragma pack(push,1) 
 typedef struct  
 { uint32_t eth_bsp_error;
   uint32_t eth_udp_error[MAX_UDP_SOCK];
   uint32_t IS61_error;
   uint32_t f415_spi_error;
   uint32_t f415_i2c_error;
   uint32_t mt29f4_error;
   uint32_t at24_error;
   uint32_t map_bb_error;
   uint32_t rtc_error;
   uint32_t write_driver_error;
 } initial_info_struct;
#pragma pack(pop)

#pragma pack(push,1)  
 typedef struct   
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint8_t data[256];
 } f415_message_struct;
#pragma pack(pop)
 
#pragma pack(push,1)   
 typedef struct
 { uint16_t data[512];
   uint16_t msg_len;
 } byte_stuffing_message_struct;  
#pragma pack(pop) 
 
 #pragma pack(push,1)
 typedef struct
 { uint32_t  page_index;
   uint32_t  page_address;  
 } index_pointer_struct;
#pragma pack(pop)

#pragma pack(push,1)
 typedef struct
 { uint32_t id;
   uint8_t  status;
   uint64_t time_open;
   uint64_t time_close;
   uint32_t sb_num;
   uint32_t page_real_write;
   uint32_t super_block_prev;
   uint32_t super_block_next;
   index_pointer_struct ips[MAX_DATA_PAGE];  
 } super_block_struct;
#pragma pack(pop)
 
#pragma pack(push,1)   
 typedef struct 
 { uint32_t unit_index;     // индекс комплекиа нанд-флеш
   uint32_t sbrw;           // индекс последнего записанного файла
   uint64_t time[2];        // время  первого - последнего записанного файла
 } tab_struct[2];
#pragma pack(pop) 
 
#pragma pack(push,1)
 typedef struct
 { uint32_t index; 
   uint32_t state;
   uint32_t super_block_real_read;
   uint32_t super_block_real_write;
   uint64_t super_block_time[2];
   
   uint32_t PageRealWrite;
   uint32_t PageRealErase;
      
   uint32_t PageAddress; 

   uint32_t ErasePageCounter;
 
   uint32_t super_block_begin;
   uint32_t super_block_prev;
   uint32_t super_block_current;
 
   uint32_t err_crc_binar;
   uint32_t err_crc_wr_Nand;
   uint32_t err_crc_rd_Nand;

   tab_struct bms[MAX_BOOKMARK];
 } alarm_struct;
#pragma pack(pop) 
 
#pragma pack(push,1)
 typedef struct
 { uint32_t bad_block_number;
   uint8_t  block_address[MAX_BLOCK];
   uint32_t crc;
 } bad_block_map_struct;
#pragma pack(pop)  

#pragma pack(push,1)   
 typedef struct 
 { uint8_t  MAC_ADR[6];
   uint8_t  GW_ADR[4];
   uint8_t  IP_ADR[4];
   uint8_t  MASK[4];
   uint32_t UDP_RX_PORT[MAX_UDP_SOCK];
   uint32_t UDP_TX_PORT[MAX_UDP_SOCK];
   struct ip_addr addr[MAX_UDP_SOCK];
   uint32_t crc;
} ethernet_initial_struct;
#pragma pack(pop) 

#define PAGE_HEADER_SIZE 6
 
#pragma pack(push,1)   
  typedef struct
  { uint8_t  page_index;
    uint8_t  unit_index;
    uint32_t page_address;
  } page_header_struct;
#pragma pack(pop) 


////////////////////////////////////////////////////////////////////////////////
// FUNCTION_DEFINITION * FUNCTION_DEFINITION * FUNCTION_DEFINITION * FUNCTION_DE
////////////////////////////////////////////////////////////////////////////////

void delay(uint32_t delay);
void udp_rx0_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port);
void udp_rx1_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port);
void udp_rx2_handler(void *arg, struct udp_pcb *upcb,struct pbuf *p, struct ip_addr *addr, u16_t port);

void TIM7_Config(void);
void LED_Config(void);
void SRAM_Config(void);
void RTC_Config(void);
void ETH_Config(void);
void SysTim_Config(void);
void RNG_Config(void);
void CRC_Config(void);
uint32_t f415_SPI_Config(void);
uint32_t f415_I2C_Config(void);
uint32_t SuperBlock_Config(void);

void nand_erase_super_block(uint32_t id, uint32_t page);
void nand_erase_handler(void);
void nand_sb_write_handler(void);
void nand_sb_read_handler(void);
void SetTime(uint8_t *time);

uint32_t crc32(void* pcBlock,  uint32_t len, uint32_t tot_len);
uint32_t crc32_t(uint32_t crc, void * pcBlock, uint32_t len, uint32_t tot_len);
void SRAM_WriteBuffer(uint16_t* pBuffer, uint32_t WriteAddr, uint32_t NumHalfwordToWrite);
void SRAM_ReadBuffer(uint16_t* pBuffer, uint32_t ReadAddr, uint32_t NumHalfwordToRead);
void SendEthInfoMessage(uint32_t msg_id, uint32_t param);
uint32_t SRAM_Test(void);
uint64_t GetTime(void);
void GetSuperBlockHeader(uint32_t un_index, uint32_t sb_index);
void GetSuperBlockPage(uint32_t page_address);
void cmd_handler(int id);
void SysTickTimer_Config(void);
