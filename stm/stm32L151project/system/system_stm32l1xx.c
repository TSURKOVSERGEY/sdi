#include "stm32l1xx.h"

void SetSysClock(void);

void SystemInit (void)
{
  /*!< Set MSION bit */
  RCC->CR |= (uint32_t)0x00000100;

  /*!< Reset SW[1:0], HPRE[3:0], PPRE1[2:0], PPRE2[2:0], MCOSEL[2:0] and MCOPRE[2:0] bits */
  RCC->CFGR &= (uint32_t)0x88FFC00C;
  
  /*!< Reset HSION, HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xEEFEFFFE;

  /*!< Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /*!< Reset PLLSRC, PLLMUL[3:0] and PLLDIV[1:0] bits */
  RCC->CFGR &= (uint32_t)0xFF02FFFF;

  /*!< Disable all interrupts */
  RCC->CIR = 0x00000000;
    
   SetSysClock();

}


void SetSysClock(void)
{
  __IO uint32_t StartUpCounter = 0, HSEStatus = 0;
  
  
  RCC->CR |= ((uint32_t)RCC_CR_HSEON);

  do
  {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;  
  } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET)
  {
    HSEStatus = (uint32_t)0x01;
  }
  else
  {
    HSEStatus = (uint32_t)0x00;
  }  

  if (HSEStatus == (uint32_t)0x01)
  { 
    
    // Enable the PWR APB1 Clock 
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  
    // Select the Voltage Range 1 (1.8V) 
    PWR->CR = PWR_CR_VOS_0;
  
    // Wait Until the Voltage Regulator is ready 
    while((PWR->CSR & PWR_CSR_VOSF) != RESET)
    {
    }    
    
    

    FLASH->ACR |= (uint32_t)FLASH_ACR_ACC64;   
    
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY;   

    FLASH->ACR |= FLASH_ACR_PRFTEN;
    
    // HCLK = SYSCLK 
    RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV2;
      
    // PCLK2 = HCLK 
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
    // PCLK1 = HCLK 
    RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV1;

    //  PLL configuration: PLLCLK = (HSE * 32) / 4 = 32MHz 
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL |
                                        RCC_CFGR_PLLDIV));
    
    RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL32 | RCC_CFGR_PLLDIV4);

    // Enable PLL 
    RCC->CR |= RCC_CR_PLLON;

    // Wait till PLL is ready 
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }
        
    // Select PLL as system clock source 
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

    // Wait till PLL is used as system clock source 
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x0C)
    {
    }
  }
  else
  { 
    // If HSE fails to start-up, the application will have wrong clock 
     //  configuration. User can add here some code to deal with this error     
  }
}

