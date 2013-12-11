#include"main.h"
#include"at24c512.h"

uint32_t AT24_Write(uint16_t Addr, uint8_t *pdata, int size)
{
  while(size >= 0)
  {
    if(AT24_PageWrite(Addr,pdata)) return 1;
    delay(100);
    Addr+=128;
    pdata+=128;
    size-=128;
  }
  return 0;
}
 
uint32_t AT24_PageWrite(uint16_t Addr, uint8_t *pdata)
{
  uint32_t Timeout;
  
  I2C_AcknowledgeConfig(I2C1, ENABLE);
  
  Timeout = I2C_TIMEOUT;
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 1; }
 
  I2C_GenerateSTART(I2C1, ENABLE);
  
  Timeout = I2C_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 1; }
 
  I2C_Send7bitAddress(I2C1, 0xA0, I2C_Direction_Transmitter);
 
  Timeout = I2C_TIMEOUT; 
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { if((Timeout--) == 0) return 1; }
 
  I2C_SendData(I2C1, (uint8_t)((Addr & 0xFF00) >> 8));
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 1; }
 
  I2C_SendData(I2C1, (uint8_t)(Addr & 0x00FF));
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  { if((Timeout--) == 0) return 1; }
 
  for(int i = 0; i < 128; i++)
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


uint32_t AT24_Read(uint16_t Addr, uint8_t *pdata, int size)
{
  uint32_t Timeout;
   
  I2C_AcknowledgeConfig(I2C1, ENABLE);
    
  Timeout = I2C_TIMEOUT;   
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) { if((Timeout--) == 0) return 1; }
 
  I2C_GenerateSTART(I2C1, ENABLE);
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 1; }
 
  I2C_Send7bitAddress(I2C1, 0xA0, I2C_Direction_Transmitter);

  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) { if((Timeout--) == 0) return 1; }
 
  I2C_SendData(I2C1, (uint8_t)((Addr & 0xFF00) >> 8));
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 1; }
 
  I2C_SendData(I2C1, (uint8_t)(Addr & 0x00FF));
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) { if((Timeout--) == 0) return 0; }
 
  I2C_GenerateSTART(I2C1, ENABLE);
 
  Timeout = I2C_TIMEOUT;   
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) { if((Timeout--) == 0) return 1; }
 
  I2C_Send7bitAddress(I2C1, 0xA1, I2C_Direction_Receiver);
 
  while(size-- > 0)
  {
    Timeout = I2C_TIMEOUT;   
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED))
    {
      if((Timeout--) == 0) return 1; 
    }
    *(pdata++) = I2C_ReceiveData(I2C1);
  }
  
  I2C_AcknowledgeConfig(I2C1, DISABLE);
  I2C_GenerateSTOP(I2C1, ENABLE);
  return 0;
}





