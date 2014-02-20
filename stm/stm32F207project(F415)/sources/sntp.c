#include "main.h" 
#include "time.h" 
#include "ethernet.h" 

  

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DataStruct;
STNP_Struct     SNTP_Msg;
struct tm*      ptm_zapros;
struct tm*      ptm_otvet;
struct ip_addr  sntp_adr;

struct tm     tms;

uint32_t enable_sinc_flag = 0;
uint32_t dsec = 0;

/*
void sntp_handler(uint32_t id, void *p)
{
  uint32_t InpOtvetTimeDSec;
  
  if(id == GET_TIME + 100)
  {
    delay(0);
  }
  else if(id == GET_TIME + 200)
  {
    InpOtvetTimeDSec  = dsec;       
    memcpy(&SNTP_Msg,p,sizeof(SNTP_Msg));
    dsec = SNTP_Msg.TransmitTimestampH + ((InpOtvetTimeDSec - SNTP_Msg.OriginateTimestampH))/2;
    enable_sinc_flag = 1;
  }
} 
*/

void Set_SNTP_Timer(void)
{
}
 

void TIM2_IRQHandler(void)
{
  static int oldsecond = 0;     
  static int zapros_time = 0;    
  
  ptm_zapros = &tms;
    
  TIM_ClearITPendingBit(TIM2,TIM2_IRQn);
    
  if(enable_sinc_flag)  
  {
    if(dsec >= 999)
    {
      ptm_otvet = localtime((uint32_t*)&SNTP_Msg + 11);

      RTC_TimeStruct.RTC_Hours   = ptm_otvet->tm_hour;
      RTC_TimeStruct.RTC_Minutes = ptm_otvet->tm_min;
      RTC_TimeStruct.RTC_Seconds = ptm_otvet->tm_sec + 1;
      RTC_SetTime(0, &RTC_TimeStruct);  

      RTC_DataStruct.RTC_Date =  ptm_otvet->tm_mday;  
      RTC_DataStruct.RTC_Month = ptm_otvet->tm_mon; 
      RTC_DataStruct.RTC_Year =  ptm_otvet->tm_year  - 100; 
      RTC_SetDate(0, &RTC_DataStruct);  
      
      oldsecond = ptm_otvet->tm_sec + 1;

      dsec = 0;
      
      enable_sinc_flag = 0;
      
      return; 
    }
    dsec++; return;
  }  
  
  dsec++;

  RTC_GetTime(0, &RTC_TimeStruct);
  
  if(RTC_TimeStruct.RTC_Seconds !=  oldsecond) // ÑÈÍÕÐÎÍÈÇÀÖÈß ÏÎ ÑÅÊÓÍÄÅ
  {    
    dsec = 0;
    
   // GPIO_ToggleBits(GPIOI, GPIO_Pin_1); 

    oldsecond =  RTC_TimeStruct.RTC_Seconds;
    
 
    if(zapros_time++ >= 10) // ÇÀÏÐÎÑ ÐÀÇ Â 10 ÑÅÊÓÍÄ
    {
      zapros_time = 0;

        
      RTC_GetDate(0, &RTC_DataStruct);
      
      ptm_zapros->tm_sec   = RTC_TimeStruct.RTC_Seconds; 
      ptm_zapros->tm_min   = RTC_TimeStruct.RTC_Minutes;
      ptm_zapros->tm_hour  = RTC_TimeStruct.RTC_Hours;  
      ptm_zapros->tm_mday  = RTC_DataStruct.RTC_Date;  
      ptm_zapros->tm_mon   = RTC_DataStruct.RTC_Month;  
      ptm_zapros->tm_year  = RTC_DataStruct.RTC_Year + 2000 - 1900;  
      ptm_zapros->tm_isdst = 0;
    
      SNTP_Msg.OriginateTimestampL = mktime(ptm_zapros);    
      SNTP_Msg.OriginateTimestampH = dsec;                        
        
      SendMessage(SNTP,GET_TIME,&SNTP_Msg,sizeof(SNTP_Msg));
    }
  }
} 

