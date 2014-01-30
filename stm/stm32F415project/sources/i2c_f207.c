#include "main.h"
#include "spi_f207.h"

#define I2C_SPEED            100000
#define I2C_TIMEOUT          0xffffffff


uint8_t I2C_InputBuffer_index  = 0;
uint8_t I2C_OutputBuffer_index = 0;
uint8_t I2C_NumberOfByteToTransmit = 0;

uint8_t I2C_InputBuffer[264];
uint8_t I2C_OutputBuffer[264];

f207_message_struct* prx_f207_msg = (f207_message_struct*)I2C_InputBuffer;
f207_message_struct* ptx_f207_msg = (f207_message_struct*)I2C_OutputBuffer;

extern int8_t               msg_index;
extern uint8_t              gain[16];
extern adpcm_message_struct adpcm_msg[2]; 
extern int                  msg_8bit_size;
extern int                  msg_16bit_size;
extern int                  msg_32bit_size;
extern uint16_t             spi_tx_buffer[DMA_SPI_SIZE];

uint32_t I2C_Config(void)
{
  NVIC_InitTypeDef  NVIC_InitStructure;
  I2C_InitTypeDef I2C_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);     
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C1);  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C1);  
    
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
  I2C_DeInit(I2C2);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x50;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
 
  I2C_Cmd(I2C2, ENABLE);
  I2C_Init(I2C2, &I2C_InitStructure);
  I2C_AcknowledgeConfig(I2C1,ENABLE);
   
  NVIC_InitStructure.NVIC_IRQChannel =  I2C2_EV_IRQn; 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  I2C_ITConfig(I2C2,I2C_IT_EVT, ENABLE);
  
  return 0;
}

void f207_ReciveHandler(uint16_t msg_id)
{
  int i, wrw = 0;
  uint16_t *p;
            
  switch(msg_id)
  {
    case F415_CHECK_I2C_CONNECT:
      
      ptx_f207_msg->msg_id  = F415_CHECK_I2C_CONNECT + 100;
      ptx_f207_msg->msg_len = 0;
      ptx_f207_msg->msg_crc = crc32(((uint8_t*)ptx_f207_msg)+4,ptx_f207_msg->msg_len+4,ptx_f207_msg->msg_len+4);
      
    break;
    
    case F415_CHECK_SPI_CONNECT:
      
          p = (uint16_t*)&adpcm_msg[msg_index];
          adpcm_msg[msg_index].msg_id = F415_CHECK_SPI_CONNECT+100;
          adpcm_msg[msg_index].msg_counter = 0;
          CRC_ResetDR();
          adpcm_msg[msg_index].crc = CRC_CalcBlockCRC(((uint32_t*)&adpcm_msg[msg_index])+1,msg_32bit_size-1);
      
          
          spi_tx_buffer[wrw++] = DLE;
          spi_tx_buffer[wrw++] = STX;
         
          for(int i = 0; i < msg_16bit_size; i++)
          {
            spi_tx_buffer[wrw++] = *(p++);  
            if(spi_tx_buffer[wrw-1] == DLE) spi_tx_buffer[wrw++] = DLE;
          }
      
          spi_tx_buffer[wrw++] = DLE;
          spi_tx_buffer[wrw++] = ETX;
          
          SPI_f207SendMessage(spi_tx_buffer,1500);
    
        
    break;  
    
    case F415_SET_GAIN:
      
      for(i = 0; i < 16; i++) gain[i] = prx_f207_msg->data[i];

    break;
    
    case F415_GET_GAIN:
      
      ptx_f207_msg->msg_id  = F415_GET_GAIN + 100;
      ptx_f207_msg->msg_len = 16;
      for(i = 0; i < 16; i++) ptx_f207_msg->data[i] = gain[i];
      ptx_f207_msg->msg_crc = crc32(((uint8_t*)ptx_f207_msg)+4,ptx_f207_msg->msg_len+4,ptx_f207_msg->msg_len+4);
     
    break;    
    
    case F415_START_STREAM: 
      
      TIM2_Enable();
    
    break;    

    case F415_STOP_STREAM: 
      
      TIM2_Disable();
    
    break;    

    
  }
}

void I2C2_EV_IRQHandler(void)
{
  uint32_t status = I2C_GetLastEvent(I2C2);
  
  switch(status & 0xFFFFFEFF)
  {
    case 0x00020402: 
    case 0x00020002: 
      
     I2C_ClearFlag(I2C2,I2C_FLAG_ADDR);
     I2C_InputBuffer_index = 0;
        
    break;
    
    case 0x00060482: 
    case 0x00060082:  
      
     I2C_OutputBuffer_index = 0;
     I2C_SendData(I2C2,I2C_OutputBuffer[I2C_OutputBuffer_index++]);
     I2C_ClearFlag(I2C2,I2C_FLAG_AF);
     I2C2->CR1 |= 0x1; 
     
    break;
        
    case 0x00060484:
    case 0x00060084:
      
     I2C_SendData(I2C2,I2C_OutputBuffer[I2C_OutputBuffer_index++]);
     
    break;
       
    case 0x450:
    case 0x50:
      
     I2C2->CR1 |= 0x1; 

     I2C_InputBuffer[I2C_InputBuffer_index++] = I2C_ReceiveData(I2C2);  
     I2C_ClearFlag(I2C2,I2C_FLAG_RXNE);
     I2C_ClearFlag(I2C2,I2C_FLAG_STOPF);

     if(prx_f207_msg->msg_crc == crc32(((uint8_t*)prx_f207_msg)+4,prx_f207_msg->msg_len+4,prx_f207_msg->msg_len+4))
     {
       f207_ReciveHandler(prx_f207_msg->msg_id);
     }
     
    break;
    
    case 0x00020444:
    case 0x00020044:
      
     I2C_InputBuffer[I2C_InputBuffer_index++] = I2C_ReceiveData(I2C2);    
     I2C_ClearFlag(I2C2,I2C_FLAG_RXNE);
      
    break;
   }
  
  printf("%d",status);
}


uint32_t I2C_Write(uint8_t *pdata, int size)
{
  uint32_t Timeout;
  
  while(size-- >0)
  {
    I2C_SendData(I2C2,*(pdata++));

    Timeout = I2C_TIMEOUT;   
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); { if((Timeout--) == 0) return 0; }
  }
  
  return 1;
}
