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

#define MAP_BB_MAX_BLOCK     4092  // максимальное количество блоков FLASH (для карты исправности блоков)
#define MAP_BB_ADDRES        4095  // физический адрес блока FLASH ( располажения карты исправности блоков )
#define TWS_ADDRES           4094  // физический адрес блока FLASH ( общего времени наработки на отказ )

#define DATA_CONFIG_LOAD     1
#define DATA_CONFIG_NEW      2
#define ERASE_MODE           1
#define NOT_ERASE_MODE       2


////////////////////////////////////////////////////////////////////////////////
// MT24 array organization
// страница - 2048 байт действительных данных, + 64 бита служебной
// блок - 64 страницы
// всего 4096 блоков
////////////////////////////////////////////////////////////////////////////////

#define SB_HEADER_SIZE       33 // размер заголовка структуры файла (без crc и IPS)
#define BEGIN_PAGE           (MAX_PAGE_IN_NAND - 6000) //0
#define MAX_FILE             1000
#define MAX_ERASE_PAGE       PAGE_IN_SBLOCK 
#define MAX_PAGE_IN_NAND     261888 // (4092 * 64 )  // максимальное колличество страниц на всей флеш ADR = (0..4091)
#define PAGE_IN_BLOCK        64
#define MAX_DATA_PAGE        256 // максимальное кол-во страниц в супер блоке
#define PAGE_IN_SBLOCK       ( MAX_DATA_PAGE + 64 )
#define MAX_SBLOCK           100
#define MAX_BLOCK            ( MAX_SBLOCK * PAGE_IN_SBLOCK / 64 ) // (100 * (256 + 64) / 64 = 500) // не более (4096 - 4 = 4092)


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
// идентификаторы сетевых команд

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

// доп команды сетевых настроек

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

#pragma pack(push,1)  // структура adpcm блока данных (одного канала)
 typedef struct 
 { int16_t  prevsample;                 // синхро предвыборка
   int8_t   previndex;                  // синхро прединдекс квантования
   int8_t   channel_id;                 // номер аудио канала
   int8_t   adpcm_data[MAX_SAMPLE / 2]; // ADPCM данные
   uint64_t time;                       // время создания блока  
   uint32_t crc;                        // контрольная сумма блока
 } adpcm_struct;
#pragma pack(pop)

#pragma pack(push,1)  // структура adpcm блока данных (всех каналов)
 typedef struct  
 { uint32_t crc;                          // контрольная сумма сообщения
   uint16_t msg_id;                       // идентификатор сообщения 
   uint16_t msg_counter;                  // счетчик сообщений (для контроля пропусков)
   adpcm_struct adpcm_block[MAX_CHANNEL]; // adpcm блок данных
 } adpcm_message_struct;  
#pragma pack(pop)

#define ADPCM_BLOCK_SIZE  4048 // 4 + (36 * (224/2)) + 8 + 4 !!!! 4048
#define ADPCM_MAX_BLOCK   36
 
#pragma pack(push,1)   // структура adpcm блока данных отформатированного для страницы FLASH
 typedef struct
 { int16_t  prevsample;                                  // синхро предвыборка
   int8_t   previndex;                                   // синхро прединдекс квантования
   int8_t   channel_id;                                  // номер аудио канала
   int8_t   adpcm_data[ADPCM_MAX_BLOCK][MAX_SAMPLE / 2]; // ADPCM данные
   uint64_t time;                                        // время создания блока 
   uint32_t crc;                                         // контрольная сумма блока
 } adpcm_page_struct;
#pragma pack(pop) 
 
#pragma pack(push,1) // вспомогательная структура переупаковки данных ( adpcm_message_struct->adpcm_page_struct )
typedef struct        
{ int id;                 // указатель на активный SPI-DMA буффер
  int adr;                // текущий адресс записи
  int done;               // флаг готовности
  uint32_t crc;           // контрольная сумма
} adpcm_page_ctrl_struct;
#pragma pack(pop) 

#define UDP_DATA_SIZE    1026 
#define UDP_HEADER_SIZE  12
#define UDP_MESSAGE_SIZE UDP_HEADER_SIZE + UDP_DATA_SIZE

#pragma pack(push,1) // структура udp сообщений
 typedef struct  
 { uint32_t msg_crc;             // контрольная сумма
   uint32_t msg_id;              // идентификатор сообщения
   uint32_t msg_len;             // длинна сообщения в байтах
   uint8_t  data[UDP_DATA_SIZE]; // данные
 } udp_message_struct;
#pragma pack(pop)

#define MAX_ERROR 23
 
#pragma pack(push,1)   // структура исправности блоков
 typedef struct  
 { 
   uint32_t eth_bsp_error;               // (0)     ошибка конфигурации сетевого драйвера
   uint32_t eth_udp_error[MAX_UDP_SOCK]; // (1+2+3) ошибка конфигурации udp сокета
   uint32_t IS61_error;                  // (4)     ошибка динамической памяти
   uint32_t f415_i2c_error;              // (5)     ошибка канала i2c(f207->f415)
   uint32_t read_eth_ini_error[2];       // (6+7)   ошибка чтения структуры инициализационных сетевых параметров (*)
   uint32_t read_total_time_error[2];    // (8+9)   ошибка чтения структуры общего времени работы и количества циклов записи FLASH
   uint32_t read_map_bb_error[2];        // (10+11) ошибка инициализации карты файловой системы (две копии)
   uint32_t nand_fs_error[2];            // (12+13) ошибка инициализации файловой системы
   uint32_t rtc_error;                   // (14)    ошибка RTC (*)
   uint32_t f_write_error;               // (15)    ошибка записи файла 
   uint32_t server_error;                // (16)    ошибка сервера ( колличество несчитанных файлов )
   uint32_t f415_spi1_error;             // (17)    ошибка канала spi(f415->f207) 
   uint32_t f415_spi2_error;             // (18)    ошибка канала spi(f415->pga112)
   uint32_t f415_adc_config_error;       // (19)    ошибка инициализации АЦП 
   uint32_t crc_binar_error;             // (20)    ошибка crc данных (по binar от f415) 
   uint32_t crc_wr_nand_error;           // (21)    ошибка crc данных страницы FLASH (проверка перед записью)
   uint32_t crc_rd_nand_error;           // (22)    ошибка crc данных страницы FLASH (проверка после чтения)
   
   uint32_t bad_block_number;            // количество испорченных блоков FLASH
   uint32_t nand_work_counter[2];        // количество циклов записи FLASH (наработка на отказ) 
   uint32_t pga112_gain[16];             // значения коэффициентов усиления PGA112
   uint32_t f415_mode;                   // текущий режим работы F415
   uint32_t f207_mode;                   // текущий режим работы F207
   uint64_t current_time;                // время текущее
   uint64_t total_time;                  // общее время работы платы ( наработки на отказ )
   
   uint8_t initial_done;
   uint8_t mask[MAX_ERROR];                     // маска разрешенных ошибок для анализа светодиодом индекации

 } total_info_struct;
#pragma pack(pop)

#pragma pack(push,1)    // структура I2C сообщения
 typedef struct   
 { uint32_t msg_crc;    // контрольная сумма
   uint16_t msg_len;    // длинна сообщения в байтах
   uint16_t msg_id;     // идентификатор сообщения
   uint8_t data[256];   // данные
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
 
#pragma pack(push,1)  // структура переназначения адресса страницы файла
 typedef struct
 { uint32_t  page_index;        // индекс типа данных ( ADPCM_PAGE_ID, EONE_PAGE_ID )
   uint32_t  page_address;      // физический адресс страницы 
 } index_pointer_struct;
#pragma pack(pop)
 
#pragma pack(push,1)  // структура заголовка файла
 typedef struct
 { uint8_t  status;                         // текущее состояние файла
   uint64_t time_open;                      // время создания файла
   uint64_t time_close;                     // время закрытия файла
   uint32_t sb_num;                         // имя файла ( идентификатор - номер )
   uint32_t page_real_write;                // колличество записанных страниц в файле
   uint32_t super_block_prev;               // указатель на предидущий файл
   uint32_t super_block_next;               // указатель на следующий файл
   uint32_t crc_header;                     // контрольная сумма начального заголовка файла
   index_pointer_struct ips[MAX_DATA_PAGE]; // структура физический адресов страниц файла 
   uint32_t crc_total;                      // контрольная сумма целикового заголовка файла
 } super_block_struct;
#pragma pack(pop)
 
#pragma pack(push,1)   
 typedef struct 
 { uint32_t unit_index;     // индекс комплекиа FLASH
   uint32_t sbrw;           // индекс последнего записанного файла
   uint64_t time[2];        // время  первого - последнего записанного файла
 } tab_struct[2];
#pragma pack(pop) 

#pragma pack(push,1)   
 typedef struct 
 { uint64_t total_time;      // общее время работы (в секундах)
   uint64_t total_cucle[2];  // количество полных циклов записи FLASH
   uint8_t  index;
   uint8_t  mode;
   uint32_t crc;             // контрольная сумма
 } total_work_struct;
#pragma pack(pop) 

 
#pragma pack(push,1)
 typedef struct
 { 
   uint32_t index;                   // указатель на текущий комплект FLASH
   uint32_t state;                   // параметр состояние, микропрограммы стирания и изменения комплекта FLASH 
   uint32_t super_block_real_write;  // количество записанных файлов
   uint64_t super_block_time[2];     // время откр. первого и закр. последнего записанного файла комплекта
   uint32_t PageRealWrite;           // количество записанных страниц в текущем файле
   uint32_t PageAdressErase;         // адресс текущей стираемой страницы FLASH
   uint32_t PageAdressWrite;         // адресс текущей записываемой страницы FLASH
   uint32_t super_block_begin;       // адрес первого файла  
   uint32_t super_block_prev;        // адрес последнего записанного файла  
   uint32_t super_block_current;     // адрес текущего записываемого файла  
   uint32_t nand_real_write[2];      // признак заполнености комплекта nand

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
  
  uint32_t RootDelay;             //секунд до эталона точного времени
  uint32_t RootDispersion;        //номинальное значение временной ошибки относительно эталона (0-сот милисек) 
  uint32_t IdentificEtalon;
  uint32_t EtSecondMark;
  uint32_t EtDolySecondMark;

  uint32_t OriginateTimestampL;   // время запроса клиента к серверу
  uint32_t OriginateTimestampH;   // 64 бита  
  
  uint32_t ReceiveTimestampL;     // время прихода запроса к серверу
  uint32_t ReceiveTimestampH;     // 64 бита
  
  uint32_t TransmitTimestampL;    // время отклика клиенту 64бита
  uint32_t TransmitTimestampH;    // 64 бита
  
  uint32_t Autentificator;        // (опционно) используется, когда необходима
                                  // аутентификация, и содержит в себе ключевой идентификатор и сообщение
  uint32_t Dayjest1;              // хранит код аутентификации сообщения MAC (Message Authentication Code)     
  uint32_t Dayjest2;              // 128 бит
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
