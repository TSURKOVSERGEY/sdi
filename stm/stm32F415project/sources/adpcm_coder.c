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

extern adpcm_message_struct adpcm_msg[2];  
extern int8_t               msg_index;

uint16_t                    raw_buffer[DMA_ADC_SIZE];
uint32_t                    msg_counter = 0; 
uint8_t                     half_index = 0;
int16_t                     prevsample[MAX_CHANNEL];
int16_t                     previndex[MAX_CHANNEL];


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
       
  for(int i = 0; i < MAX_CHANNEL; i++)
  {
    
    if(half_index == 0)
    {
      adpcm_msg[msg_index].adpcm_block[i].prevsample = prevsample[i];
      adpcm_msg[msg_index].adpcm_block[i].previndex  = previndex[i];
      adpcm_msg[msg_index].adpcm_block[i].channel_id = i;
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
}