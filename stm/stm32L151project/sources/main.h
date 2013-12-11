#define MODE_ERR        111
#define MODE_SYNC       9
#define MODE_CMD        10
#define MODE_STREAM     11
#define MODE_SET_GAIN   12
#define MODE_GET_GAIN   14


#define MSG_STERAM   MODE_STREAM


#define DLE          0x0010
#define STX          0x0002
#define ETX          0x0003
#define DLE_DLE      0x00100010
#define DLE_STX      0x00100002 
#define DLE_ETX      0x00100003 

#define MAX_CHANNEL     16
#define MAX_SAMPLE      224

#define DMA_RX_SIZE     16
#define DMA_TX_SIZE     2048

#define RX_MSG_SIZE     100
#define TX_MSG_SIZE     5

void TIM2_Config(void);
void CRC_Config(void);
void ADC_Config(void);
void ADC_DMA_Config(void);
void TIM2_Enable(void);
void TIM2_Disable(void);
void SendF207Message(uint16_t *pbuffer,int size);

uint16_t Synchronization(void);
uint16_t Command_Handler(void);
uint8_t  ADPCMEncoder(int id, int16_t sample);
uint32_t crc32(uint8_t *pcBlock, int  len);

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


