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

#define DONE                 0
#define FAILURE              1
#define ENDED_MEMORY         2

#define MAX_UDP_SOCK         3
#define HEADER_MODE          1
#define DATA_MODE            2

#define SYSTEMTICK_PERIOD_MS 100

#define	sram_bank1	     0x60000000
#define	sram_bank2	     0x64000000
#define sram_bank3	     0x68000000
#define sram_bank4	     0x6C000000

#define BLOCK_GOOD           0x1
#define BLOCK_BAD            0x2

#define MAP_BB_MAX_BLOCK     4092  // ������������ ���������� ������ FLASH (��� ����� ����������� ������)
#define MAP_BB_ADDRES        4095  // ���������� ����� ����� FLASH ( ������������ ����� ����������� ������ )
#define TWS_ADDRES           4094  // ���������� ����� ����� FLASH ( ������ ������� ��������� �� ����� )

#define DATA_CONFIG_LOAD     1
#define DATA_CONFIG_NEW      2
#define ERASE_MODE           1
#define NOT_ERASE_MODE       2


////////////////////////////////////////////////////////////////////////////////
// MT24 array organization
// �������� - 2048 ���� �������������� ������, + 64 ���� ���������
// ���� - 64 ��������
// ����� 4096 ������
////////////////////////////////////////////////////////////////////////////////

#define SB_HEADER_SIZE       33 // ������ ��������� ��������� ����� (��� crc � IPS)
#define BEGIN_PAGE           (MAX_PAGE_IN_NAND - 6000) //0
#define MAX_FILE             1000
#define MAX_ERASE_PAGE       PAGE_IN_SBLOCK 
#define MAX_PAGE_IN_NAND     261888 // (4092 * 64 )  // ������������ ����������� ������� �� ���� ���� ADR = (0..4091)
#define PAGE_IN_BLOCK        64
#define MAX_DATA_PAGE        256 // ������������ ���-�� ������� � ����� �����
#define PAGE_IN_SBLOCK       ( MAX_DATA_PAGE + 64 )
#define MAX_SBLOCK           100
#define MAX_BLOCK            ( MAX_SBLOCK * PAGE_IN_SBLOCK / 64 ) // (100 * (256 + 64) / 64 = 500) // �� ����� (4096 - 4 = 4092)


#define SERV                 0
#define PTUK                 1
#define SNTP                 2

#define ETH_INFO_CONNECT     1
#define ETH_INFO_SRAM        2
#define ETH_INFO_L151        3
#define ETH_INFO_ASTREAM     4

#define F415_IDLE_MODE            0
#define F415_STOP_MODE            1
#define F415_AUDIO_STREAM_MODE    2

#define F207_IDLE_MODE            0
#define F207_STOP_MODE            1
#define F207_AUDIO_STREAM_MODE    2

////////////////////////////////////////////////////////////////////////////////
// �������������� ������� ������

#define CHECK_CONNECT        1
#define START_AUDIO_STREAM   2
#define STOP_AUDIO_STREAM    3
#define NEW                  4

#define GET_SYS_INFO         20
#define GET_TOTAL_INFO       21

#define GET_SB_HEADER        30
#define GET_PAGE             31
#define GET_TIME             32
#define GET_ETH_PARAM        33

#define SET_GAIN             40
#define SET_TIME             42
#define SET_ETH_PARAM        43
#define FORMAT_MAP_BB        44
#define FORMAT_TWS           45

// ��� ������� ������� ��������

#define SET_MAC              1
#define SET_GW               2
#define SET_IP               3
#define SET_MASK             4
#define SET_UDP_RX1_PORT     5
#define SET_UDP_TX1_PORT     6
#define SET_UDP_RX2_PORT     7
#define SET_UDP_TX2_PORT     8
#define SET_UDP_RX3_PORT     9
#define SET_UDP_TX3_PORT     10

////////////////////////////////////////////////////////////////////////////////

#define SUPER_BLOCK_CLEAR    1
#define SUPER_BLOCK_OPEN     2
#define SUPER_BLOCK_RECORDED 3
#define SUPER_BLOCK_READ     4
 
//#define SUPER_BLOCK_ID       0xA1B2C3D4
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
#define F415_CHECK_ERROR          7


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

#pragma pack(push,1)  // ��������� adpcm ����� ������ (������ ������)
 typedef struct 
 { int16_t  prevsample;                 // ������ �����������
   int8_t   previndex;                  // ������ ���������� �����������
   int8_t   channel_id;                 // ����� ����� ������
   int8_t   adpcm_data[MAX_SAMPLE / 2]; // ADPCM ������
   uint64_t time;                       // ����� �������� �����  
   uint32_t crc;                        // ����������� ����� �����
 } adpcm_struct;
#pragma pack(pop)

#pragma pack(push,1)  // ��������� adpcm ����� ������ (���� �������)
 typedef struct  
 { uint32_t crc;                          // ����������� ����� ���������
   uint16_t msg_id;                       // ������������� ��������� 
   uint16_t msg_counter;                  // ������� ��������� (��� �������� ���������)
   adpcm_struct adpcm_block[MAX_CHANNEL]; // adpcm ���� ������
 } adpcm_message_struct;  
#pragma pack(pop)

#define ADPCM_BLOCK_SIZE  4048 // 4 + (36 * (224/2)) + 8 + 4 !!!! 4048
#define ADPCM_MAX_BLOCK   36
 
#pragma pack(push,1)   // ��������� adpcm ����� ������ ������������������ ��� �������� FLASH
 typedef struct
 { int16_t  prevsample;                                  // ������ �����������
   int8_t   previndex;                                   // ������ ���������� �����������
   int8_t   channel_id;                                  // ����� ����� ������
   int8_t   adpcm_data[ADPCM_MAX_BLOCK][MAX_SAMPLE / 2]; // ADPCM ������
   uint64_t time;                                        // ����� �������� ����� 
   uint32_t crc;                                         // ����������� ����� �����
 } adpcm_page_struct;
#pragma pack(pop) 
 
#pragma pack(push,1) // ��������������� ��������� ������������ ������ ( adpcm_message_struct->adpcm_page_struct )
typedef struct        
{ int id;                 // ��������� �� �������� SPI-DMA ������
  int adr;                // ������� ������ ������
  int done;               // ���� ����������
  uint32_t crc;           // ����������� �����
} adpcm_page_ctrl_struct;
#pragma pack(pop) 

#define UDP_DATA_SIZE    1026 
#define UDP_HEADER_SIZE  12
#define UDP_MESSAGE_SIZE UDP_HEADER_SIZE + UDP_DATA_SIZE

#pragma pack(push,1) // ��������� udp ���������
 typedef struct  
 { uint32_t msg_crc;             // ����������� �����
   uint32_t msg_id;              // ������������� ���������
   uint32_t msg_len;             // ������ ��������� � ������
   uint8_t  data[UDP_DATA_SIZE]; // ������
 } udp_message_struct;
#pragma pack(pop)

#define MAX_ERROR 23
 
#pragma pack(push,1)   // ��������� ����������� ������
 typedef struct  
 { 
   uint32_t eth_bsp_error;               // (0)     ������ ������������ �������� ��������
   uint32_t eth_udp_error[MAX_UDP_SOCK]; // (1+2+3) ������ ������������ udp ������
   uint32_t IS61_error;                  // (4)     ������ ������������ ������
   uint32_t f415_i2c_error;              // (5)     ������ ������ i2c(f207->f415)
   uint32_t read_eth_ini_error[2];       // (6+7)   ������ ������ ��������� ����������������� ������� ���������� (*)
   uint32_t read_total_time_error[2];    // (8+9)   ������ ������ ��������� ������ ������� ������ � ���������� ������ ������ FLASH
   uint32_t read_map_bb_error[2];        // (10+11) ������ ������������� ����� �������� ������� (��� �����)
   uint32_t nand_fs_error[2];            // (12+13) ������ ������������� �������� �������
   uint32_t rtc_error;                   // (14)    ������ RTC (*)
   uint32_t f_write_error;               // (15)    ������ ������ ����� 
   uint32_t server_error;                // (16)    ������ ������� ( ����������� ����������� ������ )
   uint32_t f415_spi1_error;             // (17)    ������ ������ spi(f415->f207) 
   uint32_t f415_spi2_error;             // (18)    ������ ������ spi(f415->pga112)
   uint32_t f415_adc_config_error;       // (19)    ������ ������������� ��� 
   uint32_t crc_binar_error;             // (20)    ������ crc ������ (�� binar �� f415) 
   uint32_t crc_wr_nand_error;           // (21)    ������ crc ������ �������� FLASH (�������� ����� �������)
   uint32_t crc_rd_nand_error;           // (22)    ������ crc ������ �������� FLASH (�������� ����� ������)
   
   uint32_t bad_block_number;            // ���������� ����������� ������ FLASH
   uint32_t nand_work_counter[2];        // ���������� ������ ������ FLASH (��������� �� �����) 
   uint32_t pga112_gain[16];             // �������� ������������� �������� PGA112
   uint32_t f415_mode;                   // ������� ����� ������ F415
   uint32_t f207_mode;                   // ������� ����� ������ F207
   uint64_t current_time;                // ����� �������
   uint64_t total_time;                  // ����� ����� ������ ����� ( ��������� �� ����� )
   
   uint8_t initial_done;
   uint8_t mask[MAX_ERROR];                     // ����� ����������� ������ ��� ������� ����������� ���������

 } total_info_struct;
#pragma pack(pop)

#pragma pack(push,1)    // ��������� I2C ���������
 typedef struct   
 { uint32_t msg_crc;    // ����������� �����
   uint16_t msg_len;    // ������ ��������� � ������
   uint16_t msg_id;     // ������������� ���������
   uint8_t data[256];   // ������
 } f415_message_struct;
#pragma pack(pop)
 
#ifdef L151 // IFDEF L151 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#pragma pack(push,1)  
 typedef struct   
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint16_t data[256];
 } L151_message_struct;
#pragma pack(pop)
 
 #pragma pack(push,1)   
 typedef struct
 { uint16_t data[512];
   uint16_t msg_len;
 } byte_stuffing_message_struct;  
#pragma pack(pop) 
 
#endif // IFDEF L151 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 
#pragma pack(push,1)  // ��������� �������������� ������� �������� �����
 typedef struct
 { uint32_t  page_index;        // ������ ���� ������ ( ADPCM_PAGE_ID, EONE_PAGE_ID )
   uint32_t  page_address;      // ���������� ������ �������� 
 } index_pointer_struct;
#pragma pack(pop)
 
#pragma pack(push,1)  // ��������� ��������� �����
 typedef struct
 { uint8_t  status;                         // ������� ��������� �����
   uint64_t time_open;                      // ����� �������� �����
   uint64_t time_close;                     // ����� �������� �����
   uint32_t sb_num;                         // ��� ����� ( ������������� - ����� )
   uint32_t page_real_write;                // ����������� ���������� ������� � �����
   uint32_t super_block_prev;               // ��������� �� ���������� ����
   uint32_t super_block_next;               // ��������� �� ��������� ����
   uint32_t crc_header;                     // ����������� ����� ���������� ��������� �����
   index_pointer_struct ips[MAX_DATA_PAGE]; // ��������� ���������� ������� ������� ����� 
   uint32_t crc_total;                      // ����������� ����� ���������� ��������� �����
 } super_block_struct;
#pragma pack(pop)
 
#pragma pack(push,1)   
 typedef struct 
 { uint32_t unit_index;     // ������ ��������� FLASH
   uint32_t sbrw;           // ������ ���������� ����������� �����
   uint64_t time[2];        // �����  ������� - ���������� ����������� �����
 } tab_struct[2];
#pragma pack(pop) 

#pragma pack(push,1)   
 typedef struct 
 { uint64_t total_time;      // ����� ����� ������ (� ��������)
   uint64_t total_cucle[2];  // ���������� ������ ������ ������ FLASH
   uint8_t  index;
   uint8_t  mode;
   uint32_t crc;             // ����������� �����
 } total_work_struct;
#pragma pack(pop) 

 
#pragma pack(push,1)
 typedef struct
 { 
   uint32_t index;                   // ��������� �� ������� �������� FLASH
   uint32_t state;                   // �������� ���������, �������������� �������� � ��������� ��������� FLASH 
   uint32_t super_block_real_write;  // ���������� ���������� ������
   uint64_t super_block_time[2];     // ����� ����. ������� � ����. ���������� ����������� ����� ���������
   uint32_t PageRealWrite;           // ���������� ���������� ������� � ������� �����
   uint32_t PageAdressErase;         // ������ ������� ��������� �������� FLASH
   uint32_t PageAdressWrite;         // ������ ������� ������������ �������� FLASH
   uint32_t super_block_begin;       // ����� ������� �����  
   uint32_t super_block_prev;        // ����� ���������� ����������� �����  
   uint32_t super_block_current;     // ����� �������� ������������� �����  
   uint32_t nand_real_write[2];      // ������� ������������ ��������� nand

 } alarm_struct;
#pragma pack(pop) 
 
 
#pragma pack(push,1)
 typedef struct
 { uint8_t  block_address[MAP_BB_MAX_BLOCK];
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

  
#pragma pack(push,1)   
typedef struct 
{
  uint32_t Zagolovok;
  
  uint32_t RootDelay;             //������ �� ������� ������� �������
  uint32_t RootDispersion;        //����������� �������� ��������� ������ ������������ ������� (0-��� �������) 
  uint32_t IdentificEtalon;
  uint32_t EtSecondMark;
  uint32_t EtDolySecondMark;

  uint32_t OriginateTimestampL;   // ����� ������� ������� � �������
  uint32_t OriginateTimestampH;   // 64 ����  
  
  uint32_t ReceiveTimestampL;     // ����� ������� ������� � �������
  uint32_t ReceiveTimestampH;     // 64 ����
  
  uint32_t TransmitTimestampL;    // ����� ������� ������� 64����
  uint32_t TransmitTimestampH;    // 64 ����
  
  uint32_t Autentificator;        // (��������) ������������, ����� ����������
                                  // ��������������, � �������� � ���� �������� ������������� � ���������
  uint32_t Dayjest1;              // ������ ��� �������������� ��������� MAC (Message Authentication Code)     
  uint32_t Dayjest2;              // 128 ���
  uint32_t Dayjest3;
  uint32_t Dayjest4;
 
} STNP_Struct;
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
void RTC_Config(void);
void ETH_Config(void);
void SysTim_Config(void);
void RNG_Config(void);
void CRC_Config(void);
uint32_t f415_SPI_Config(void);
uint32_t f415_Config(void);
void SuperBlock_Config(void);
void Data_Config(int mode);
void LoadTwsStruct(void);

void nand_erase_super_block(uint32_t id, uint32_t page);
void nand_erase_handler(void);
void nand_sb_write_handler(void);
void nand_sb_read_handler(void);
void SetTime(uint8_t *time);
int Restart_Handler(void);
void SaveTotalTimeTotalMode(uint8_t mode);

uint32_t crc32(void* pcBlock,  uint32_t len, uint32_t tot_len);
uint32_t crc32_t(uint32_t crc, void * pcBlock, uint32_t len, uint32_t tot_len);
void SRAM_WriteBuffer(uint16_t* pBuffer, uint32_t WriteAddr, uint32_t NumHalfwordToWrite);
void SRAM_ReadBuffer(uint16_t* pBuffer, uint32_t ReadAddr, uint32_t NumHalfwordToRead);
void SendEthInfoMessage(uint32_t msg_id, uint32_t param);
void SRAM_Test(void);
uint64_t GetTime(void);
void GetSuperBlockHeader(uint32_t un_index, uint32_t sb_index);
void GetSuperBlockPage(uint32_t page_address);
void cmd_handler(int id);
void SysTickTimer_Config(void);
void TIM2_Config(void);
void Set_SNTP_Timer(void);
