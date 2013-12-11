

uint32_t AT24_PageWrite(uint16_t Addr, uint8_t *pdata);
uint32_t AT24_Write(uint16_t Addr, uint8_t *pdata, int size);
uint32_t AT24_Read(uint16_t Addr, uint8_t *pdata, int size);
uint32_t I2C_Write(uint8_t *pdata, int size);
uint32_t I2C_Read(uint8_t *pdata, u16 size);
