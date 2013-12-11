#include "stm32l1xx_conf.h"
#include "spi_L151.h"
#include "spi_PGA112.h"
#include "stdio.h"
#include "stdlib.h"
#include "main.h"

const int16_t IndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

const int16_t StepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

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
 struct adpcm_message
 { uint32_t crc;
   uint16_t msg_id;
   uint16_t msg_counter;
   adpcm_struct adpcm_block[MAX_CHANNEL];
 } adpcm_msg[2];  
#pragma pack(pop)

#pragma pack(push,1)   
 struct spi_message
 { uint32_t msg_crc;
   uint16_t msg_len;
   uint16_t msg_id;
   uint16_t data[256];
 } tx_spi_msg, rx_spi_msg;  
#pragma pack(pop)
 
#pragma pack(push,1)   
 struct byte_stuffing_message
 { uint16_t data[512];
   uint16_t msg_len;
 } bs_tx_msg;  
#pragma pack(pop)
 
 
//uint16_t tx_msg[TX_MSG_SIZE] = {DLE,STX,0,DLE,ETX};
uint32_t msg_counter = 0; 
uint8_t  half_index = 0;
int8_t   msg_index = 0;
int16_t  prevsample[MAX_CHANNEL];
int16_t  previndex[MAX_CHANNEL];

uint16_t spi_tx_buffer[DMA_TX_SIZE];
uint16_t raw_buffer[MAX_CHANNEL];
int16_t  gain_mass[MAX_CHANNEL] = {GN1,GN0,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1,GN1};
uint16_t *p;

int msg_8bit_size;
int msg_16bit_size;
int msg_32bit_size;

int mode_work = MODE_CMD;

int16_t  wrw;  

  
void main()
{
  int index;

  ADC_DMA_Config(); 

  msg_8bit_size  = sizeof(adpcm_msg) / 2;
  msg_16bit_size = msg_8bit_size / 2;
  msg_32bit_size = msg_8bit_size / 4;
  
  SPI_PGA112Config();

  for(int i = 0; i < MAX_CHANNEL; i++) PGA112_WriteData(i,gain_mass[i],CH1);
  
  CRC_Config();

  TIM2_Config(); 
 
  tx_spi_msg.msg_len = 1;
  tx_spi_msg.msg_id  = MODE_SYNC;
  CRC_ResetDR();
  tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
  SendF207Message((uint16_t*)&tx_spi_msg,4);
 
  
  while(1)
  {
    switch(mode_work)
    {
      case MODE_ERR: while(1);
      break;

      case MODE_CMD: 
        mode_work = Command_Handler();
        TIM2_Enable();
      break;

      case MODE_STREAM:
        
      if((SPI1->SR & SPI_SR_RXNE))
       { if(SPI1->DR == DLE) 
        {
          mode_work = MODE_CMD;
          TIM2_Disable();
          continue;
        }
       }
       else if(half_index == MAX_SAMPLE) 
        { 
          index      = msg_index;
          msg_index ^= 1;
          half_index = 0;
          wrw        = 0;
            
          p = (uint16_t*)&adpcm_msg[index];
          adpcm_msg[index].msg_id = MSG_STERAM;
          adpcm_msg[index].msg_counter = msg_counter++;
          CRC_ResetDR();
          adpcm_msg[index].crc = CRC_CalcBlockCRC(((uint32_t*)&adpcm_msg[index])+1,msg_32bit_size-1);
          
          spi_tx_buffer[wrw++] = DLE;
          spi_tx_buffer[wrw++] = STX;
         
          for(int i = 0; i < msg_16bit_size; i++)
          {
            spi_tx_buffer[wrw++] = *(p++);  
            if(spi_tx_buffer[wrw-1] == DLE) spi_tx_buffer[wrw++] = DLE;
          }
      
          spi_tx_buffer[wrw++] = DLE;
          spi_tx_buffer[wrw++] = ETX;
         
          DMA1_Channel3->CCR  &= (uint16_t)(~DMA_CCR1_EN);
          DMA1_Channel3->CMAR  = (uint32_t)&spi_tx_buffer;
          DMA1_Channel3->CNDTR = 1500;
          DMA1_Channel3->CCR  |= DMA_CCR1_EN;
        }
      break;
    }
  }
}



void TIM2_IRQHandler(void)
{
  int16_t         sample;
  uint8_t         code;              
  int16_t         tempstep;   
  int16_t         diff;               
  int16_t         diffq;   
  int16_t         step;                      
  int32_t         predsample;          
  int8_t          index; 
  uint8_t         byte_index = half_index >> 1;
  uint8_t         bit_index  = half_index & 0x1;
  
  TIM_ClearITPendingBit(TIM2,TIM2_IRQn);
       
 // GPIOC->BSRRL = GPIO_Pin_7; 

  for(int i = 0; i < MAX_CHANNEL; i++)
  {
    
    if(half_index == 0)
    {
      adpcm_msg[msg_index].adpcm_block[i].prevsample = prevsample[i];
      adpcm_msg[msg_index].adpcm_block[i].previndex  = previndex[i];
      adpcm_msg[msg_index].adpcm_block[i].channel_id = i;
    }
    
    
    if(raw_buffer[i] > 4095)
    {
      mode_work = MODE_ERR;
      tx_spi_msg.msg_len = 1;
      tx_spi_msg.msg_id  = MODE_ERR;
      CRC_ResetDR();
      tx_spi_msg.msg_crc = CRC_CalcBlockCRC(((uint32_t*)&tx_spi_msg)+1,(uint32_t)tx_spi_msg.msg_len);
      SendF207Message((uint16_t*)&tx_spi_msg,4);
      return;
    }
    
    sample = (raw_buffer[i] * 16) - 32768;
    
// ADPCM CODER /////////////////////////////////////////////////////////////////    
    
    predsample = prevsample[i];
    index      = previndex[i];
    step       = StepSizeTable[index];
    diff       = sample - predsample;
  
    if(diff >= 0) 
    {
      code = 0;
    }
    else
    {
      code = 8;
      diff = -diff;
    }

// ÏÐßÌÎÉ ÊÂÀÍÒÎÂÀÒÅËÜ  ////////////////////////////////////////////////////////
  
    tempstep = step;
  
    if(diff >= tempstep)
    {
      code |= 4;
      diff -= tempstep;
    }
  
    tempstep >>= 1;

    if(diff >= tempstep)
    {
      code |= 2;
      diff -= tempstep;
    }

    tempstep >>= 1;

    if(diff >= tempstep) code |= 1;
    
  
// ÎÁÐÀÒÍÛÉ ÊÂÀÍÒÎÂÀÒÅËÜ /////////////////////////////////////////////////////// 
  
    diffq = step >> 3;
    
    if(code & 4) diffq += step;
    
    if(code & 2) diffq += step >> 1;
    
    if(code & 1) diffq += step >> 2;

// ÏÐÅÄÑÊÀÇÑÒÅËÜ /////////////////////////////////////////////////////////////// 

    if(code & 8) predsample -= diffq;
    else         predsample += diffq;

    if(predsample > 32767)       predsample = 32767;
    
    else if(predsample < -32768) predsample = -32768;
  
    index += IndexTable[code];

    if(index < 0)  index = 0;
    if(index > 88) index = 88;

    prevsample[i] = predsample;
    previndex[i] = index;
  
    code = code & 0x0f;

// ADPCM CODER ///////////////////////////////////////////////////////////////// 
    
    if(bit_index) adpcm_msg[msg_index].adpcm_block[i].adpcm_data[byte_index] |= code << 4;
    else          adpcm_msg[msg_index].adpcm_block[i].adpcm_data[byte_index]  = code;
    
  }

   ADC_SoftwareStartConv(ADC1);

   half_index++;
  
   //GPIOC->BSRRH = GPIO_Pin_7;
}

void SysTick_Handler(void)
{ 
  DMA1_Channel3->CCR  &= (uint16_t)(~DMA_CCR1_EN);
  DMA1_Channel3->CMAR = (uint32_t)&bs_tx_msg;  
  DMA1_Channel3->CNDTR = bs_tx_msg.msg_len;
  DMA1_Channel3->CCR  |= DMA_CCR1_EN;
}

void SendF207Message(uint16_t *pbuffer,int size)
{
  bs_tx_msg.msg_len = 0;
  
  bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  bs_tx_msg.data[bs_tx_msg.msg_len++] = STX;
  
  while(size-- > 0)
  {
    bs_tx_msg.data[bs_tx_msg.msg_len++] = *(pbuffer++);
    if(*(pbuffer-1) == DLE) bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  }
    
  bs_tx_msg.data[bs_tx_msg.msg_len++] = DLE;
  bs_tx_msg.data[bs_tx_msg.msg_len++] = ETX;
    
}

 
