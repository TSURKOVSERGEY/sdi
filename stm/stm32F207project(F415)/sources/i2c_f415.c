#include "main.h"
#include "i2c_f415.h"

extern total_info_struct      t_info;

uint32_t  f415_Config(void)
{
  uint32_t Timeout = 100; 
  uint8_t  f415_error[2];
  uint8_t  pga112_gain[16];
  
  t_info.f415_spi1_error = 1;
  
  // отключение режима аудио потока (если был)
  if(!f415_WriteMessage(F415_STOP_STREAM,NULL,0)) 
  {
      return 0;
  }

  delay(100);
  
  // проверка i2c интерфейса
  if(!f415_WriteMessage(F415_CHECK_I2C_CONNECT,NULL,0)) 
  {
     return 0;
  }

  delay(100);
  
  if(!f415_ReadMessage(F415_CHECK_I2C_CONNECT+100,NULL,0))  
  {
    return 0;
  }

  delay(100);
  
  // проверка spi интерфейса
  if(!f415_WriteMessage(F415_CHECK_SPI_CONNECT,NULL,0)) 
  {
    return 0;
  }
  
  while(t_info.f415_spi1_error == 1)
  { 
    if((Timeout--) == 0)
    {
      return 0;
    }
    else delay(10);
  }
  
  // проверка ошибок конфигурации F415 (L151)
   if(!f415_WriteMessage(F415_CHECK_ERROR,NULL,0)) 
   {
     return 0;
   }

  delay(100);
  
  if(f415_ReadMessage(F415_CHECK_ERROR+100,(uint8_t*)f415_error,sizeof(f415_error)))  
  {
    t_info.f415_spi2_error = f415_error[0];
    t_info.f415_adc_config_error = f415_error[1];
  }
  else return 0;

 // запрос коэффициентов усиления pga_112
  if(!f415_WriteMessage(F415_GET_GAIN,NULL,0)) 
  {
     return 0;
  }

  delay(100);
  
  if(f415_ReadMessage(F415_GET_GAIN+100,pga112_gain,sizeof(pga112_gain)))  
  {
    for(int i = 0; i < 16; i++)
    {
      t_info.pga112_gain[i] = pga112_gain[i];
    }

  }
  else return 0;

  
  
  
  return 1;
}

void I2C1_Config(void)
{
  I2C_InitTypeDef I2C_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_9; 
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);     
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);  
    
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  I2C_DeInit(I2C1);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x33;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  I2C_Init(I2C1, &I2C_InitStructure);
  
  I2C_Cmd(I2C1, ENABLE);  
  
  //f415_WriteMessage(F415_STOP_STREAM,NULL,0);

}

uint32_t f415_WriteMessage(uint16_t msg_id, uint8_t pbuffer[], uint8_t msg_len)
{
  f415_message_struct tx_f415_msg;
  
  tx_f415_msg.msg_id  = msg_id;
  tx_f415_msg.msg_len = msg_len;
  
  if(pbuffer)
  {
    for(int i = 0; i < msg_len; i++) tx_f415_msg.data[i] = pbuffer[i];
  }
  
  tx_f415_msg.msg_crc = crc32(((uint8_t*)&tx_f415_msg)+4,msg_len+4,msg_len+4); 

  if(t_info.f415_i2c_error = f415_Write((uint8_t*) &tx_f415_msg,msg_len+8)) return 0;
  
  return 1;
  
}

uint32_t f415_ReadMessage(uint16_t msg_id, uint8_t pbuffer[], uint8_t msg_len)
{
  f415_message_struct rx_f415_msg;
  
  if(t_info.f415_i2c_error = f415_Read((uint8_t*) &rx_f415_msg,msg_len+8)) return 0;
  else if(!rx_f415_msg.msg_crc == crc32(((uint8_t*)&rx_f415_msg)+4,msg_len+4,msg_len+4)) return 0;
       else if(rx_f415_msg.msg_id != msg_id) return 0;
            else if(pbuffer)
                 {
                   for(int i = 0; i < msg_len; i++) pbuffer[i] = rx_f415_msg.data[i];
                 }
  return 1;
}


uint32_t f415_Write(uint8_t *pdata, u16 size)
{
  uint32_t Timeout;

  I2C_AcknowledgeConfig(I2C1, ENABLE);
  
  Timeout = I2C_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 1; }
 
  I2C_GenerateSTART(I2C1, ENABLE);
  
  Timeout = I2C_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 1; }
 
  I2C_Send7bitAddress(I2C1, 0x50, I2C_Direction_Transmitter);

  Timeout = I2C_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { if((Timeout--) == 0) return 1; }
 
  while(size-- >0)
  {
    I2C_SendData(I2C1,*(pdata++));

    Timeout = I2C_TIMEOUT;   
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
      if((Timeout--) == 0) return 1; 
    }
  }
  
  I2C_AcknowledgeConfig(I2C1, DISABLE);
  
  I2C_GenerateSTOP(I2C1, ENABLE);

  return 0;
  
}


uint32_t f415_Read(uint8_t *pdata, u16 size)
{
  uint32_t Timeout;
   
  I2C_AcknowledgeConfig(I2C1, ENABLE);
    
  Timeout = I2C_TIMEOUT;   
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 1; }
 
  I2C_GenerateSTART(I2C1, ENABLE);
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 1; }
 
  I2C_Send7bitAddress(I2C1, 0x50, I2C_Direction_Receiver);
  
  while(size)
  {
     if(size == 1) I2C_AcknowledgeConfig(I2C1, DISABLE);
     Timeout = I2C_TIMEOUT;   
     while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED))
     {
       if((Timeout--) == 0) return 1; 
     }
     *(pdata++) = I2C_ReceiveData(I2C1);
     size--;
  }

  I2C_GenerateSTOP(I2C1, ENABLE);
  I2C_AcknowledgeConfig(I2C1,ENABLE);
  
  return 0;
}