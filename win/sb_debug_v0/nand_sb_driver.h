
typedef char           int8_t;
typedef unsigned char  uint8_t; 
typedef short          int16_t;
typedef unsigned short uint16_t;
typedef int            int32_t; 
typedef unsigned int   uint32_t; 
typedef long		   int64_t;
typedef unsigned long  uint64_t;

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#define MAX_BLOCK            4096

#define BLOCK_GOOD           0x1
#define BLOCK_BAD            0x2

#define SUPER_BLOCK_ID       0xA1B2C3D4
#define ADPCM_PAGE_ID        2

#define MAX_CHANNEL          16
#define MAX_SAMPLE           224

#define SUPER_BLOCK_CLEAR    1
#define SUPER_BLOCK_OPEN     2
#define SUPER_BLOCK_RECORDED 3
#define SUPER_BLOCK_READ     4

#define ADR_MAP_BB           0

#define REWRITE_BLOCK        1
#define REWRITE_BLOCK_NEXT   2


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#pragma pack(push,1)
 typedef struct
 { uint32_t bad_block_number;
   uint8_t  block_address[MAX_BLOCK];
   uint32_t crc;
 } bad_block_map_struct;
#pragma pack(pop)  


#pragma pack(push,1)
 typedef struct
 { uint32_t id;
   uint32_t TargetIndex;
   uint32_t PageRealWrite;
   uint32_t PageAddress;  
   
   uint32_t super_block_prev;
   uint32_t super_block_current;
   
   uint32_t crc;   
 } alarm_struct;
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
   uint32_t super_block_prev;
   uint32_t super_block_next;
   index_pointer_struct ips[16320];
   uint32_t crc;
 } super_block_struct;
#pragma pack(pop)

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

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

int CheckBlockGetPageAdr(void);
void WriteNandPage(adpcm_message_struct *padpcm_msg);
void WriteSuperBlockHeader(void);
int  nand_write_page(uint16_t *page_bufer,unsigned long int page,FILE *fp);
int nand_read_page(uint16_t *page_bufer,unsigned long int page,FILE *fp);
uint64_t GetTime(void);
uint32_t SuperBlock_Config(void);
uint32_t AT45_Read(uint16_t Addr, uint8_t *pdata, int size);
void rewrite_block(int mode_rewrite,uint16_t *page_bufer_old);
void OnClose(void);