
void I2C1_Config(void);
uint32_t f415_Write(uint8_t *pdata, u16 size);
uint32_t f415_Read(uint8_t *pdata, u16 size);
uint32_t f415_WriteMessage(uint16_t msg_id, uint8_t pbuffer[], uint8_t msg_len);
uint32_t f415_ReadMessage(uint16_t msg_id, uint8_t pbuffer[], uint8_t msg_len);

