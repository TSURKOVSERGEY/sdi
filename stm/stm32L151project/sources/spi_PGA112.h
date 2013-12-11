
#define CSn     16
 
typedef enum {CS0,CS1,CS2,CS3,CS4,CS5,CS6,CS7,CS8,CS9,CS10,CS11,CS12,CS13,CS14,CS15} CS_TypeDef;
 
#define CS0_PIN                          GPIO_Pin_6
#define CS0_GPIO_PORT                    GPIOC
#define CS0_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS1_PIN                          GPIO_Pin_7
#define CS1_GPIO_PORT                    GPIOC
#define CS1_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS2_PIN                          GPIO_Pin_8
#define CS2_GPIO_PORT                    GPIOC
#define CS2_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS3_PIN                          GPIO_Pin_9
#define CS3_GPIO_PORT                    GPIOC
#define CS3_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS4_PIN                          GPIO_Pin_10
#define CS4_GPIO_PORT                    GPIOC
#define CS4_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS5_PIN                          GPIO_Pin_11
#define CS5_GPIO_PORT                    GPIOC
#define CS5_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS6_PIN                          GPIO_Pin_12
#define CS6_GPIO_PORT                    GPIOC
#define CS6_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS7_PIN                          GPIO_Pin_13
#define CS7_GPIO_PORT                    GPIOC
#define CS7_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS8_PIN                          GPIO_Pin_14
#define CS8_GPIO_PORT                    GPIOC
#define CS8_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS9_PIN                          GPIO_Pin_15
#define CS9_GPIO_PORT                    GPIOC
#define CS9_GPIO_CLK                     RCC_AHBPeriph_GPIOC

#define CS10_PIN                         GPIO_Pin_4
#define CS10_GPIO_PORT                   GPIOB
#define CS10_GPIO_CLK                    RCC_AHBPeriph_GPIOB

#define CS11_PIN                         GPIO_Pin_5
#define CS11_GPIO_PORT                   GPIOB
#define CS11_GPIO_CLK                    RCC_AHBPeriph_GPIOB

#define CS12_PIN                         GPIO_Pin_6
#define CS12_GPIO_PORT                   GPIOB
#define CS12_GPIO_CLK                    RCC_AHBPeriph_GPIOB

#define CS13_PIN                         GPIO_Pin_7
#define CS13_GPIO_PORT                   GPIOB
#define CS13_GPIO_CLK                    RCC_AHBPeriph_GPIOB

#define CS14_PIN                         GPIO_Pin_8
#define CS14_GPIO_PORT                   GPIOB
#define CS14_GPIO_CLK                    RCC_AHBPeriph_GPIOB

#define CS15_PIN                         GPIO_Pin_9
#define CS15_GPIO_PORT                   GPIOB
#define CS15_GPIO_CLK                    RCC_AHBPeriph_GPIOB


void SPI_PGA112Config(void);

void SPI_PGA112GPIO_Config(void);

void PGA112_WriteData(int CS, unsigned short gain, unsigned short channel);

void SPI_PGA112_CS_GPIO_Config(CS_TypeDef CS);

void PGA112_Set_CS(CS_TypeDef CS);

void PGA112_Clr_CS(CS_TypeDef CS);

