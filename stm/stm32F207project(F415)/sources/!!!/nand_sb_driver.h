#include "stm32f2xx.h"


#define REWRITE_BLOCK        1
#define REWRITE_BLOCK_NEXT   2

#define HEADER_MODE          1
#define DATA_MODE            2

uint64_t GetTime(void);
uint32_t SuperBlock_Config(void);
int  CheckBlockGetPageAdr(int mode);
int  ReadFile(uint32_t file_begin, uint32_t file_end);
int  write_page(uint8_t *page_bufer,unsigned long int page);
//void reset_bad_block_map(void);
void WriteNandPage(void *padpcm_msg);
void WriteSuperBlockHeader(void);
void rewrite_block(int mode_rewrite,uint8_t *page_buffer_old);
void erase_block(uint32_t id, uint32_t start_adr, uint32_t end_adr);
void SetPageReadAdr(uint32_t id,uint32_t start_adr, uint32_t end_adr);
void SetSuperBlockAdr(uint32_t page_begin, uint32_t page_end);
void SendDataHandler(void);
void NandWriteDataHandler(void);
//void DebugWritePaga(uint8_t *pbuffer,uint32_t mode);
void ReadSuperBlockHeader(uint32_t adr, int read_mode);
void GetSuperBlockInfo(uint8_t id, void *p);

////////////////////////////////////////////////////////////////////////////////
//
//  ������������� ������ EEPROM - AT24C512
//
//  ����� ����� 65536 ���� (512 ������� �� 128 ����)
//
// *****************************************************************************
//  ������[0-19] ( information )
//
//  uint32_t product_id
//  uint32_t product_ver
//  
//
// *****************************************************************************
//  ������[20-39] ( alarm_struct )
//
//  uint32_t id
//  uint32_t target_index
//  uint32_t SuperBlockRealWrite
//  uint32_t address
//  uint32_t crc
//
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  MT29F4 ���� �������� - 2048 + 64(��������)
//  MT29F4 ���� ����     - 64 �������� 
//  MT29F4 ���� �������  -  2048 ������
//
// *****************************************************************************
//  ����� ���� 
//  
//  ��� �������� ������ 1 ���� - 64 ��������    (64 * 2048 = 131072) ���� �� ���������
//  ������ �������� � ������� ������� [64*255]  (64*255*(4+4) = 130560) ���� �� ������
//  ��� ��������� ���������� ��������� �������� (131072 - 130560 = 512) ���� �� ���.���.
//
//  ������ ����������� � ����������� 255-������, �������� (64*255=16320) �������
//
// *****************************************************************************
//  ���������� ����� �����   ( 64 * 2048 = 131072)
//
//  uint32_t id;                         4
//  uint8_t  status;                     1
//  uint64_t time_open;                  8
//  uint64_t time_close;                 8
//  uint32_t super_block_prev;           4
//  uint32_t super_block_next;           4
//  index_pointer_struct ips[16320];        
//  uint32_t crc;                        4  
// 
//  DATA (2048 * 64 * 255 = 33'423'360)
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ������������� ������ SRAM - IS61
//
//  ����� ����� 525'288 16 ��������� ����  
//
//  ��������� ����� 0x68000000   
//
//
////////////////////////////////////////////////////////////////////////////////